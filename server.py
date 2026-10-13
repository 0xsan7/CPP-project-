"""Local account and quiz API backed by the existing C++ quiz engine."""

from __future__ import annotations

import hashlib
import hmac
import json
import os
import random
import re
import secrets
import sqlite3
import subprocess
import threading
import time
from datetime import datetime
from http.cookies import SimpleCookie
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parent
WEB = ROOT / "dist"
SOURCE = ROOT / "quiz_app.cpp"
BINARY = ROOT / "build" / "quiz_app"
DATABASE = Path(os.environ.get("QUIZ_DB", ROOT / "quizline.sqlite3"))
PORT = int(os.environ.get("QUIZ_PORT", "8765"))
COOKIE_NAME = "quizline_session"
SESSION_LIFETIME = 7 * 24 * 60 * 60
PASSWORD_ITERATIONS = 310_000
USERNAME_PATTERN = re.compile(r"[a-zA-Z0-9_.-]{3,24}\Z")
QUIZZES: dict[str, dict] = {}
QUIZ_LOCK = threading.Lock()
LOGIN_ATTEMPTS: dict[str, list[float]] = {}
ATTEMPT_LOCK = threading.Lock()
CATALOG: list[dict] = []


class ApiError(Exception):
    """Represent an expected API error with an HTTP status."""

    def __init__(self, status: int, message: str):
        self.status = status
        super().__init__(message)


def build_backend() -> None:
    """Compile the C++ engine when its executable is missing or stale."""
    if BINARY.exists() and BINARY.stat().st_mtime >= SOURCE.stat().st_mtime:
        return
    BINARY.parent.mkdir(exist_ok=True)
    subprocess.run(
        ["g++", "-std=c++17", "-Wall", "-Werror", "-o", str(BINARY), str(SOURCE)],
        cwd=ROOT,
        check=True,
    )


def build_frontend() -> None:
    """Build the React frontend when its source or configuration changes."""
    sources = [
        *ROOT.joinpath("web").rglob("*"),
        *ROOT.joinpath("components").rglob("*"),
        *ROOT.joinpath("styles").rglob("*"),
        *ROOT.joinpath("lib").rglob("*"),
        ROOT / "package.json", ROOT / "package-lock.json",
        ROOT / "vite.config.ts", ROOT / "tsconfig.json",
    ]
    output = WEB / "index.html"
    if output.exists() and all(
        path.stat().st_mtime <= output.stat().st_mtime
        for path in sources if path.is_file()
    ):
        return
    if not (ROOT / "node_modules").exists():
        raise RuntimeError("Install frontend dependencies with npm install, then restart.")
    subprocess.run(["npm", "run", "build"], cwd=ROOT, check=True)


def backend(*args: str) -> object:
    """Run a JSON-producing C++ quiz command."""
    result = subprocess.run(
        [str(BINARY), "--web", *args],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=8,
        check=True,
    )
    return json.loads(result.stdout)


def connect_db() -> sqlite3.Connection:
    """Open one SQLite connection with foreign keys and row access enabled."""
    connection = sqlite3.connect(DATABASE, timeout=10)
    connection.row_factory = sqlite3.Row
    connection.execute("PRAGMA foreign_keys = ON")
    return connection


