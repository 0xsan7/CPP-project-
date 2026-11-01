#include <ncurses.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <clocale>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

struct Question {
    string text;
    vector<string> options;  // exactly 4
    int correctIndex;        // 0-based
    string hint;
    string difficulty;       // "Easy" | "Medium" | "Hard"
    int timeLimit;           // seconds
    int points;              // base award
};

struct PlayerScore {
    string name;
    int score;
    int totalQuestions;
    int correctAnswers;
    double percentage;
    string date;       // "YYYY-MM-DD HH:MM"
    string difficulty;
};

const string SCORE_FILE = "scores.dat";

// Build the four-category question bank from the reference implementation.
map<string, vector<Question>> buildQuestionBank() {
    map<string, vector<Question>> bank;

    // ── SCIENCE ──
    bank["Science"] = {
        {
            "What is the chemical symbol for water?",
            {"H2O", "CO2", "O2", "NaCl"},
            0, "It contains Hydrogen and Oxygen.", "Easy", 20, 10
        },
        {
            "Which planet is known as the Red Planet?",
            {"Venus", "Mars", "Jupiter", "Saturn"},
            1, "It has iron oxide on its surface.", "Easy", 20, 10
        },
        {
            "What is the speed of light (approx.) in km/s?",
            {"150,000", "300,000", "450,000", "600,000"},
            1, "It is roughly 3 × 10^8 m/s.", "Medium", 25, 20
        },
        {
            "DNA stands for?",
            {"Deoxyribonucleic Acid", "Dinucleic Acid", "Dynamic Nuclear Atom", "Digital Nuclear Array"},
            0, "It carries genetic instructions.", "Medium", 25, 20
        },
        {
            "What particle has a negative charge in an atom?",
            {"Proton", "Neutron", "Electron", "Positron"},
            2, "It orbits the nucleus.", "Hard", 30, 30
        }
    };

    // ── HISTORY ──
    bank["History"] = {
        {
            "In which year did World War II end?",
            {"1943", "1944", "1945", "1946"},
            2, "It ended with Japan's surrender.", "Easy", 20, 10
        },
        {
            "Who was the first President of the United States?",
            {"Abraham Lincoln", "Thomas Jefferson", "John Adams", "George Washington"},
            3, "He is on the one-dollar bill.", "Easy", 20, 10
        },
        {
            "The French Revolution began in which year?",
            {"1776", "1789", "1800", "1815"},
            1, "It started with storming a famous prison.", "Medium", 25, 20
        },
        {
            "Which empire was ruled by Julius Caesar?",
            {"Greek", "Ottoman", "Roman", "Persian"},
            2, "Its capital was Rome.", "Medium", 25, 20
        },
        {
            "The Berlin Wall fell in which year?",
            {"1985", "1987", "1989", "1991"},
            2, "It happened late in the Cold War.", "Hard", 30, 30
        }
    };

    // ── TECHNOLOGY ──
    bank["Technology"] = {
        {
            "What does CPU stand for?",
            {"Central Processing Unit", "Computer Personal Unit", "Core Power Unit", "Central Power Usage"},
            0, "It is the 'brain' of a computer.", "Easy", 20, 10
        },
        {
            "Which programming language is this project written in?",
            {"Java", "Python", "C++", "Rust"},
            2, "It was created by Bjarne Stroustrup.", "Easy", 20, 10
        },
        {
            "What does HTTP stand for?",
            {"HyperText Transfer Protocol", "High-Tech Transfer Process", "Hyper Terminal Text Program", "Host Transfer Text Protocol"},
            0, "It is the foundation of the Web.", "Medium", 25, 20
        },
        {
            "Which company developed the Android OS?",
            {"Apple", "Microsoft", "Google", "Samsung"},
            2, "It acquired Android Inc. in 2005.", "Medium", 25, 20
        },
        {
            "In binary, what is the decimal value of 1010?",
            {"8", "10", "12", "14"},
            1, "Each bit is a power of 2.", "Hard", 30, 30
        }
    };

    // ── SPORTS ──
    bank["Sports"] = {
        {
            "How many players are on a football (soccer) team?",
            {"9", "10", "11", "12"},
            2, "One of them is the goalkeeper.", "Easy", 20, 10
        },
        {
            "In which sport is the term 'love' used as a score?",
            {"Cricket", "Tennis", "Badminton", "Squash"},
            1, "It means zero in this racket sport.", "Easy", 20, 10
        },
        {
            "How many rings are on the Olympic flag?",
            {"4", "5", "6", "7"},
            1, "They represent five continents.", "Medium", 25, 20
        },
        {
            "Which country has won the most FIFA World Cups?",
            {"Germany", "Italy", "Argentina", "Brazil"},
            3, "They have won it five times.", "Medium", 25, 20
        },
        {
            "What is the maximum break in snooker?",
            {"145", "147", "150", "155"},
            1, "It requires potting every ball perfectly.", "Hard", 30, 30
        }
    };

    return bank;
}

// Append one completed player's score and report whether the write succeeded.
bool saveScore(const PlayerScore& ps) {
    ofstream file(SCORE_FILE, ios::app);
    if (!file) {
        return false;
    }
    file << ps.name << '|'
         << ps.score << '|'
         << ps.totalQuestions << '|'
         << ps.correctAnswers << '|'
         << fixed << setprecision(1) << ps.percentage << '|'
         << ps.date << '|'
         << ps.difficulty << '\n';
    file.close();
    return file.good();
}

// Load valid score records while skipping incomplete or malformed lines.
vector<PlayerScore> loadScores() {
    vector<PlayerScore> scores;
    ifstream file(SCORE_FILE);
    if (!file) {
        return scores;
    }

    string line;
    while (getline(file, line)) {
        array<string, 7> fields;
        size_t start = 0;
        size_t fieldIndex = 0;
        while (fieldIndex < fields.size() - 1) {
            const size_t separator = line.find('|', start);
            if (separator == string::npos) {
                break;
            }
            fields[fieldIndex++] = line.substr(start, separator - start);
            start = separator + 1;
        }
        if (fieldIndex != fields.size() - 1 || line.find('|', start) != string::npos) {
            continue;
        }
        fields.back() = line.substr(start);
        if (fields.front().empty() || fields[5].empty() || fields[6].empty()) {
            continue;
        }

        try {
            size_t used = 0;
            PlayerScore ps;
            ps.name = fields[0];
            ps.score = stoi(fields[1], &used);
            if (used != fields[1].size()) continue;
            ps.totalQuestions = stoi(fields[2], &used);
            if (used != fields[2].size()) continue;
            ps.correctAnswers = stoi(fields[3], &used);
            if (used != fields[3].size()) continue;
            ps.percentage = stod(fields[4], &used);
            if (used != fields[4].size() || !isfinite(ps.percentage)) continue;
            ps.date = fields[5];
            ps.difficulty = fields[6];
            if (ps.score < 0 || ps.totalQuestions <= 0 || ps.correctAnswers < 0 ||
                ps.correctAnswers > ps.totalQuestions || ps.percentage < 0.0 ||
                ps.percentage > 100.0) {
                continue;
            }
            scores.push_back(ps);
        } catch (const invalid_argument&) {
            continue;
        } catch (const out_of_range&) {
            continue;
        }
    }
    return scores;
}

