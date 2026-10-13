# Quizline local frontend

Install the frontend dependencies once, then run the app from this folder:

```bash
npm install
python3 server.py
```

Open [http://127.0.0.1:8765](http://127.0.0.1:8765). The server listens on this computer only, builds the frontend, and compiles `quiz_app.cpp` with C++17, `-Wall`, and `-Werror` when needed. Requirements: Node.js 20.19+ or 22.12+, npm, Python 3, and `g++`.

The frontend now supports React, TypeScript, Tailwind CSS v4, and shadcn's project structure:

- Reusable UI components: `/components/ui`
- Shared global styles and Tailwind theme: `/styles/globals.css`
- App appearance: `/styles/theme.css` and `/web/styles.css`
- Shared utilities: `/lib/utils.ts`
- Import alias: `@/` resolves to the project root.

`components.json` tells shadcn to install components in `/components/ui`. Keeping that folder and alias consistent allows copied components and CLI-generated imports to resolve correctly. The supplied `AsciiArt` and its demo live there. The artwork is mounted with React on the login page; existing quiz interactions continue through the same API.

For frontend development, keep the Python server running and use `npm run dev` in a second terminal. Vite proxies `/api` to port 8765. `npm run build` checks TypeScript and creates `/dist`. To add a shadcn component, run `npx shadcn@latest add button`; initialization is already configured here.

For a fresh project elsewhere, create a React TypeScript Vite project with `npm create vite@latest my-app -- --template react-ts`, install `tailwindcss @tailwindcss/vite`, configure the Tailwind Vite plugin and `@/` alias, then run `npx shadcn@latest init`.

The supplied animation uses its original remote MP4 and WebP poster. It needs no data, state provider, or extra image assets. The login wrapper pauses animation for reduced-motion preferences and hidden browser tabs.

The browser uses the C++ question bank, hints, scoring, and grades. Accounts, login sessions, and account-owned scores are stored in `quizline.sqlite3`. Passwords are salted and hashed; sign-in uses an HttpOnly cookie. The terminal version still runs with:

```bash
g++ -std=c++17 -Wall -Werror -o quiz_app quiz_app.cpp
./quiz_app
```

The terminal version continues to save to `scores.dat`. Its older scores appear on the web leaderboard, but they cannot be attached to a profile because the old format has no account ID. New web scores belong to the signed-in account and appear in its history and profile.

Press Ctrl+C in the server terminal to stop local hosting. Deployment is intentionally deferred. Before public hosting, provide HTTPS, a persistent database and backups, and an account-recovery flow; set `QUIZ_HTTPS=1` so session cookies require HTTPS.
