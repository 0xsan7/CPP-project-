# Quizline

A C++17 quiz application with two interfaces: an interactive terminal application and a browser frontend. Both versions use the same four-category question bank and scoring rules, with persistence, leaderboards, and player history.

C++ provides the question data, answer evaluation, and time-based scoring. The terminal version adds an ncurses interface; the browser version connects a React frontend to the C++ engine through a local Python API.

## Versions

| | Terminal application | Web application |
|---|---|---|
| Interface | ncurses terminal UI | Browser frontend |
| Entry point | `quiz_terminal.cpp` | `server.py` |
| Main technologies | C++17, STL, ncurses | C++17, Python, SQLite, React, TypeScript, Vite, Tailwind CSS |
| Player identity | Local player names | Registered accounts and profiles |
| Question timing | Live countdown with an enforced cutoff | Elapsed-time display and time-based scoring |
| Additional features | Power-ups, streaks, ghost challenges, animated podium | Account management, profile statistics, account-owned history |
| Storage | Local pipe-delimited files | SQLite for accounts, sessions, and web scores |

The two interfaces run independently. The web application does not require ncurses, and the terminal application does not require Node.js or Python.

## Quiz content and scoring

- Science, History, Technology, and Sports, with five questions per category.
- Twenty multiple-choice questions, each with four options and a written hint.
- Easy, Medium, Hard, and All (Mixed) difficulty selections.
- Random category selection and shuffled question order.
- Base awards of 10, 20, and 30 points for Easy, Medium, and Hard questions.
- A time bonus of up to 50% of the base award for a timely correct answer.
- Zero points for incorrect answers and skipped questions.
- Results with score, correct answers, accuracy, and grade.

| Grade | Accuracy |
|---|---|
| S | 90% and above |
| A | 75% to below 90% |
| B | 60% to below 75% |
| C | 40% to below 60% |
| D | Below 40% |

## Getting started

Clone the repository and enter the project directory:

```bash
git clone https://github.com/0xsan7/CPP-project-.git
cd CPP-project-
```

Choose either version below. All commands run from the repository root.

## Terminal application

The terminal version displays four panels for player details, questions, the countdown, and session status. It includes arrow-key menus, hints, three single-use power-ups, streak feedback, personal-best ghost challenges, an animated leaderboard podium, and category performance charts.

### Requirements

- A compiler with C++17 support, available as `g++`.
- ncurses headers and libraries.
- A color terminal at least 80 columns wide and 24 rows tall.

On macOS, install the command line developer tools if needed:

```bash
xcode-select --install
```

On Debian or Ubuntu:

```bash
sudo apt update
sudo apt install g++ libncurses-dev
```

On Windows, use WSL with Ubuntu and follow the Linux instructions. Additional platform details are in [the terminal guide](README_terminal.md).

### Build and run

```bash
g++ -std=c++17 -Wall -Werror -o quiz_terminal quiz_terminal.cpp -lncurses
./quiz_terminal
```

On older Linux toolchains that require explicit thread linkage, append `-pthread` to the build command. Apple command line tools provide a Clang-based `g++` command that supports this build.

### Controls

| Key | Action |
|---|---|
| Up / Down | Navigate menus and scroll tables |
| Enter | Confirm a selection |
| A / B / C / D | Select an answer |
| H | Reveal a hint |
| S | Skip a question |
| F1 | Use 50/50 to remove two wrong options |
| F2 | Freeze the countdown for ten seconds |
| F3 | Double the current question's points if correct |
| Q | Go back, abandon a quiz, or quit from the main menu |

Power-ups require confirmation. On some macOS keyboards, hold Fn when pressing a function key. Complete a quiz to save the session; abandoned runs are not saved.

Ghost mode activates when the same player repeats a category and difficulty. Beating the recorded best awards 20 bonus points. Three consecutive correct answers trigger the ON FIRE banner. Five arm a multiplier for the following question; the supplied question bank limits a session to five questions, so this multiplier arms on the final question of a perfect Mixed run.

See [README_terminal.md](README_terminal.md) for detailed features, timing behavior, file formats, and troubleshooting.

## Web application

