# Quiz Master Terminal

The terminal edition of Quizline: a self-contained C++17 quiz application with
an ncurses interface. For the browser edition, project architecture, and a
comparison of both versions, see [the project README](README.md).

## Installation

Use a color terminal with at least **80 columns × 24 rows**, a C++17 compiler,
and ncurses headers/libraries. A larger terminal displays more table rows.

### Linux (Debian / Ubuntu)

```sh
sudo apt update
sudo apt install g++ libncurses-dev
```

For Fedora, install `gcc-c++` and `ncurses-devel` with `dnf` instead.

### macOS

Install Apple's command line tools (C++ compiler and SDK ncurses):

```sh
xcode-select --install
```

Apple's `g++` command invokes Clang and supports the build below. If using a
separate compiler, configure it to find the SDK's ncurses headers and library.

### Windows

Use **WSL with Ubuntu**, which supplies the same ncurses environment as Linux.
Run in an administrator PowerShell terminal:

```powershell
wsl --install -d Ubuntu
```

Complete Ubuntu setup, open its terminal, and run the Linux installation commands
above. Open the project in WSL (Windows drives are under `/mnt/c/`), then build
and run inside WSL. This creates a Linux executable; the app does not depend on
Windows console APIs or a PDCurses replacement.

## Build and run

From the directory containing `quiz_terminal.cpp`:

```sh
g++ -std=c++17 -Wall -Werror -o quiz_terminal quiz_terminal.cpp -lncurses
./quiz_terminal
```

The executable uses the current working directory for saved data. On older Linux
toolchains that require explicit thread linkage, append `-pthread` to the build
command. No nonstandard C++ libraries are used beyond ncurses.

## Keys

| Screen | Key | Action |
|---|---|---|
| Menus | Up / Down | Select an option, wrapping at either end |
| Menus | Enter | Confirm the highlighted option |
| Menus | Q / Esc | Go back, or quit from the main menu |
| Name entry | Printable ASCII, Backspace | Enter/edit up to 40 characters; `|` is rejected |
| Name entry | Enter / Esc | Accept (blank becomes `Player`) / cancel |
| Question | A / B / C / D | Answer, case insensitive |
| Question | H | Hint overlay; any key dismisses it, or it closes after 5 seconds |
| Question | S | Skip for zero points |
| Question | F1 | Confirm and use 50/50 |
| Question | F2 | Confirm and use Freeze |
| Question | F3 | Confirm and use Double Points |
| Question | Q | Abandon this run without saving |
| Results | Any key | Advance immediately to the performance breakdown |
| Performance breakdown | Any key | Return to main menu |
| Leaderboard / history | Up / Down | Scroll rows |
| Leaderboard / history | Q | Return to main menu |
| How to Play | Up / Down | Scroll instructions; any other key returns |

Each received key flushes queued keystrokes (empty timer polls leave input intact),
so type names and menu choices normally instead of pasting several keys at once. Invalid printable answer keys such as
`Z` are marked wrong for zero points, retaining the PRD behavior. Eliminated
options cannot be selected. Function keys must be passed through by the terminal;
some macOS keyboards require holding Fn.

## Features

- **Four panels:** player/score/category header, main content, live countdown,
  and status with streak and colored power-up availability badges. Borders and
  headings remain visible together. The header title occupies its upper border
  so the requested three-row panel also fits player metadata and a ghost target.
- **Menus:** Start New Quiz, View Leaderboard, My Stats and History, Change
  Player, How to Play, and Quit. Category selection includes Random.
- **Original question bank:** all 20 questions, options, answers, hints, point
  values, and time limits are copied unchanged. Each category has two Easy,
  two Medium, and one Hard question. All (Mixed) selects all five. Question order
  is shuffled per run; the original A–D option ordering is retained.
- **Countdown:** a C++ worker thread redraws a smooth bar every 100 ms. It is
  green above 50%, yellow from 20–50%, and red below 20%. Expiry flashes the main
  panel three times, awards zero, resets the streak, and advances automatically.
  A shared mutex protects all ncurses operations; timer threads stop and join
  before answer feedback or window cleanup.