struct GhostRecord {
    string category;
    string difficulty;
    int bestScore;
    int bestCorrect;
    double avgTimeTaken;
    string date;
};

struct NamedGhost { string player; GhostRecord record; };

// Read validated personal bests, tolerating missing and malformed records.
vector<NamedGhost> loadGhosts() {
    vector<NamedGhost> records;
    ifstream file("ghost.dat");
    string line;
    while (getline(file, line)) {
        vector<string> fields;
        istringstream parser(line);
        string field;
        while (getline(parser, field, '|')) fields.push_back(field);
        if (fields.size() != 7 || line.back() == '|' || fields[0].empty() ||
            fields[1].empty() || fields[2].empty() || fields[6].empty()) continue;
        try {
            size_t used = 0;
            int score = stoi(fields[3], &used);
            if (used != fields[3].size() || score < 0) continue;
            int correct = stoi(fields[4], &used);
            if (used != fields[4].size() || correct < 0) continue;
            double average = stod(fields[5], &used);
            if (used != fields[5].size() || !isfinite(average) || average < 0) continue;
            records.push_back({fields[0], {fields[1], fields[2], score, correct, average, fields[6]}});
        } catch (const exception&) { continue; }
    }
    return records;
}

// Return the best record for this player and exact category/difficulty, or an empty record.
GhostRecord loadGhost(const string& player, const string& category, const string& difficulty) {
    GhostRecord best{};
    for (const auto& item : loadGhosts()) {
        const auto& g = item.record;
        if (item.player == player && g.category == category && g.difficulty == difficulty &&
            (best.category.empty() || g.bestScore > best.bestScore)) best = g;
    }
    return best;
}

// Preserve each personal best and replace only the matching, improved record.
bool saveGhost(const string& player, const GhostRecord& record) {
    auto records = loadGhosts();
    for (auto& item : records) {
        if (item.player == player && item.record.category == record.category &&
            item.record.difficulty == record.difficulty) {
            if (record.bestScore <= item.record.bestScore) return true;
        }
    }
    records.erase(remove_if(records.begin(), records.end(), [&](const NamedGhost& item) {
        return item.player == player && item.record.category == record.category &&
               item.record.difficulty == record.difficulty;
    }), records.end());
    records.push_back({player, record});
    ofstream file("ghost.dat.tmp", ios::trunc);
    if (!file) return false;
    for (const auto& item : records) {
        const auto& g = item.record;
        file << item.player << '|' << g.category << '|' << g.difficulty << '|'
             << g.bestScore << '|' << g.bestCorrect << '|' << setprecision(10)
             << g.avgTimeTaken << '|' << g.date << '\n';
    }
    file.close();
    if (!file.good()) { remove("ghost.dat.tmp"); return false; }
    if (rename("ghost.dat.tmp", "ghost.dat") != 0) { remove("ghost.dat.tmp"); return false; }
    return true;
}

struct SessionState {
    string playerName = "Player";
    string category = "-";
    string difficulty = "All";
    int totalScore = 0;
    int correctAnswers = 0;
    int totalQuestions = 0;
    int currentStreak = 0;
    int maxStreak = 0;
    bool doubleStreakActive = false;
    bool hasFiftyFifty = true;
    bool hasFreeze = true;
    bool hasDouble = true;
    bool doubleActive = false;
    bool ghostActive = false;
    GhostRecord ghost{};
    map<string, pair<int,int>> categoryStats;
    int streakBonus = 0;
    bool doubleUsed = false;
    bool frozen = false;
    int ghostComparison = 0;
    int ghostBonus = 0;
    double totalTimeTaken = 0.0;
    string persistenceError;
    string notice;
};

// Limit text to interior cells so long names never overwrite a border.
void put(WINDOW* win, int row, int col, const string& text, int color = 6, int attributes = 0) {
    if (!win || row < 0 || row >= getmaxy(win) || col < 0 || col >= getmaxx(win) - 1) return;
    wattrset(win, COLOR_PAIR(color) | attributes);
    mvwaddnstr(win, row, col, text.c_str(), getmaxx(win) - col - 1);
    wattrset(win, COLOR_PAIR(6));
}

// Center a bounded line inside a window.
void centered(WINDOW* win, int row, const string& text, int color = 4, int attributes = 0) {
    const string clipped = text.substr(0, max(0, getmaxx(win) - 4));
    put(win, row, max(1, (getmaxx(win) - static_cast<int>(clipped.size())) / 2), clipped, color, attributes);
}