The browser version provides a styled quiz interface with account registration, sign-in, editable profiles, a leaderboard, and personal session history. The frontend uses React, TypeScript, Vite, Tailwind CSS v4, and a component structure configured for shadcn/ui. Quiz interactions are implemented in `web/app.js`.

The Python server serves the frontend, manages accounts and SQLite persistence, and calls the C++ engine for the question catalog, hints, and answer scoring. Python uses its standard library; a separate package installation with pip is not required.

### Requirements

- A C++17 compiler available as `g++`.
- Python 3.10 or newer.
- Node.js 20.19 or newer in the 20.x line, or Node.js 22.12 or newer.
- npm and a modern browser.

### Install and run

```bash
npm ci
python3 server.py
```

Open [http://127.0.0.1:8765](http://127.0.0.1:8765) in a browser. Create an account or sign in, select a category and difficulty, and start a quiz.

On startup, the server compiles `quiz_app.cpp` into `build/quiz_app` and builds the frontend when its source is newer than the current output. It listens on the local computer only. Press Ctrl+C in the server terminal to stop it.

### Frontend development

Keep the Python server running. In a second terminal:

```bash
npm run dev
```

Open the local URL printed by Vite. Its development server proxies `/api` requests to the Python server on port 8765.

To type-check and build the frontend:

```bash
npm run build
```

The production frontend files are written to `dist/`. Public deployment is not configured; the included Python server is intended for local use.

## Architecture and project structure

```text
Terminal version
  quiz_terminal.cpp -> ncurses UI and C++ quiz logic -> local data files

Web version
  Browser -> React frontend -> Python API -> C++ quiz engine
                                         -> SQLite database
```

| Path | Purpose |
|---|---|
| `quiz_terminal.cpp` | Standalone terminal UI, quiz engine, timer, power-ups, and ghost mode |
| `quiz_app.cpp` | Original console application and C++ command adapter used by the web API |
| `server.py` | Local HTTP API, authentication, database persistence, and build startup |
| `web/` | Browser markup, quiz interactions, React entry point, styles, and public assets |
| `components/` | Reusable React interface components |
| `styles/` | Shared Tailwind styles and theme |
| `lib/` | Shared frontend utilities |
| `package.json` | Frontend dependencies and development scripts |
| `README_terminal.md` | Detailed terminal documentation |
| `PRD.md` | Original product requirements and acceptance checklist |

The original console interface remains available for compatibility:

```bash
g++ -std=c++17 -Wall -Werror -o quiz_app quiz_app.cpp
./quiz_app
```

## Data persistence

| File | Used by | Purpose |
|---|---|---|
| `scores.dat` | Terminal and original console | Completed quiz scores in the original pipe-delimited format |
| `ghost.dat` | Terminal | Personal best per player, category, and difficulty |
| `terminal_sessions.dat` | Terminal | Category metadata for favourite-category statistics |
| `quizline.sqlite3` | Web | Accounts, login sessions, and account-owned quiz scores |

Terminal data files are relative to the working directory. The web database defaults to the repository root; set `QUIZ_DB` to use another path. `QUIZ_PORT` changes the Python server port; update the Vite proxy if using a different port for frontend development.

The web leaderboard can display legacy entries from `scores.dat`, but those entries are not linked to registered accounts. New web sessions are saved to the signed-in account. Power-ups, ghost records, and streak state belong to the terminal version and are not synchronized with web accounts.

Local scores, databases, compiled executables, dependencies, and build output are excluded from version control. The login artwork uses remotely hosted media and requires an internet connection to display that artwork.

## Verification

The terminal application has been built with C++17, `-Wall`, and `-Werror` and checked against all fifteen acceptance cases in the original PRD. Additional checks cover power-ups, ghost comparisons, timer expiry, resizing, and saved-record validation.

For a manual check:

1. Start the terminal application and complete a Science quiz with a hint, a correct answer, and a skipped or incorrect answer.
2. Inspect the results, category breakdown, leaderboard, and player history.
3. Replay the same category and difficulty to verify the ghost target.
4. Start the web application, register a local account, complete a quiz, and inspect its leaderboard and profile history.

Run `npm run build` to validate the frontend. Terminal runtime checks have been executed on macOS; Linux and Windows/WSL instructions are provided but have not been executed in this workspace.
