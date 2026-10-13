# Product Requirements Document
## C++ Interactive Quiz Application — Enhanced Edition

**Project:** `quiz_app`
**Language:** C++17
**Target:** CLI (Terminal / Console)
**Author:** \[Your Name / Student ID\]
**Date:** 2026-10-13
**Codex Orchestrator:** `claude-sonnet-5-5` (sol)
**Codex Sub-Agent:** `claude-haiku-5-5` (luna)

---

## 1. Project Overview

A fully-featured, terminal-based interactive quiz game written in C++17.
The application lets players take quizzes across multiple knowledge categories, tracks their scores in a persistent binary/text file, and provides leaderboards and per-player history across sessions.

---

## 2. Goals

| Goal | Description |
|---|---|
| Educational | Demonstrate C++ file I/O, structs, STL containers, chrono, and OOP patterns |
| Functional | A complete, playable quiz game with real replay value |
| Persistent | Scores survive program restarts via `scores.dat` |
| Extensible | New categories and questions can be added with zero structural changes |

---

## 3. Features (Implemented)

### 3.1 Core Quiz Engine
- **4 categories:** Science, History, Technology, Sports
- **5 questions per category** (20 total), each with 4 MCQ options
- **3 difficulty levels:** Easy (10 pts / 20 s), Medium (20 pts / 25 s), Hard (30 pts / 30 s)
- **Mixed / All** mode that combines every difficulty in one session
- **Random category picker** at menu selection
- **Answer shuffling** (Fisher-Yates) per session so replays feel fresh

### 3.2 Scoring System
- Base points awarded per correct answer (10 / 20 / 30 by difficulty)
- **Time bonus:** up to 50 % extra points for fast answers (scales linearly)
- Zero points for wrong answers or skips — no penalty to encourage engagement
- Grade scale: S (≥90 %), A (≥75 %), B (≥60 %), C (≥40 %), D (<40 %)

### 3.3 Hint System
- Each question has a pre-written hint string
- Player types `H` before answering to reveal the hint (answer still scored normally)

### 3.4 File Handling (`scores.dat`)
- **Append-mode writes** — every completed session appended as one pipe-delimited line
- **Fields stored:** `name | score | totalQ | correct | percentage | date | difficulty`
- **Fault-tolerant reader** — malformed lines silently skipped; missing file handled gracefully
- File path is a compile-time constant (`SCORE_FILE`) for easy override

### 3.5 Leaderboard
- Loaded from `scores.dat`, sorted by score desc → accuracy desc
- Displays top 10 entries with medal emoji (🥇🥈🥉) for top 3
- ANSI color highlights: Gold / Silver / Bronze rows

### 3.6 Player History
- Filter leaderboard data to the current player's name
- Show per-session breakdown: score, correct/total, accuracy %, difficulty, date
- Summary footer: total games, total points, average points per game

### 3.7 Multi-Player / Name Change
- Players enter name once at startup
- **Change Player** menu option — allows switching names mid-session
- Each name's scores are independently tracked

### 3.8 UX / Terminal
- ANSI color-coded output (green = correct, red = wrong, yellow = options/prompts)
- Clear screen between major views (portable `system("clear")` / `system("cls")`)
- Input validation loops — bad input never crashes the program
- Skip option (`S`) for any question

---

## 4. File Structure

```
quiz_project/
├── quiz_app.cpp        ← Single-file implementation (all logic)
├── PRD.md              ← This document
├── scores.dat          ← Auto-created at runtime; persists across runs
└── quiz_app            ← Compiled binary (after build)
```

---

## 5. Build Instructions

### Linux / macOS
```bash
g++ -std=c++17 -Wall -o quiz_app quiz_app.cpp
./quiz_app
```

### Windows (MinGW)
```cmd
g++ -std=c++17 -Wall -o quiz_app.exe quiz_app.cpp
quiz_app.exe
```

### Requirements
- g++ ≥ 7 or clang++ ≥ 5 with C++17 support
- Terminal with ANSI escape support (all modern terminals)
- No external libraries — stdlib only

---

## 6. Data Model

### Question struct
```cpp
struct Question {
    string text;
    vector<string> options;  // exactly 4
    int correctIndex;        // 0-based
    string hint;
    string difficulty;       // "Easy" | "Medium" | "Hard"
    int timeLimit;           // seconds
    int points;              // base award
};
```

### PlayerScore struct
```cpp
struct PlayerScore {
    string name;
    int score;
    int totalQuestions;
    int correctAnswers;
    double percentage;
    string date;       // "YYYY-MM-DD HH:MM"
    string difficulty;
};
```

### scores.dat format (one line per session)
```
Alice|90|5|4|80.0|2026-10-13 14:22|Medium
```

---

## 7. Codex Agent Instructions

> **Orchestrator model:** `claude-sonnet-5-5` (sol)
> **Sub-agent model:** `claude-haiku-5-5` (luna)

### 7.1 Orchestrator Tasks (sol — claude-sonnet-5-5)