// Wrap words without dropping long tokens.
vector<string> wrapText(const string& text, int width) {
    vector<string> lines;
    istringstream stream(text);
    string word, line;
    while (stream >> word) {
        if (!line.empty() && line.size() + word.size() + 1 > static_cast<size_t>(width)) {
            lines.push_back(line); line.clear();
        }
        while (word.size() > static_cast<size_t>(width)) {
            if (!line.empty()) { lines.push_back(line); line.clear(); }
            lines.push_back(word.substr(0, width)); word.erase(0, width);
        }
        if (!word.empty()) { if (!line.empty()) line += ' '; line += word; }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// Own all persistent windows and serialize every ncurses operation with one mutex.
class TerminalUI {
public:
    mutex screenMutex;
    WINDOW* header = nullptr;
    WINDOW* content = nullptr;
    WINDOW* timer = nullptr;
    WINDOW* status = nullptr;
    SessionState state;
    function<bool()> menuExpired;

    // Release all windows before main calls endwin, including during exception unwinding.
    ~TerminalUI() { lock_guard<mutex> lock(screenMutex); destroy(); }

    // Destroy partially or fully constructed layouts; caller holds screenMutex.
    void destroy() {
        for (WINDOW** win : {&header, &content, &timer, &status}) {
            if (*win) { delwin(*win); *win = nullptr; }
        }
    }

    // Recreate the four panels after initialization or a terminal resize.
    void rebuild() {
        destroy();
        if (COLS < 80 || LINES < 24) throw runtime_error("Terminal must be at least 80 columns by 24 rows.");
        erase(); refresh();
        header = newwin(3, COLS, 0, 0);
        content = newwin(LINES - 9, COLS, 3, 0);
        timer = newwin(3, COLS, LINES - 6, 0);
        status = newwin(3, COLS, LINES - 3, 0);
        if (!header || !content || !timer || !status) throw runtime_error("Unable to allocate terminal panels.");
        keypad(content, TRUE); wtimeout(content, 20);
        chrome();
        frame("QUIZ MASTER"); wrefresh(content);
        werase(timer); box(timer, 0, 0); centered(timer, 1, "Timer ready", 1); wrefresh(timer);
    }

    // Clear the main panel and label its upper border.
    void frame(const string& title) {
        werase(content); wattrset(content, COLOR_PAIR(4)); box(content, 0, 0);
        centered(content, 0, " " + title + " ", 4, A_BOLD);
    }

    // Redraw player, score, category, ghost, and all power-up availability badges.
    void chrome() {
        werase(header); wattrset(header, COLOR_PAIR(4)); box(header, 0, 0);
        centered(header, 0, " QUIZ MASTER TERMINAL ", 4, A_BOLD);
        const int third = (COLS - 4) / 3;
        put(header, 1, 2, ("Player: " + state.playerName).substr(0, third));
        centered(header, 1, "Score: " + to_string(state.totalScore) + " pts", 1);
        string category = "Category: " + state.category;
        put(header, 1, COLS - min(third, static_cast<int>(category.size())) - 2, category.substr(0, third), 4);
        if (state.ghostActive) centered(header, 2, " Ghost target: " + to_string(state.ghost.bestScore) + " pts ", 5);
        wrefresh(header);
        werase(status); wattrset(status, COLOR_PAIR(4)); box(status, 0, 0);
        put(status, 0, 2, " Streak: " + to_string(state.currentStreak) + (state.currentStreak >= 3 ? " 🔥 " : " "), 3);
        put(status, 0, COLS - 23, " H: hint | S: skip ", 3);
        int col = 2;
        const array<pair<string, bool>, 3> badges = {{{"F1:50/50", state.hasFiftyFifty},
            {"F2:Freeze", state.hasFreeze}, {"F3:Double", state.hasDouble}}};
        for (const auto& badge : badges) {
            string label = badge.first + (badge.second ? " [OK]" : " [USED]");
            put(status, 1, col, label, badge.second ? 1 : 2);
            col += static_cast<int>(label.size());
            if (&badge != &badges.back()) { put(status, 1, col, " | "); col += 3; }
        }
        string message = state.frozen ? "TIMER FROZEN for 10s" : state.notice;
        if (state.doubleActive) message += " DOUBLE POINTS ACTIVE";
        if (state.doubleStreakActive) message += " STREAK x2 ACTIVE";
        if (!message.empty()) centered(status, 2, " " + message + " ", 3);
        wrefresh(status);
    }

    // Poll under the screen lock; flush queued input after a key, never after an empty timeout.
    int key() {
        lock_guard<mutex> lock(screenMutex);
        const int ch = wgetch(content);
        // ERR is not an input event: flushing then races with the next arriving key.
        if (ch != ERR) flushinp();
        if (ch == KEY_RESIZE) rebuild();
        return ch;
    }
};

// Share one UI instance with the mandated free renderMenu entry point.
TerminalUI& ui() { static TerminalUI instance; return instance; }

// Wait in small intervals so input polling and timer painting can coexist.
void idle() { this_thread::sleep_for(chrono::milliseconds(20)); }

// Navigate any application menu with wrapping arrows, Enter, and Q.
int renderMenu(WINDOW* win, string title, vector<string> options) {
    if (options.empty()) return -1;
    int selected = 0;
    while (true) {
        if (ui().menuExpired && ui().menuExpired()) return -1;
        {
            lock_guard<mutex> lock(ui().screenMutex);
            win = ui().content;
            ui().frame(title);
            const int capacity = getmaxy(win) - 5;
            const int first = max(0, selected - capacity + 1);
            for (int i = first; i < min(static_cast<int>(options.size()), first + capacity); ++i) {
                const string line = to_string(i + 1) + ". " + options[i];
                put(win, 2 + i - first, 4, line, i == selected ? 7 : 6, i == selected ? A_BOLD : 0);
            }
            centered(win, getmaxy(win) - 2, "Up/Down: navigate   Enter: confirm   Q: back", 3);
            wrefresh(win);
        }
        const int ch = ui().key();
        if (ch == 'q' || ch == 'Q' || ch == 27) return -1;
        if (ch == KEY_UP) selected = (selected + static_cast<int>(options.size()) - 1) % options.size();
        if (ch == KEY_DOWN) selected = (selected + 1) % options.size();
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return selected;
        idle();
    }
}

// Keep the six main actions in the requested order.
vector<string> mainOptions() {
    return {"Start New Quiz", "View Leaderboard", "My Stats and History", "Change Player", "How to Play", "Quit"};
}

// Paint a wrapped question and its options, including elimination and answer feedback.
void drawQuestion(const Question& q, int number, int total, const array<bool,4>& eliminated,
                  int selected = -1, bool reveal = false, const string& feedback = "") {
    lock_guard<mutex> lock(ui().screenMutex);
    ui().frame("Q" + to_string(number) + " of " + to_string(total));
    WINDOW* win = ui().content;
    const string badge = "[" + q.difficulty + " | " + to_string(q.points) + " pts]";
    put(win, 1, getmaxx(win) - static_cast<int>(badge.size()) - 3, badge, 3);
    int row = 3;
    for (const auto& line : wrapText(q.text, getmaxx(win) - 8)) centered(win, row++, line, 6, A_BOLD);
    ++row;
    for (int i = 0; i < 4; ++i) {
        const int color = reveal && i == q.correctIndex ? 1 : (reveal && i == selected ? 2 : 6);
        put(win, row++, 4, eliminated[i] && !reveal ? "   --  (eliminated)  " :
            string(1, static_cast<char>('A' + i)) + ". " + q.options[i], color,
            eliminated[i] && !reveal ? A_DIM : (reveal && (i == selected || i == q.correctIndex) ? A_BOLD : 0));
    }
    centered(win, getmaxy(win) - 2, feedback.empty() ? "A/B/C/D: answer   H: hint   S: skip   Q: abandon" : feedback,
             feedback.empty() ? 3 : (selected == q.correctIndex ? 1 : 2));
    wrefresh(win);
}

// Show a temporary overlay; delete it before resize handling can rebuild the panels.
void popup(const string& title, const string& text, int color, int milliseconds,
           bool dismissible = true, const function<bool()>& expired = {}) {
    const auto until = chrono::steady_clock::now() + chrono::milliseconds(milliseconds);
    bool done = false;
    while (!done && chrono::steady_clock::now() < until && (!expired || !expired())) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            int width = min(COLS - 6, max(34, static_cast<int>(text.size()) + 6));
            auto lines = wrapText(text, width - 6);
            int height = static_cast<int>(lines.size()) + 4;
            WINDOW* overlay = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
            if (!overlay) throw runtime_error("Unable to allocate popup.");
            unique_ptr<WINDOW, decltype(&delwin)> owner(overlay, &delwin);
            wbkgd(overlay, COLOR_PAIR(color)); box(overlay, 0, 0);
            centered(overlay, 1, title, color, A_BOLD);
            int row = 2;
            for (const auto& line : lines) centered(overlay, row++, line, color);
            wrefresh(overlay);
        }
        const int ch = ui().key();
        done = dismissible && ch != ERR && ch != KEY_RESIZE;
        idle();
    }
    lock_guard<mutex> lock(ui().screenMutex);
    touchwin(ui().content); wrefresh(ui().content);
}

struct QuestionAnswer { int selected; double secondsTaken; bool expired; bool abandoned; };

