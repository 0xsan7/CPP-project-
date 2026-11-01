# Quizline Media

Repository media shows the running application with demonstration data. It is stored locally so the README does not depend on an external image host.

## Terminal captures

The terminal screenshots and GIF are rendered from the ncurses application's actual pseudo-terminal output, including its text, borders, ANSI colors, answer feedback, and power-up state. They are not photographs of a native Terminal window or concept mockups.

The application was run at 100 columns by 30 rows. A caption strip outside the terminal identifies the current action. Sample player names and saved scores come from isolated temporary directories. The GIF contains seventeen selected stages from three completed sessions; pauses are shortened for readability, so playback is not real time.

| Asset | Content |
|---|---|
| `assets/terminal-question.png` | Question, countdown, answer choices, and power-up availability |
| `assets/terminal-results.png` | A completed quiz with an S grade and a ghost bonus |
| `assets/terminal-menu.png` | Keyboard-driven main menu |
| `assets/terminal-leaderboard.png` | Saved score table and podium |
| `assets/terminal-performance.png` | Four-category performance breakdown |
| `assets/terminal-demo.gif` | Hints, power-ups, scoring, ghost replay, history, and streak feedback |

## Browser captures

The browser images are direct screenshots of the locally running frontend. Desktop captures use a 1440 by 960 viewport; full-page capture includes content below the viewport where needed. The mobile dashboard was captured at 431 pixels wide.

A temporary SQLite database and fictional `Demo Player` account were used. No existing account data, passwords, access tokens, or local user scores are published.

| Asset | Content |
|---|---|
| `assets/web-home.jpg` | Desktop subject selection and session setup |
| `assets/web-question.jpg` | Desktop quiz question and elapsed-time sidebar |
| `assets/web-profile.jpg` | Demo account statistics and saved session |
| `assets/web-mobile.jpg` | Responsive mobile dashboard |

## Branding

`assets/quizline-banner.svg` is a repository banner using the application's existing Q/check brand mark and mint/dark palette. It is an illustration for documentation, not a screenshot. It uses self-contained SVG shapes and text without remote fonts, scripts, or external image references.

To refresh media, run the matching app with disposable demo data, capture the real screens, replace the files in `docs/assets/`, and check every README image and link. Keep personal accounts and runtime data outside version control.

[Project overview](../README.md) · [Terminal guide](../README_terminal.md) · [Frontend guide](../README_web.md)