Sol is responsible for high-level planning, architecture decisions, and code review.

**Responsibilities:**
1. Parse this PRD and decompose work into discrete sub-tasks
2. Assign each sub-task to luna with a focused prompt
3. Review luna's code output for correctness and style
4. Merge sub-tasks into the final `quiz_app.cpp`
5. Run the build (`g++ -std=c++17 -Wall -Werror`) and confirm zero errors/warnings
6. Write the final test plan and verify each feature manually

**Orchestrator system prompt (paste into Claude Code):**
```
You are sol, the orchestrator agent for the quiz_app C++ project.
You coordinate luna (claude-haiku-5-5) to implement specific modules.
For each feature below, write a focused prompt for luna, collect its output,
review for correctness, and integrate into quiz_app.cpp.
Always compile with: g++ -std=c++17 -Wall -Werror -o quiz_app quiz_app.cpp
Do not proceed to the next feature until the build is clean.

Features to implement in order:
1. Question bank (structs + 4 categories × 5 questions)
2. File I/O (saveScore, loadScores using pipe-delimited text)
3. Quiz engine (askQuestion with timer, hint, skip)
4. Scoring (base points + time bonus + grade calculation)
5. Leaderboard (sorted display, top 10, medal ranks)
6. Player history (filter by name, summary footer)
7. Main menu loop (category select, difficulty select, replay, quit)
8. UX polish (ANSI colors, input validation, clear screen)
```

### 7.2 Sub-Agent Tasks (luna — claude-haiku-5-5)

Luna implements individual modules as instructed by sol.

**Luna's rules:**
- Write C++17; use stdlib only (no Boost, no third-party headers)
- Every function must have a one-line comment describing its purpose
- No global mutable state except `SCORE_FILE` constant
- Match the struct definitions in Section 6 exactly
- Return complete, compilable code snippets — not pseudocode

**Example luna prompt (sent by sol):**
```
Implement the `saveScore(const PlayerScore& ps)` and
`vector<PlayerScore> loadScores()` functions in C++17.
Use append-mode ofstream for saving.
Fields are pipe-delimited in this order:
  name|score|totalQ|correct|percentage|date|difficulty
Handle missing file and malformed lines gracefully.
Include required headers. No global state.
```

### 7.3 Agent Interaction Pattern

```
sol reads PRD
  └─► sol writes sub-task prompt → luna
        luna returns code snippet
      sol reviews + compiles
        ├── PASS → integrate, move to next task
        └── FAIL → sol corrects prompt → luna again
sol runs full build
sol runs manual test checklist (Section 8)
sol commits final quiz_app.cpp
```

---

## 8. Test Checklist

Run these manually after building:

| # | Test | Expected |
|---|---|---|
| T1 | Launch app | Splash + name prompt |
| T2 | Enter name, choose Science/Easy | 2 questions appear, A-D options visible |
| T3 | Answer correctly within time | Green "✓ Correct! +X pts" with time bonus |
| T4 | Answer incorrectly | Red "✗ Wrong!" shows correct answer |
| T5 | Type `H` before answering | Hint line displayed, then re-prompted |
| T6 | Type `S` to skip | "Skipped." shown, 0 pts, next question |
| T7 | Type invalid input (`Z`) | Red "Invalid — marked wrong" |
| T8 | Complete a full session | Results screen with score, grade |
| T9 | Check `scores.dat` after session | New line appended in correct format |
| T10 | View leaderboard | Top 10 rows, medals on top 3 |
| T11 | View My History | Only entries for current player shown |
| T12 | Change player name | New name used in all subsequent saves |
| T13 | Quit | Goodbye screen shown, program exits 0 |
| T14 | Run twice | Second session scores appended, not overwritten |
| T15 | Delete `scores.dat`, view leaderboard | "No scores yet" message, no crash |

---

## 9. Known Limitations & Future Work

| Item | Notes |
|---|---|
| Time enforcement | Timer is cosmetic — actual cutoff not enforced (user types at their own pace). Future: use `select()` or threads for hard cutoff |
| Question bank | Hard-coded in source. Future: load from `questions.json` or `questions.csv` |
| Scores file | Plain text pipe-delimited. Future: SQLite for richer queries |
| Network | No multiplayer. Future: TCP socket-based lobby |
| Windows ANSI | Requires enabling virtual terminal in `cmd.exe` on older Windows (Win10+ works automatically) |

---

## 10. Grading Alignment (Class Rubric)

| Requirement | Implementation |
|---|---|
| Interactive quiz | ✅ MCQ, 4 options, hint, skip |
| Answer evaluation | ✅ correctIndex check, wrong answer reveal |
| File handling | ✅ `scores.dat` append write + line-by-line read |
| Performance tracking | ✅ per-session score, accuracy %, grade |
| Replay | ✅ main menu loop, change player, same session |
| Extra credit areas | Timer bonus, leaderboard, ANSI color, difficulty levels |