// Keep the live countdown and its window updates synchronized with foreground input.
class Countdown {
    double total;
    double remaining;
    chrono::steady_clock::time_point lastTick;
    thread worker;
    chrono::steady_clock::time_point freezeUntil{};
public:
    atomic<bool> stopFlag{false};
    atomic<bool> timeExpired{false};
    atomic<int> remainingSeconds{0};
    atomic<double> remainingFraction{1.0};

    // Reset all state and start one timer worker per question.
    explicit Countdown(int seconds) : total(seconds), remaining(seconds), lastTick(chrono::steady_clock::now()) {
        remainingSeconds.store(seconds);
        { lock_guard<mutex> lock(ui().screenMutex); paint(); }
        worker = thread([this] {
            while (!stopFlag.load()) {
                this_thread::sleep_for(chrono::milliseconds(100));
                if (stopFlag.load()) break;
                lock_guard<mutex> lock(ui().screenMutex);
                advance(); paint();
                if (remaining <= 0) { timeExpired.store(true); stopFlag.store(true); }
            }
        });
    }

    // Join before a window can be destroyed or a result can be drawn.
    ~Countdown() { stop(); }

    // Advance using elapsed steady time rather than accumulating scheduler drift.
    void advance(chrono::steady_clock::time_point now = chrono::steady_clock::now()) {
        now = max(now, lastTick);
        const double elapsed = chrono::duration<double>(now - lastTick).count();
        const double paused = freezeUntil > lastTick ? chrono::duration<double>(min(now, freezeUntil) - lastTick).count() : 0.0;
        remaining = max(0.0, remaining - max(0.0, elapsed - paused));
        lastTick = now;
        remainingSeconds.store(static_cast<int>(ceil(remaining)));
        remainingFraction.store(remaining / total);
        if (remaining <= 0) timeExpired.store(true);
    }

    // Draw ACS blocks and a color-coded label; caller holds the UI mutex.
    void paint() {
        WINDOW* win = ui().timer;
        werase(win); wattrset(win, COLOR_PAIR(4)); box(win, 0, 0);
        if (chrono::steady_clock::now() < freezeUntil) {
            centered(win, 1, "FROZEN - timer paused for 10 seconds", 3, A_BOLD);
            wrefresh(win); return;
        }
        const double fraction = remainingFraction.load();
        const int color = fraction > .5 ? 1 : fraction >= .2 ? 3 : 9;
        const int width = getmaxx(win) - 20;
        const int filled = static_cast<int>(fraction * width);
        put(win, 1, 2, "Time", color);
        wattrset(win, COLOR_PAIR(color));
        for (int i = 0; i < width; ++i) mvwaddch(win, 1, 7 + i, i < filled ? ACS_BLOCK : ACS_HLINE);
        put(win, 1, getmaxx(win) - 11, to_string(remainingSeconds.load()) + "s left", color);
        wrefresh(win);
    }

    // Pause for exactly ten seconds, retaining the active-time balance.
    void freeze() {
        lock_guard<mutex> lock(ui().screenMutex);
        advance();
        freezeUntil = chrono::steady_clock::now() + chrono::seconds(10);
        paint();
    }

    // Inspect freeze state safely while the worker updates its countdown.
    bool frozen() {
        lock_guard<mutex> lock(ui().screenMutex);
        return chrono::steady_clock::now() < freezeUntil;
    }

    // Stop promptly and return the precise active time used for scoring.
    double stop() {
        const bool alreadyStopped = stopFlag.exchange(true);
        double elapsed;
        {
            lock_guard<mutex> lock(ui().screenMutex);
            if (!alreadyStopped) advance();
            elapsed = total - remaining;
        }
        if (worker.joinable()) worker.join();
        return elapsed;
    }
};

// Flash the whole main panel three times on a hard timer cutoff.
void timeoutFlash() {
    for (int i = 0; i < 3; ++i) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            wbkgd(ui().content, COLOR_PAIR(9)); werase(ui().content);
            box(ui().content, 0, 0); centered(ui().content, getmaxy(ui().content) / 2, "TIME UP", 9, A_BOLD);
            wrefresh(ui().content);
        }
        this_thread::sleep_for(chrono::milliseconds(100));
        {
            lock_guard<mutex> lock(ui().screenMutex);
            wbkgd(ui().content, COLOR_PAIR(6)); werase(ui().content); wrefresh(ui().content);
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    { lock_guard<mutex> lock(ui().screenMutex); centered(ui().content, getmaxy(ui().content) / 2, "TIME UP", 2, A_BOLD); wrefresh(ui().content); }
}

// Collect answers while the countdown continues, including while a hint is visible.
QuestionAnswer askQuestion(const Question& q, int number, int total) {
    array<bool,4> eliminated{};
    drawQuestion(q, number, total, eliminated);
    Countdown timer(q.timeLimit);
    auto noticeUntil = chrono::steady_clock::time_point{};
    auto pollTimer = [&] {
        ui().state.frozen = timer.frozen();
        if (chrono::steady_clock::now() > noticeUntil) ui().state.notice.clear();
        { lock_guard<mutex> lock(ui().screenMutex); ui().chrome(); }
        return timer.timeExpired.load();
    };
    while (true) {
        if (pollTimer()) {
            double elapsed = timer.stop();
            timeoutFlash();
            this_thread::sleep_for(chrono::milliseconds(1500));
            return {-1, elapsed, true, false};
        }
        const int ch = ui().key();
        if (ch == KEY_RESIZE) drawQuestion(q, number, total, eliminated);
        if (ch == 'h' || ch == 'H') {
            popup("HINT", q.hint, 4, 5000, true, pollTimer);
            drawQuestion(q, number, total, eliminated);
        }
        if (ch == KEY_F(1) || ch == KEY_F(2) || ch == KEY_F(3)) {
            bool& available = ch == KEY_F(1) ? ui().state.hasFiftyFifty : ch == KEY_F(2) ? ui().state.hasFreeze : ui().state.hasDouble;
            if (!available) {
                ui().state.notice = "ALREADY USED";
                noticeUntil = chrono::steady_clock::now() + chrono::milliseconds(1200);
            } else {
                ui().menuExpired = pollTimer;
                int choice = -1;
                try { choice = renderMenu(ui().content, "USE POWER-UP? (timer continues)", {"Use power-up", "Cancel"}); }
                catch (...) { ui().menuExpired = {}; throw; }
                ui().menuExpired = {};
                if (choice == 0 && !timer.timeExpired.load()) {
                    available = false;
                    if (ch == KEY_F(1)) {
                        vector<int> wrong;
                        for (int i = 0; i < 4; ++i) if (i != q.correctIndex) wrong.push_back(i);
                        static mt19937 generator(random_device{}());
                        shuffle(wrong.begin(), wrong.end(), generator);
                        eliminated[wrong[0]] = eliminated[wrong[1]] = true;
                    } else if (ch == KEY_F(2)) {
                        timer.freeze(); ui().state.frozen = true;
                    } else {
                        ui().state.doubleActive = true; ui().state.doubleUsed = true;
                    }
                }
            }
            drawQuestion(q, number, total, eliminated);
        }
        if (ch == 'q' || ch == 'Q') return {-1, timer.stop(), false, true};
        int answer = ch >= 'a' && ch <= 'd' ? ch - 'a' : ch >= 'A' && ch <= 'D' ? ch - 'A' : -1;
        const bool skip = ch == 's' || ch == 'S';
        const bool invalid = ch >= 32 && ch <= 126 && answer < 0 && !skip &&
            ch != 'h' && ch != 'H' && ch != 'q' && ch != 'Q';
        if ((answer >= 0 && !eliminated[answer]) || skip || invalid) {
            const double elapsed = timer.stop();
            if (timer.timeExpired.load()) {
                timeoutFlash(); this_thread::sleep_for(chrono::milliseconds(1500));
                return {-1, elapsed, true, false};
            }
            return {invalid ? -2 : answer, elapsed, false, false};
        }
        if (ch != ERR && ch != KEY_RESIZE && ch != 'h' && ch != 'H' && ch != KEY_F(1) && ch != KEY_F(2) && ch != KEY_F(3)) {
            ui().state.notice = answer >= 0 ? "Option eliminated - choose a remaining answer" : "Invalid key - use A/B/C/D, H or S";
            noticeUntil = chrono::steady_clock::now() + chrono::milliseconds(1200);
        }
        idle();
    }
}