def init_db() -> None:
    """Create account, login-session, and account-owned score tables."""
    DATABASE.parent.mkdir(parents=True, exist_ok=True)
    with connect_db() as db:
        db.executescript(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY,
                username TEXT NOT NULL UNIQUE,
                display_name TEXT NOT NULL,
                password_salt BLOB NOT NULL,
                password_hash BLOB NOT NULL,
                created_at TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS auth_sessions (
                token_hash BLOB PRIMARY KEY,
                user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                csrf_token TEXT NOT NULL,
                expires_at INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS auth_sessions_user_id ON auth_sessions(user_id);
            CREATE TABLE IF NOT EXISTS scores (
                id INTEGER PRIMARY KEY,
                user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
                score INTEGER NOT NULL CHECK(score >= 0),
                total_questions INTEGER NOT NULL CHECK(total_questions > 0),
                correct_answers INTEGER NOT NULL CHECK(correct_answers >= 0),
                percentage REAL NOT NULL,
                category TEXT NOT NULL,
                difficulty TEXT NOT NULL,
                played_at TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS scores_user_id ON scores(user_id, id DESC);
            CREATE INDEX IF NOT EXISTS scores_rank ON scores(score DESC, percentage DESC);
            """
        )
        db.execute("DELETE FROM auth_sessions WHERE expires_at <= ?", (int(time.time()),))
    os.chmod(DATABASE, 0o600)


def hash_password(password: str, salt: bytes) -> bytes:
    """Derive a password hash using a unique salt and PBKDF2."""
    return hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, PASSWORD_ITERATIONS)


def valid_password(password: object) -> bool:
    """Check password length without changing or trimming its contents."""
    return isinstance(password, str) and 8 <= len(password) <= 128


def valid_display_name(value: object) -> bool:
    """Check that a display name is readable and bounded."""
    return (
        isinstance(value, str)
        and 2 <= len(value.strip()) <= 32
        and value == value.strip()
        and all(ord(ch) >= 32 and ord(ch) != 127 for ch in value)
    )


def public_user(row: sqlite3.Row) -> dict:
    """Return non-secret account fields for a signed-in user."""
    return {
        "id": row["id"],
        "username": row["username"],
        "displayName": row["display_name"],
        "joined": row["created_at"],
    }


def score_record(row: sqlite3.Row) -> dict:
    """Convert one SQLite score row to the browser's score shape."""
    return {
        "name": row["display_name"],
        "score": row["score"],
        "totalQuestions": row["total_questions"],
        "correctAnswers": row["correct_answers"],
        "percentage": row["percentage"],
        "category": row["category"],
        "difficulty": row["difficulty"],
        "date": row["played_at"],
    }


def grade_band(percentage: float) -> str:
    """Map accuracy to the same grade thresholds as the C++ app."""
    if percentage >= 90:
        return "S"
    if percentage >= 75:
        return "A"
    if percentage >= 60:
        return "B"
    if percentage >= 40:
        return "C"
    return "D"


def public_question(quiz: dict) -> dict:
    """Return the active question without its correct answer."""
    question = quiz["questions"][quiz["index"]]
    return {**question, "number": quiz["index"] + 1, "total": len(quiz["questions"])}


def recent_stats(db: sqlite3.Connection, user_id: int) -> dict:
    """Calculate the signed-in player's profile statistics."""
    totals = db.execute(
        """
        SELECT COUNT(*) AS games, COALESCE(SUM(score), 0) AS points,
               COALESCE(AVG(score), 0) AS average,
               COALESCE(MAX(score), 0) AS best
        FROM scores WHERE user_id = ?
        """,
        (user_id,),
    ).fetchone()
    return {
        "games": totals["games"],
        "points": totals["points"],
        "average": round(totals["average"], 1),
        "best": totals["best"],
    }


class Handler(SimpleHTTPRequestHandler):
    """Serve the local frontend and its authenticated quiz API."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WEB), **kwargs)

    def json_response(self, status: int, value: object, cookie: str | None = None) -> None:
        """Write a JSON response and optional session cookie."""
        data = json.dumps(value, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        if cookie is not None:
            self.send_header("Set-Cookie", cookie)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def read_json(self) -> dict:
        """Read a small JSON object from the request."""
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError as error:
            raise ApiError(400, "Invalid request size") from error
        if length < 1 or length > 4096:
            raise ApiError(400, "Invalid request size")
        try:
            value = json.loads(self.rfile.read(length))
        except (UnicodeError, json.JSONDecodeError) as error:
            raise ApiError(400, "Invalid JSON") from error
        if not isinstance(value, dict):
            raise ApiError(400, "Expected a JSON object")
        return value

    def origin_is_valid(self) -> bool:
        """Allow write requests only from this origin when Origin is supplied."""
        origin = self.headers.get("Origin")
        if not origin:
            return True
        host = self.headers.get("Host", "")
        return origin in (f"http://{host}", f"https://{host}")

    def session_cookie(self, token: str, max_age: int = SESSION_LIFETIME) -> str:
        """Build a cookie that JavaScript cannot read."""
        secure = "; Secure" if os.environ.get("QUIZ_HTTPS") == "1" else ""
        return (
            f"{COOKIE_NAME}={token}; Path=/; HttpOnly; SameSite=Lax; "
            f"Max-Age={max_age}{secure}"
        )

    def session_token(self) -> str | None:
        """Read the opaque login token from the request cookie."""
        cookie = SimpleCookie()
        try:
            cookie.load(self.headers.get("Cookie", ""))
        except Exception:
            return None
        return cookie[COOKIE_NAME].value if COOKIE_NAME in cookie else None

    def current_user(self, db: sqlite3.Connection) -> tuple[sqlite3.Row, str, str]:
        """Resolve the cookie to an unexpired account session."""
        token = self.session_token()
        if not token:
            raise ApiError(401, "Sign in to continue")
        token_hash = hashlib.sha256(token.encode("utf-8")).digest()
        row = db.execute(
            """
            SELECT users.*, auth_sessions.csrf_token
            FROM auth_sessions JOIN users ON users.id = auth_sessions.user_id
            WHERE auth_sessions.token_hash = ? AND auth_sessions.expires_at > ?
            """,
            (token_hash, int(time.time())),
        ).fetchone()
        if row is None:
            raise ApiError(401, "Session expired. Sign in again")
        return row, row["csrf_token"], token

    def require_csrf(self, csrf_token: str) -> None:
        """Reject an authenticated write without the session's CSRF token."""
        supplied = self.headers.get("X-CSRF-Token", "")
        if not hmac.compare_digest(supplied, csrf_token):
            raise ApiError(403, "Refresh the page and try again")

    def create_login(self, db: sqlite3.Connection, user_id: int) -> tuple[str, str]:
        """Persist a new opaque login token and CSRF token."""
        token = secrets.token_urlsafe(32)
        csrf = secrets.token_urlsafe(24)
        db.execute(
            "INSERT INTO auth_sessions(token_hash, user_id, csrf_token, expires_at) VALUES (?, ?, ?, ?)",
            (hashlib.sha256(token.encode()).digest(), user_id, csrf, int(time.time()) + SESSION_LIFETIME),
        )
        return token, csrf

    def rate_limit_key(self, username: str) -> str:
        """Group failed sign-ins by local address and username."""
        return f"{self.client_address[0]}:{username}"

    def check_rate_limit(self, username: str) -> None:
        """Limit repeated wrong-password attempts for one username."""
        key = self.rate_limit_key(username)
        now = time.monotonic()
        with ATTEMPT_LOCK:
            attempts = [stamp for stamp in LOGIN_ATTEMPTS.get(key, []) if now - stamp < 900]
            LOGIN_ATTEMPTS[key] = attempts
            if len(attempts) >= 5:
                raise ApiError(429, "Too many sign-in attempts. Try again in 15 minutes")

    def record_failed_login(self, username: str) -> None:
        """Record a failed sign-in for rate limiting."""
        with ATTEMPT_LOCK:
            LOGIN_ATTEMPTS.setdefault(self.rate_limit_key(username), []).append(time.monotonic())

    def do_GET(self) -> None:
        """Serve static files and authenticated profile or score views."""
        path = urlparse(self.path).path
        try:
            if path == "/api/health":
                self.json_response(200, {"ok": True})
                return
            if path == "/api/me":
                with connect_db() as db:
                    user, csrf, _ = self.current_user(db)
                    self.json_response(200, {
                        "user": public_user(user),
                        "csrfToken": csrf,
                        "stats": recent_stats(db, user["id"]),
                    })
                return
            if path == "/api/leaderboard":
                with connect_db() as db:
                    rows = db.execute(
                        """
                        SELECT users.display_name, scores.* FROM scores
                        JOIN users ON users.id = scores.user_id
                        ORDER BY scores.score DESC, scores.percentage DESC, scores.id DESC
                        LIMIT 10
                        """
                    ).fetchall()
                    entries = [score_record(row) for row in rows]
                legacy = backend("scores")
                entries.extend({**entry, "category": "Legacy"} for entry in legacy)
                entries.sort(key=lambda entry: (-entry["score"], -entry["percentage"]))
                self.json_response(200, {"entries": entries[:10]})
                return
            if path in ("/api/history", "/api/profile"):
                with connect_db() as db:
                    user, _, _ = self.current_user(db)
                    rows = db.execute(
                        """
                        SELECT users.display_name, scores.* FROM scores
                        JOIN users ON users.id = scores.user_id
                        WHERE scores.user_id = ? ORDER BY scores.id DESC
                        """,
                        (user["id"],),
                    ).fetchall()
                    entries = [score_record(row) for row in rows]
                    if path == "/api/history":
                        self.json_response(200, {"entries": entries})
                    else:
                        self.json_response(200, {
                            "user": public_user(user),
                            "stats": recent_stats(db, user["id"]),
                            "recent": entries[:5],
                        })
                return
            if path.startswith("/api/"):
                raise ApiError(404, "Unknown endpoint")
            super().do_GET()
        except ApiError as error:
            self.json_response(error.status, {"error": str(error)})
        except (sqlite3.Error, subprocess.SubprocessError, OSError):
            self.json_response(500, {"error": "Scores unavailable"})

    def do_POST(self) -> None:
        """Register, sign in, edit profiles, and play authenticated quizzes."""
        try:
            if not self.origin_is_valid():
                raise ApiError(403, "Invalid request origin")
            payload = self.read_json()
            path = urlparse(self.path).path
            if path == "/api/register":
                username = payload.get("username")
                display_name = payload.get("displayName")
                password = payload.get("password")
                if not isinstance(username, str) or not USERNAME_PATTERN.fullmatch(username):
                    raise ApiError(400, "Username must be 3–24 letters, numbers, dots, dashes, or underscores")
                if not valid_display_name(display_name):
                    raise ApiError(400, "Display name must be 2–32 readable characters")
                if not valid_password(password):
                    raise ApiError(400, "Password must be 8–128 characters")
                username = username.lower()
                salt = secrets.token_bytes(16)
                with connect_db() as db:
                    try:
                        cursor = db.execute(
                            """
                            INSERT INTO users(username, display_name, password_salt, password_hash, created_at)
                            VALUES (?, ?, ?, ?, ?)
                            """,
                            (
                                username, display_name, salt, hash_password(password, salt),
                                datetime.now().astimezone().strftime("%Y-%m-%d %H:%M"),
                            ),
                        )
                    except sqlite3.IntegrityError as error:
                        raise ApiError(409, "That username is taken") from error
                    token, csrf = self.create_login(db, cursor.lastrowid)
                    user = db.execute("SELECT * FROM users WHERE id = ?", (cursor.lastrowid,)).fetchone()
                    db.commit()
                self.json_response(201, {"user": public_user(user), "csrfToken": csrf},
                                   self.session_cookie(token))
                return

            if path == "/api/login":
                username = payload.get("username")
                password = payload.get("password")
                if not isinstance(username, str) or not isinstance(password, str):
                    raise ApiError(400, "Enter your username and password")
                username = username.strip().lower()
                self.check_rate_limit(username)
                with connect_db() as db:
                    user = db.execute("SELECT * FROM users WHERE username = ?", (username,)).fetchone()
                    salt = user["password_salt"] if user else b"\0" * 16
                    candidate = hash_password(password, salt)
                    expected = user["password_hash"] if user else b"\0" * 32
                    if user is None or not hmac.compare_digest(candidate, expected):
                        self.record_failed_login(username)
                        raise ApiError(401, "Invalid username or password")
                    token, csrf = self.create_login(db, user["id"])
                    db.commit()
                with ATTEMPT_LOCK:
                    LOGIN_ATTEMPTS.pop(self.rate_limit_key(username), None)
                self.json_response(200, {"user": public_user(user), "csrfToken": csrf},
                                   self.session_cookie(token))
                return

            with connect_db() as db:
                user, csrf, token = self.current_user(db)
                self.require_csrf(csrf)
                if path == "/api/logout":
                    db.execute(
                        "DELETE FROM auth_sessions WHERE token_hash = ?",
                        (hashlib.sha256(token.encode()).digest(),),
                    )
                    with QUIZ_LOCK:
                        QUIZZES.pop(token, None)
                    db.commit()
                    self.json_response(200, {"ok": True}, self.session_cookie("", 0))
                    return

                if path == "/api/profile":
                    display_name = payload.get("displayName")
                    if not valid_display_name(display_name):
                        raise ApiError(400, "Display name must be 2–32 readable characters")
                    current_password = payload.get("currentPassword")
                    new_password = payload.get("newPassword")
                    if current_password is not None or new_password is not None:
                        if not isinstance(current_password, str) or not valid_password(new_password):
                            raise ApiError(400, "Enter your current password and a new password of at least 8 characters")
                        if not hmac.compare_digest(
                            hash_password(current_password, user["password_salt"]),
                            user["password_hash"],
                        ):
                            raise ApiError(403, "Current password is incorrect")
                        salt = secrets.token_bytes(16)
                        db.execute(
                            "UPDATE users SET display_name = ?, password_salt = ?, password_hash = ? WHERE id = ?",
                            (display_name, salt, hash_password(new_password, salt), user["id"]),
                        )
                        db.execute(
                            "DELETE FROM auth_sessions WHERE user_id = ? AND token_hash != ?",
                            (user["id"], hashlib.sha256(token.encode()).digest()),
                        )
                    else:
                        db.execute("UPDATE users SET display_name = ? WHERE id = ?",
                                   (display_name, user["id"]))
                    updated = db.execute("SELECT * FROM users WHERE id = ?", (user["id"],)).fetchone()
                    db.commit()
                    self.json_response(200, {"user": public_user(updated)})
                    return

                if path == "/api/start":
                    category = payload.get("category")
                    difficulty = payload.get("difficulty")
                    if category not in ("Science", "History", "Technology", "Sports", "Random") or (
                        difficulty not in ("Easy", "Medium", "Hard", "All")
                    ):
                        raise ApiError(400, "Choose a valid category and difficulty")
                    if category == "Random":
                        category = secrets.choice(["Science", "History", "Technology", "Sports"])
                    questions = [
                        question for question in CATALOG
                        if question["category"] == category
                        and (difficulty == "All" or question["difficulty"] == difficulty)
                    ]
                    random.SystemRandom().shuffle(questions)
                    quiz = {
                        "user_id": user["id"],
                        "category": category,
                        "difficulty": difficulty,
                        "questions": questions,
                        "index": 0,
                        "score": 0,
                        "correct": 0,
                        "started": time.monotonic(),
                    }
                    with QUIZ_LOCK:
                        QUIZZES[token] = quiz
                    self.json_response(200, {
                        "category": category,
                        "difficulty": difficulty,
                        "question": public_question(quiz),
                    })
                    return

                if path in ("/api/hint", "/api/answer", "/api/next"):
                    with QUIZ_LOCK:
                        quiz = QUIZZES.get(token)
                        if quiz is None or quiz["user_id"] != user["id"]:
                            raise ApiError(409, "Quiz expired. Start a new quiz")
                        if path == "/api/next":
                            if quiz["started"] is not None:
                                raise ApiError(409, "Answer this question first")
                            quiz["started"] = time.monotonic()
                            self.json_response(200, {"question": public_question(quiz)})
                            return
                        if quiz["started"] is None:
                            raise ApiError(409, "Open the next question first")
                        question = quiz["questions"][quiz["index"]]
                        if path == "/api/hint":
                            self.json_response(200, backend("hint", question["id"]))
                            return
                        answer = payload.get("answer")
                        if not isinstance(answer, str) or len(answer) > 8:
                            raise ApiError(400, "Invalid answer")
                        elapsed = max(0.0, time.monotonic() - quiz["started"])
                        feedback = backend("grade", question["id"], answer.strip().upper(), str(elapsed))
                        quiz["score"] += feedback["points"]
                        quiz["correct"] += feedback["outcome"] == "correct"
                        quiz["index"] += 1
                        quiz["started"] = None
                        done = quiz["index"] == len(quiz["questions"])
                        response = {"feedback": feedback, "done": done}
                        if done:
                            total = len(quiz["questions"])
                            percentage = 100 * quiz["correct"] / total
                            played_at = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M")
                            db.execute(
                                """
                                INSERT INTO scores(user_id, score, total_questions, correct_answers,
                                                   percentage, category, difficulty, played_at)
                                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                                """,
                                (
                                    user["id"], quiz["score"], total, quiz["correct"], percentage,
                                    quiz["category"], quiz["difficulty"], played_at,
                                ),
                            )
                            db.commit()
                            response["result"] = {
                                "score": quiz["score"],
                                "correct": quiz["correct"],
                                "total": total,
                                "percentage": percentage,
                                "grade": grade_band(percentage),
                                "saved": True,
                                "category": quiz["category"],
                                "difficulty": quiz["difficulty"],
                            }
                            del QUIZZES[token]
                    self.json_response(200, response)
                    return
            raise ApiError(404, "Unknown endpoint")
        except ApiError as error:
            self.json_response(error.status, {"error": str(error)})
        except (sqlite3.Error, subprocess.SubprocessError, OSError):
            self.json_response(500, {"error": "Server error. Please try again"})


def main() -> None:
    """Start the loopback server with its database and C++ question catalog."""
    global CATALOG
    build_backend()
    build_frontend()
    init_db()
    CATALOG = backend("catalog")
    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print(f"Quizline: http://127.0.0.1:{PORT}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