- **Scoring:** Easy/Medium/Hard awards 10/20/30 base points. A correct answer adds
  `int(basePoints * 0.5 * max(0.0, 1.0 - secondsTaken / timeLimit))` bonus points.
  Seconds taken exclude Freeze time and include hints and confirmation prompts.
  Wrong answers, skips, and timeouts award zero. Answer feedback lasts 1.5 seconds.
- **Streaks:** three correct answers produce a green ON FIRE overlay for 1.2
  seconds and a fire indicator in the status panel. Five correct answers arm a
  2× award for the following question; it expires after that question. The
  supplied bank has at most five questions per session, so this multiplier arms
  on the last question of a perfect Mixed run. Its scoring logic also supports
  longer future sessions without changing the original question bank.
- **Power-ups:** one use of each per run, with a menu confirmation. 50/50 hides
  two randomly chosen wrong options. Freeze pauses the countdown for exactly ten
  seconds while answering remains possible. Double Points doubles the current
  question's base plus time bonus if correct; a wrong answer or skip consumes it.
  Streak and power-up multipliers multiply together. Reusing a power-up shows
  ALREADY USED. Time continues during hints and confirmation menus.
- **Results and grades:** score, correct/total, accuracy, date, difficulty,
  bonuses, and power-up usage. S ≥90%, A ≥75%, B ≥60%, C ≥40%, D <40%.
  Results advance after three seconds or any key.
- **Performance breakdown:** four forty-character category accuracy bars,
  including empty 0% bars for unplayed categories, plus session totals and grade.
  Green means above 80%, yellow 50–80%, and red below 50%.
- **Ghost mode:** automatically loads the player's best for the exact category
  and difficulty, displays its target, and compares the new run before bonuses.
  Beating it earns +20 points. The saved best includes awarded bonuses. Ties and
  losses preserve the old best. Different players and difficulties are isolated.
- **Leaderboard:** score descending, then accuracy descending; animated third,
  second, and first name reveals; a centered 35-column ncurses podium; top ten
  rows with current-player highlights. The third-place label uses the block's
  border so its required three-row height can also display a name. At 80×24,
  scroll the table beneath the podium to see all ten entries.
- **History:** current-player sessions in reverse save order, total sessions,
  total/average/best points, best grade, and favourite category. Personal best
  rows are highlighted. Arrow keys scroll when the content exceeds the panel.
- **Resizing and cleanup:** panels rebuild on terminal resize and the current
  screen is repainted. Resizing below 80×24 exits with an explanatory error after
  restoring terminal mode. Startup also rejects terminals without sufficient
  color support. All persistent and popup windows are deleted before `endwin()`.

## Saved data

Files are created only when needed, relative to the run directory:

| File | Format / purpose |
|---|---|
| `scores.dat` | Original `name\|score\|totalQ\|correct\|percentage\|date\|difficulty`; completed runs append, compatible with the original app |
| `ghost.dat` | `playerName\|category\|difficulty\|bestScore\|bestCorrect\|avgTimeTaken\|date`; one personal best per player/category/difficulty |
| `terminal_sessions.dat` | `playerName\|category\|date`; category metadata for each successfully saved terminal session |

The unchanged legacy score format has no category field. Favourite category
therefore uses terminal session metadata; older runs without metadata show
Unknown until new sessions are recorded. Category ties resolve alphabetically.
Ghost average time measures active answering time across every attempted question.
Malformed score/ghost records are skipped, and missing files are treated as empty.
Ghost updates use a temporary file and rename to protect the previous file if a
write fails. Save failures appear on screen. An abandoned quiz writes no records.

## Verification

Built after each of the twelve implementation steps using the command above.
Verification includes isolated C++ scoring/file-I/O checks and pseudo-terminal
playthroughs for the fifteen PRD cases, along with power-ups, ghosts, resize,
minimum dimensions, monochrome rejection, and timed expiry. The original PRD's
cosmetic timer is replaced by the required enforced countdown, and podium ranks
replace its medal presentation. The exact legacy question-bank and score I/O
source blocks are preserved verbatim.

Build and runtime checks were executed on macOS. Linux and Windows/WSL setup
instructions are included; those platforms were not executed in this workspace.