// Compute the base award and truncated linear time bonus.
int calculatePoints(const Question& q, double secondsTaken) {
    const double ratio = q.timeLimit > 0 ? max(0.0, 1.0 - secondsTaken / q.timeLimit) : 0.0;
    return q.points + static_cast<int>(q.points * .5 * ratio);
}

// Apply one answer to the session, consuming a pending streak multiplier on the next question.
int scoreAnswer(SessionState& state, const Question& q, const QuestionAnswer& answer) {
    state.totalTimeTaken += answer.secondsTaken;
    const bool correct = !answer.expired && answer.selected == q.correctIndex;
    int points = correct ? calculatePoints(q, answer.secondsTaken) : 0;
    const bool streakMultiplier = state.doubleStreakActive;
    state.doubleStreakActive = false;
    if (correct) {
        if (streakMultiplier) {
            state.streakBonus += points * (state.doubleActive ? 2 : 1);
            points *= 2; state.notice = "STREAK BONUS APPLIED";
        }
        ++state.correctAnswers;
        ++state.currentStreak;
        state.maxStreak = max(state.maxStreak, state.currentStreak);
        if (state.currentStreak == 5) state.doubleStreakActive = true;
    } else state.currentStreak = 0;
    if (correct && state.doubleActive) points *= 2;
    state.doubleActive = false;
    state.frozen = false;
    state.totalScore += points;
    auto& stats = state.categoryStats[state.category];
    ++stats.second;
    if (correct) ++stats.first;
    return points;
}

// Translate accuracy into the specified grade boundaries.
char calculateGrade(double percentage) {
    if (percentage >= 90.0) return 'S';
    if (percentage >= 75.0) return 'A';
    if (percentage >= 60.0) return 'B';
    if (percentage >= 40.0) return 'C';
    return 'D';
}

// Give each grade a readable label.
string gradeLabel(double percentage) {
    const char grade = calculateGrade(percentage);
    const string label = grade == 'S' ? "Outstanding" : grade == 'A' ? "Excellent" :
        grade == 'B' ? "Good" : grade == 'C' ? "Keep practicing" : "Try again";
    return string(1, grade) + " - " + label;
}

// Format one percentage for all summary screens.
string decimal(double value) {
    ostringstream text; text << fixed << setprecision(1) << value; return text.str();
}

// Format the local date/time consistently with the existing score format.
string currentDate() {
    time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (!local) return "0000-00-00 00:00";
    char buffer[17] = {};
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);
    return buffer;
}

// Show the completed session and allow a key to skip the three-second wait.
void showResults(const string& date) {
    const auto& state = ui().state;
    const double accuracy = state.totalQuestions ? 100.0 * state.correctAnswers / state.totalQuestions : 0.0;
    const auto until = chrono::steady_clock::now() + chrono::seconds(3);
    while (chrono::steady_clock::now() < until) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            ui().frame("QUIZ COMPLETE");
            WINDOW* win = ui().content;
            put(win, 1, 4, "Player:      " + state.playerName);
            put(win, 2, 4, "Category:    " + state.category);
            put(win, 3, 4, "Difficulty:  " + state.difficulty);
            put(win, 4, 4, "Score:       " + to_string(state.totalScore) + " pts", 1, A_BOLD);
            put(win, 5, 4, "Correct:     " + to_string(state.correctAnswers) + " / " + to_string(state.totalQuestions));
            put(win, 6, 4, "Accuracy:    " + decimal(accuracy) + "%");
            put(win, 7, 4, "Grade:       " + gradeLabel(accuracy), accuracy >= 75 ? 1 : accuracy >= 40 ? 3 : 2);
            put(win, 8, 4, "Time:        " + date);
            if (state.ghostActive) {
                string comparison = state.ghostComparison > 0 ? "YOU BEAT YOUR GHOST" :
                    state.ghostComparison == 0 ? "YOU MATCHED YOUR GHOST" : "GHOST WINS THIS TIME";
                put(win, 9, 4, comparison + " | Ghost " + to_string(state.ghost.bestScore) + " vs You " +
                    to_string(state.totalScore - state.ghostBonus), state.ghostComparison < 0 ? 2 : 1);
                if (state.ghostBonus) put(win, 10, 4, "GHOST BONUS +20 pts", 1);
            }
            if (!state.persistenceError.empty()) put(win, 10, 4, state.persistenceError, 2);
            if (state.streakBonus) put(win, 11, 4, "STREAK BONUS APPLIED +" + to_string(state.streakBonus) + " pts", 3);
            if (state.doubleUsed) put(win, 12, 4, "DOUBLE POINTS USED this session", 3);
            centered(win, getmaxy(win) - 2, "Performance breakdown in 3s - press any key", 3);
            wrefresh(win);
        }
        int ch = ui().key();
        if (ch != ERR && ch != KEY_RESIZE) return;
        idle();
    }
}

// Show four fixed forty-cell accuracy bars and complete session totals.
void showCategoryStats() {
    const auto& state = ui().state;
    while (true) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            ui().frame("YOUR PERFORMANCE BREAKDOWN");
            WINDOW* win = ui().content;
            int row = 2;
            int answered = 0;
            for (const string& category : {"Science", "History", "Technology", "Sports"}) {
                const auto found = state.categoryStats.find(category);
                const pair<int,int> stats = found == state.categoryStats.end() ? make_pair(0, 0) : found->second;
                answered += stats.second;
                double accuracy = stats.second ? 100.0 * stats.first / stats.second : 0.0;
                int filled = static_cast<int>(accuracy * 40 / 100);
                string bar(filled, '=');
                if (filled > 0 && filled < 40) bar.back() = '>';
                bar.resize(40, ' ');
                ostringstream line;
                line << left << setw(12) << category << '[' << bar << "] " << right << setw(3)
                     << static_cast<int>(accuracy) << "% (" << stats.first << '/' << stats.second << " correct)";
                put(win, row++, 2, line.str(), accuracy > 80 ? 1 : accuracy >= 50 ? 3 : 2);
            }
            double accuracy = answered ? 100.0 * state.correctAnswers / answered : 0.0;
            put(win, 8, 4, "Total questions answered this session: " + to_string(answered));
            put(win, 9, 4, "Total correct this session: " + to_string(state.correctAnswers));
            put(win, 10, 4, "Overall accuracy this session: " + decimal(accuracy) + "%");
            put(win, 11, 4, "Points earned: " + to_string(state.totalScore) + " pts   Grade: " + gradeLabel(accuracy), 3);
            centered(win, getmaxy(win) - 2, "Press any key to return to main menu", 3); wrefresh(win);
        }
        int ch = ui().key();
        if (ch != ERR && ch != KEY_RESIZE) return;
        idle();
    }
}

bool saveSessionCategory(const string& player, const string& category, const string& date);

// Filter and shuffle a session, apply bonuses, and persist completed runs only.
void runQuiz(const string& category, const string& difficulty) {
    const auto bank = buildQuestionBank();
    vector<Question> questions;
    for (const auto& q : bank.at(category)) if (difficulty == "All" || q.difficulty == difficulty) questions.push_back(q);
    const string player = ui().state.playerName;
    ui().state = SessionState{};
    ui().state.playerName = player;
    ui().state.category = category; ui().state.difficulty = difficulty;
    mt19937 generator(random_device{}());
    shuffle(questions.begin(), questions.end(), generator);
    ui().state.totalQuestions = static_cast<int>(questions.size());
    ui().state.ghost = loadGhost(player, category, difficulty);
    ui().state.ghostActive = !ui().state.ghost.category.empty();
    { lock_guard<mutex> lock(ui().screenMutex); ui().chrome(); }
    for (int i = 0; i < static_cast<int>(questions.size()); ++i) {
        auto answer = askQuestion(questions[i], i + 1, questions.size());
        if (answer.abandoned) return;
        const int points = scoreAnswer(ui().state, questions[i], answer);
        { lock_guard<mutex> lock(ui().screenMutex); ui().chrome(); }
        if (!answer.expired) {
            const bool correct = answer.selected == questions[i].correctIndex;
            string feedback = correct ? "Correct! +" + to_string(points) + " pts" : answer.selected == -1 ? "Skipped. +0 pts" :
                string(answer.selected == -2 ? "Invalid - marked wrong. Correct answer: " : "Wrong! Correct answer: ") + string(1, 'A' + questions[i].correctIndex) + ". " + questions[i].options[questions[i].correctIndex];
            drawQuestion(questions[i], i + 1, questions.size(), {}, answer.selected, true, feedback);
            this_thread::sleep_for(chrono::milliseconds(1500));
        }
        if (ui().state.currentStreak == 3) popup("  *** ON FIRE ***  ", " 3 CORRECT IN A ROW ", 8, 1200, false);
    }
    auto& state = ui().state;
    if (state.ghostActive) {
        state.ghostComparison = state.totalScore > state.ghost.bestScore ? 1 : state.totalScore == state.ghost.bestScore ? 0 : -1;
        if (state.ghostComparison > 0) { state.ghostBonus = 20; state.totalScore += 20; }
    }
    { lock_guard<mutex> lock(ui().screenMutex); ui().chrome(); }
    const string date = currentDate();
    const double percentage = 100.0 * state.correctAnswers / state.totalQuestions;
    if (!saveScore({player, state.totalScore, state.totalQuestions, state.correctAnswers, percentage, date, difficulty}))
        state.persistenceError = "Could not save scores.dat";
    else if (!saveSessionCategory(player, category, date))
        state.persistenceError = "Could not save category history";
    showResults(date);
    if (!saveGhost(player, {category, difficulty, state.totalScore, state.correctAnswers,
                           state.totalTimeTaken / state.totalQuestions, date})) {
        state.persistenceError = "Could not save ghost.dat";
        popup("SAVE FAILED", state.persistenceError, 2, 2000);
    }
    showCategoryStats();
}

// Pad and clip one table cell to keep every column within the panel.
string cell(const string& value, int width) {
    string result = value.substr(0, max(0, width - 1));
    result.resize(width, ' ');
    return result;
}

// Format a score row with an adaptable name column and stable numeric/date columns.
string scoreRow(const PlayerScore& score, int rank, bool includeName) {
    ostringstream accuracy; accuracy << fixed << setprecision(1) << score.percentage << '%';
    string row = cell(to_string(rank), 5);
    if (includeName) row += cell(score.name, max(12, COLS - 62));
    row += cell(to_string(score.score), 8) + cell(to_string(score.correctAnswers) + "/" + to_string(score.totalQuestions), 9)
        + cell(accuracy.str(), 9) + cell(score.difficulty, 11) + score.date.substr(0, 16);
    return row;
}

// Draw a fixed-size podium block, placing the short third-place label on its border.
void podiumBlock(WINDOW* win, int y, int x, int height, const string& rank, const string& name, int color) {
    wattrset(win, COLOR_PAIR(color));
    for (int row = y; row < y + height; ++row) mvwhline(win, row, x, ' ', 11);
    mvwhline(win, y, x + 1, ACS_HLINE, 9); mvwhline(win, y + height - 1, x + 1, ACS_HLINE, 9);
    mvwvline(win, y + 1, x, ACS_VLINE, height - 2); mvwvline(win, y + 1, x + 10, ACS_VLINE, height - 2);
    mvwaddch(win, y, x, ACS_ULCORNER); mvwaddch(win, y, x + 10, ACS_URCORNER);
    mvwaddch(win, y + height - 1, x, ACS_LLCORNER); mvwaddch(win, y + height - 1, x + 10, ACS_LRCORNER);
    put(win, height == 3 ? y : y + 1, x + 3, rank, color, A_BOLD);
    put(win, height == 3 ? y + 1 : y + 2, x + 1, name.substr(0, 9), color);
}

// Draw the podium before the scrollable top-ten table.
void drawLeaderboard(const vector<PlayerScore>& scores, int offset, const array<int,3>& revealed) {
    lock_guard<mutex> lock(ui().screenMutex);
    ui().frame("LEADERBOARD");
    WINDOW* win = ui().content;
    if (scores.empty()) {
        centered(win, 4, "No scores yet.", 3);
        centered(win, getmaxy(win) - 2, "Press any key to return", 3); wrefresh(win); return;
    }
    int x = (getmaxx(win) - 35) / 2;
    const array<int,3> positions = {x + 12, x, x + 24};
    const array<int,3> heights = {6, 4, 3};
    const array<int,3> colors = {10, 3, 5};
    const array<string,3> ranks = {"1ST", "2ND", "3RD"};
    for (int i = 0; i < 3; ++i) {
        string name = i < static_cast<int>(scores.size()) ? scores[i].name.substr(0, revealed[i]) : "-";
        podiumBlock(win, 8 - heights[i], positions[i], heights[i], ranks[i], name, colors[i]);
    }
    put(win, 9, 2, cell("Rank", 5) + cell("Name", max(12, COLS - 62)) + cell("Score", 8) + cell("Correct", 9) +
        cell("Accuracy", 9) + cell("Difficulty", 11) + "Date", 4, A_BOLD);
    const int count = min(10, static_cast<int>(scores.size()));
    const int capacity = getmaxy(win) - 12;
    for (int i = offset; i < min(count, offset + capacity); ++i)
        put(win, 10 + i - offset, 2, scoreRow(scores[i], i + 1, true), scores[i].name == ui().state.playerName ? 3 : 6);
    centered(win, getmaxy(win) - 2, string(offset > 0 ? "^ " : "") + "Up/Down: scroll top 10   Q: back" +
        (offset + capacity < count ? " v" : ""), 3);
    wrefresh(win);
}

// Reveal third, second, then first names at thirty milliseconds per letter.
void showLeaderboard() {
    auto scores = loadScores();
    stable_sort(scores.begin(), scores.end(), [](const PlayerScore& a, const PlayerScore& b) {
        return a.score != b.score ? a.score > b.score : a.percentage > b.percentage;
    });
    array<int,3> revealed{};
    drawLeaderboard(scores, 0, revealed);
    for (int rank = min(2, static_cast<int>(scores.size()) - 1); rank >= 0; --rank) {
        for (int length = 1; length <= min(9, static_cast<int>(scores[rank].name.size())); ++length) {
            revealed[rank] = length;
            drawLeaderboard(scores, 0, revealed);
            const auto until = chrono::steady_clock::now() + chrono::milliseconds(30);
            int ch = ui().key();
            if (ch == 'q' || ch == 'Q') return;
            if (ch == KEY_RESIZE) drawLeaderboard(scores, 0, revealed);
            this_thread::sleep_until(until);
        }
    }
    int offset = 0;
    while (true) {
        drawLeaderboard(scores, offset, revealed);
        int ch = ui().key();
        if (ch == 'q' || ch == 'Q' || (scores.empty() && ch != ERR && ch != KEY_RESIZE)) return;
        if (ch == KEY_UP) offset = max(0, offset - 1);
        if (ch == KEY_DOWN) {
            lock_guard<mutex> lock(ui().screenMutex);
            offset = min(max(0, min(10, static_cast<int>(scores.size())) - (getmaxy(ui().content) - 12)), offset + 1);
        }
        { lock_guard<mutex> lock(ui().screenMutex); offset = min(offset, max(0, min(10, static_cast<int>(scores.size())) - (getmaxy(ui().content) - 12))); }
        idle();
    }
}

// Preserve per-session categories separately because the legacy scores.dat schema has no category field.
bool saveSessionCategory(const string& player, const string& category, const string& date) {
    ofstream file("terminal_sessions.dat", ios::app);
    if (!file) return false;
    file << player << '|' << category << '|' << date << '\n';
    file.close(); return file.good();
}

// Find the most played category among terminal sessions with available category metadata.
string favouriteCategory(const string& player) {
    map<string,int> counts;
    ifstream file("terminal_sessions.dat");
    string line;
    while (getline(file, line)) {
        const size_t first = line.find('|');
        const size_t second = first == string::npos ? string::npos : line.find('|', first + 1);
        if (first == string::npos || second == string::npos || second + 1 == line.size() ||
            line.find('|', second + 1) != string::npos || line.substr(0, first) != player) continue;
        const string category = line.substr(first + 1, second - first - 1);
        if (category == "Science" || category == "History" || category == "Technology" || category == "Sports") ++counts[category];
    }
    string best = "Unknown (no category history)";
    int most = 0;
    for (const auto& item : counts) if (item.second > most) { best = item.first; most = item.second; }
    return best;
}

// Show long reference screens with arrow scrolling and return on any other key.
void showDocument(const string& title, const vector<pair<string,int>>& source) {
    int offset = 0;
    while (true) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            ui().frame(title);
            WINDOW* win = ui().content;
            vector<pair<string,int>> lines;
            for (const auto& item : source) {
                if (item.first.empty()) lines.push_back(item);
                else for (const auto& line : wrapText(item.first, COLS - 8)) lines.push_back({line, item.second});
            }
            const int capacity = getmaxy(win) - 4;
            offset = min(offset, max(0, static_cast<int>(lines.size()) - capacity));
            for (int i = offset; i < min(static_cast<int>(lines.size()), offset + capacity); ++i)
                put(win, 2 + i - offset, 4, lines[i].first, lines[i].second, lines[i].second == 4 ? A_BOLD : 0);
            centered(win, getmaxy(win) - 2, string(offset ? "^ " : "") + "Up/Down: scroll   Any other key: back" +
                (offset + capacity < static_cast<int>(lines.size()) ? " v" : ""), 3);
            wrefresh(win);
        }
        int ch = ui().key();
        if (ch == KEY_UP) offset = max(0, offset - 1);
        else if (ch == KEY_DOWN) ++offset;
        else if (ch != ERR && ch != KEY_RESIZE) return;
        idle();
    }
}

// Explain every control, scoring rule, grade, streak, and power-up.
void showHowToPlay() {
    showDocument("HOW TO PLAY", {
        {"CONTROLS", 4},
        {"A B C D     Select answer option", 6}, {"H           Reveal hint (answer still scored)", 6},
        {"S           Skip question (0 points)", 6}, {"Arrow keys  Navigate menus", 6},
        {"Enter       Confirm selection", 6}, {"F1          Use 50/50 power-up", 6},
        {"F2          Use Freeze power-up", 6}, {"F3          Use Double Points power-up", 6},
        {"Q           Back or quit (abandons an active quiz)", 6}, {"", 6},
        {"SCORING", 4}, {"Easy questions:    10 base points + up to 5 time bonus", 6},
        {"Medium questions:  20 base points + up to 10 time bonus", 6},
        {"Hard questions:    30 base points + up to 15 time bonus", 6},
        {"Ghost bonus:       +20 points for beating your personal best", 6}, {"", 6},
        {"GRADES", 4}, {"S grade: 90 percent and above", 6}, {"A grade: 75 percent and above", 6},
        {"B grade: 60 percent and above", 6}, {"C grade: 40 percent and above", 6}, {"D grade: below 40 percent", 6}, {"", 6},
        {"STREAKS", 4}, {"3 correct in a row: ON FIRE banner", 6},
        {"5 correct in a row: next question gives double points", 6}, {"", 6},
        {"POWER-UPS (one use each per session)", 4}, {"50/50:  Removes two wrong options", 6},
        {"Freeze: Pauses timer for 10 seconds", 6}, {"Double: Doubles points on the current question if correct", 6},
        {"Power-ups require Enter to confirm; the timer continues in the prompt.", 6},
        {"Wrong answers, skips, and timeouts earn zero. A hint does not pause time.", 6},
        {"Streak and Double multipliers stack. Invalid answer keys are marked wrong with zero points.", 6}
    });
}

// Show only this player's aggregate statistics and newest sessions, with bounded scrolling.
void showPlayerHistory() {
    vector<PlayerScore> history;
    for (const auto& score : loadScores()) if (score.name == ui().state.playerName) history.push_back(score);
    reverse(history.begin(), history.end());
    long long totalPoints = 0;
    int bestScore = 0;
    double bestAccuracy = 0;
    for (const auto& score : history) {
        totalPoints += score.score; bestScore = max(bestScore, score.score); bestAccuracy = max(bestAccuracy, score.percentage);
    }
    const string favourite = favouriteCategory(ui().state.playerName);
    int offset = 0;
    while (true) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            ui().frame("MY STATS AND HISTORY - " + ui().state.playerName);
            WINDOW* win = ui().content;
            put(win, 1, 3, "OVERALL STATS", 4, A_BOLD);
            put(win, 2, 3, "Total sessions played: " + to_string(history.size()) + "    Total points earned: " + to_string(totalPoints));
            put(win, 3, 3, "Average score: " + decimal(history.empty() ? 0 : static_cast<double>(totalPoints) / history.size()) +
                "    Best single session: " + to_string(bestScore));
            put(win, 4, 3, "Favourite category: " + favourite);
            put(win, 5, 3, "Best grade achieved: " + (history.empty() ? string("-") : gradeLabel(bestAccuracy)));
            put(win, 7, 3, "SESSION HISTORY (most recent first)", 4, A_BOLD);
            put(win, 8, 3, cell("No.", 5) + cell("Score", 8) + cell("Correct", 9) + cell("Accuracy", 9) + cell("Difficulty", 11) + "Date", 4);
            int capacity = getmaxy(win) - 11;
            offset = min(offset, max(0, static_cast<int>(history.size()) - capacity));
            if (history.empty()) put(win, 9, 3, "No history for this player yet.", 3);
            for (int i = offset; i < min(static_cast<int>(history.size()), offset + capacity); ++i)
                put(win, 9 + i - offset, 3, scoreRow(history[i], history.size() - i, false), history[i].score == bestScore ? 3 : 6);
            centered(win, getmaxy(win) - 2, string(offset > 0 ? "^ " : "") + "Up/Down: scroll   Q: back" +
                (offset + capacity < static_cast<int>(history.size()) ? " v" : ""), 3);
            wrefresh(win);
        }
        int ch = ui().key();
        if (ch == 'q' || ch == 'Q') return;
        if (ch == KEY_UP) offset = max(0, offset - 1);
        if (ch == KEY_DOWN) ++offset;
        idle();
    }
}

// Read a bounded, delimiter-safe player name without leaving ncurses input mode.
bool readPlayerName(bool changing = false) {
    string name;
    string warning;
    while (true) {
        {
            lock_guard<mutex> lock(ui().screenMutex);
            ui().frame(changing ? "CHANGE PLAYER" : "WELCOME TO QUIZ MASTER");
            centered(ui().content, 3, "Enter your name (up to 40 characters)", 4);
            centered(ui().content, 5, "> " + name + "_", 6, A_BOLD);
            centered(ui().content, 7, warning.empty() ? "Enter: continue (blank = Player)   Esc: cancel" : warning, 3);
            wrefresh(ui().content);
        }
        int ch = ui().key();
        if (ch == 27) return false;
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            const size_t first = name.find_first_not_of(" \t");
            ui().state.playerName = first == string::npos ? "Player" : name.substr(first, name.find_last_not_of(" \t") - first + 1);
            lock_guard<mutex> lock(ui().screenMutex); ui().chrome(); return true;
        }
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) { if (!name.empty()) name.pop_back(); }
        else if (ch == '|') warning = "Names cannot contain | (used by score files).";
        else if (ch >= 32 && ch <= 126 && name.size() < 40) { name += static_cast<char>(ch); warning.clear(); }
        idle();
    }
}

// Reset idle timer and transient badges when returning to the menu.
void menuChrome() {
    auto& state = ui().state;
    state.doubleActive = false; state.doubleStreakActive = false; state.frozen = false; state.notice.clear();
    lock_guard<mutex> lock(ui().screenMutex);
    ui().chrome();
    werase(ui().timer); wattrset(ui().timer, COLOR_PAIR(4)); box(ui().timer, 0, 0);
    centered(ui().timer, 1, "Timer ready", 1); wrefresh(ui().timer);
}

// Destroy every panel under the same lock used for normal window operations.
void closePanels() { lock_guard<mutex> lock(ui().screenMutex); ui().destroy(); }

// Initialize the color terminal; every return restores normal terminal mode.
int main() {
    bool initialized = false;
    try {
        setlocale(LC_ALL, "");
        srand(static_cast<unsigned>(time(nullptr)));
        if (!initscr()) throw runtime_error("Could not initialize ncurses.");
        initialized = true;
        cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
        if (!has_colors()) throw runtime_error("A color terminal is required.");
        start_color();
        if (COLORS < 8 || COLOR_PAIRS <= 10) throw runtime_error("Terminal must support eight colors and eleven color pairs.");
        const short foreground[] = {0, COLOR_GREEN, COLOR_RED, COLOR_YELLOW, COLOR_CYAN,
            COLOR_MAGENTA, COLOR_WHITE, COLOR_BLACK, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK};
        const short background[] = {0, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK,
            COLOR_BLACK, COLOR_BLACK, COLOR_YELLOW, COLOR_GREEN, COLOR_RED, COLOR_CYAN};
        for (short i = 1; i <= 10; ++i) init_pair(i, foreground[i], background[i]);
        {
            lock_guard<mutex> lock(ui().screenMutex);
            ui().rebuild();
        }
        bool running = readPlayerName();
        while (running) {
            menuChrome();
            int choice = renderMenu(ui().content, "MAIN MENU", mainOptions());
            if (choice < 0 || choice == 5) break;
            if (choice == 1) showLeaderboard();
            if (choice == 2) showPlayerHistory();
            if (choice == 3 && readPlayerName(true)) {
                const string player = ui().state.playerName;
                ui().state = SessionState{}; ui().state.playerName = player;
            }
            if (choice == 4) showHowToPlay();
            if (choice == 0) {
                const vector<string> categories = {"Science", "History", "Technology", "Sports", "Random"};
                int category = renderMenu(ui().content, "CATEGORY", categories);
                if (category < 0) continue;
                if (category == 4) category = rand() % 4;
                int difficulty = renderMenu(ui().content, "DIFFICULTY", {"Easy", "Medium", "Hard", "All (Mixed)"});
                if (difficulty < 0) continue;
                const vector<string> difficulties = {"Easy", "Medium", "Hard", "All"};
                runQuiz(categories[category], difficulties[difficulty]);
            }
        }
        closePanels();
        endwin();
        cout << "GOODBYE! Thanks for playing, " << ui().state.playerName << ".\n";
        return 0;
    } catch (const exception& e) {
        if (initialized) { closePanels(); endwin(); }
        cerr << e.what() << '\n';
        return 1;
    } catch (...) {
        if (initialized) { closePanels(); endwin(); }
        cerr << "Unexpected terminal error.\n";
        return 1;
    }
}
