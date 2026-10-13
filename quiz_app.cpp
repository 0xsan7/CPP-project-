#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

namespace Color {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* GOLD = "\033[38;5;220m";
    constexpr const char* SILVER = "\033[38;5;250m";
    constexpr const char* BRONZE = "\033[38;5;172m";
}

// Clear the terminal using ANSI cursor and screen control sequences.
void clearScreen() {
    cout << "\033[2J\033[H";
}

// Remove surrounding whitespace from one line of user input.
string trimInput(const string& input) {
    const size_t first = input.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    const size_t last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

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

enum class AnswerOutcome {
    Correct,
    Wrong,
    Invalid,
    Skipped,
    EndOfInput
};

struct AnswerResult {
    AnswerOutcome outcome;
    double secondsTaken;
};

// Display a question and collect its answer, hint request, or skip with elapsed time.
AnswerResult askQuestion(const Question& q, int questionNumber, int totalQuestions) {
    cout << Color::CYAN << "\nQ" << questionNumber << "/" << totalQuestions
         << "  [" << q.difficulty << "  +" << q.points << "pts"
         << "  " << q.timeLimit << "s]\n" << Color::RESET;
    cout << q.text << "\n\n";
    for (size_t i = 0; i < q.options.size() && i < 4; ++i) {
        cout << "  " << Color::YELLOW << static_cast<char>('A' + i)
             << Color::RESET << ". " << q.options[i] << '\n';
    }

    cout << Color::YELLOW << "\n  Enter answer (A/B/C/D), H for hint, or S to skip: "
         << Color::RESET;
    const auto start = chrono::steady_clock::now();
    string input;
    if (!getline(cin, input)) {
        return {AnswerOutcome::EndOfInput, 0.0};
    }
    input = trimInput(input);
    if (input == "H" || input == "h") {
        cout << Color::BLUE << "  Hint: " << q.hint << Color::RESET << '\n';
        cout << Color::YELLOW << "  Now enter answer (A/B/C/D), or S to skip: "
             << Color::RESET;
        if (!getline(cin, input)) {
            return {AnswerOutcome::EndOfInput, 0.0};
        }
        input = trimInput(input);
    }

    const double secondsTaken = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    if (input == "S" || input == "s") {
        cout << Color::YELLOW << "  Skipped.\n" << Color::RESET;
        return {AnswerOutcome::Skipped, secondsTaken};
    }

    const char correctLetter = static_cast<char>('A' + q.correctIndex);
    const char answerLetter = input.size() == 1
        ? static_cast<char>(toupper(static_cast<unsigned char>(input[0]))) : '\0';
    if (answerLetter < 'A' || answerLetter > 'D') {
        cout << Color::RED << "  Invalid input — marked wrong.\n";
        cout << "  Correct answer: " << correctLetter << ". "
             << q.options[q.correctIndex] << '\n' << Color::RESET;
        return {AnswerOutcome::Invalid, secondsTaken};
    }
    if (answerLetter - 'A' == q.correctIndex) {
        return {AnswerOutcome::Correct, secondsTaken};
    }
    cout << Color::RED << "  ✗ Wrong! Correct answer: " << correctLetter << ". "
         << q.options[q.correctIndex] << '\n' << Color::RESET;
    return {AnswerOutcome::Wrong, secondsTaken};
}

struct QuizResult {
    int score;
    int correct;
    int total;
    bool completed;
};

// Award base points plus a linearly decreasing bonus for a timely correct answer.
int calculatePoints(const Question& q, const AnswerResult& answer) {
    if (answer.outcome != AnswerOutcome::Correct) {
        return 0;
    }
    if (answer.secondsTaken >= q.timeLimit || q.timeLimit <= 0) {
        return q.points;
    }
    const double remaining = 1.0 - answer.secondsTaken / q.timeLimit;
    const int bonus = static_cast<int>(q.points * 0.5 * remaining);
    return q.points + bonus;
}

// Convert an accuracy percentage to its PRD grade band.
char calculateGrade(double percentage) {
    if (percentage >= 90.0) return 'S';
    if (percentage >= 75.0) return 'A';
    if (percentage >= 60.0) return 'B';
    if (percentage >= 40.0) return 'C';
    return 'D';
}

// Show the score, accuracy, and grade for a completed quiz session.
void showResults(const string& playerName, const QuizResult& result, const string& difficulty) {
    const double percentage = result.total > 0
        ? 100.0 * result.correct / result.total : 0.0;
    clearScreen();
    cout << Color::CYAN << "\nRESULTS\n" << Color::RESET
         << "  Player      : " << playerName << '\n'
         << "  Score       : " << Color::GREEN << result.score << " pts"
         << Color::RESET << '\n'
         << "  Correct     : " << result.correct << " / " << result.total << '\n'
         << "  Accuracy    : " << fixed << setprecision(1) << percentage << "%\n"
         << "  Grade       : " << calculateGrade(percentage) << '\n'
         << "  Difficulty  : " << difficulty << '\n';
}

// Filter, shuffle, and score a quiz session, returning incomplete on input loss.
QuizResult runQuiz(const string& playerName, const string& category,
                   const string& difficulty, const map<string, vector<Question>>& bank) {
    const auto categoryIt = bank.find(category);
    if (categoryIt == bank.end()) {
        cout << "No questions for this category.\n";
        return {0, 0, 0, false};
    }

    vector<Question> questions;
    for (const Question& q : categoryIt->second) {
        if (difficulty == "All" || q.difficulty == difficulty) {
            questions.push_back(q);
        }
    }
    if (questions.empty()) {
        cout << "No questions for this difficulty in category.\n";
        return {0, 0, 0, false};
    }

    mt19937 generator(random_device{}());
    for (size_t i = questions.size(); i > 1; --i) {
        uniform_int_distribution<size_t> pick(0, i - 1);
        swap(questions[i - 1], questions[pick(generator)]);
    }

    QuizResult result{0, 0, static_cast<int>(questions.size()), false};
    clearScreen();
    cout << Color::CYAN << "\n" << category << " Quiz [" << difficulty << "]\n"
         << Color::RESET
         << "  Player: " << playerName << "  |  Questions: " << result.total << '\n';
    for (int i = 0; i < result.total; ++i) {
        const AnswerResult answer = askQuestion(questions[i], i + 1, result.total);
        if (answer.outcome == AnswerOutcome::EndOfInput) {
            return result;
        }
        const int points = calculatePoints(questions[i], answer);
        if (answer.outcome == AnswerOutcome::Correct) {
            ++result.correct;
            cout << Color::GREEN << "  ✓ Correct! +" << points << " pts";
            const int bonus = points - questions[i].points;
            if (bonus > 0) cout << "  (includes +" << bonus << " time bonus)";
            cout << '\n' << Color::RESET;
        }
        result.score += points;
        this_thread::sleep_for(chrono::milliseconds(800));
    }
    result.completed = true;
    showResults(playerName, result, difficulty);
    return result;
}

// Display the ten highest scores, breaking ties by accuracy.
void showLeaderboard() {
    vector<PlayerScore> scores = loadScores();
    clearScreen();
    cout << Color::CYAN << "\nLEADERBOARD\n" << Color::RESET;
    if (scores.empty()) {
        cout << Color::YELLOW << "  No scores yet.\n" << Color::RESET;
        return;
    }

    sort(scores.begin(), scores.end(), [](const PlayerScore& a, const PlayerScore& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.percentage > b.percentage;
    });

    cout << left << setw(6) << "Rank"
         << setw(20) << "Player"
         << setw(8) << "Score"
         << setw(10) << "Correct"
         << setw(10) << "Accuracy"
         << setw(12) << "Difficulty"
         << "Date\n";
    const array<string, 3> medals = {"🥇", "🥈", "🥉"};
    const size_t shown = min<size_t>(scores.size(), 10);
    for (size_t i = 0; i < shown; ++i) {
        const PlayerScore& ps = scores[i];
        const string rank = i < medals.size() ? medals[i] : to_string(i + 1) + ".";
        const string correct = to_string(ps.correctAnswers) + "/" + to_string(ps.totalQuestions);
        ostringstream accuracy;
        accuracy << fixed << setprecision(1) << ps.percentage << '%';
        const char* rowColor = i == 0 ? Color::GOLD
            : i == 1 ? Color::SILVER : i == 2 ? Color::BRONZE : Color::RESET;
        cout << rowColor << left << setw(6) << rank
             << setw(20) << ps.name.substr(0, 18)
             << setw(8) << ps.score
             << setw(10) << correct
             << setw(10) << accuracy.str()
             << setw(12) << ps.difficulty
             << ps.date << Color::RESET << '\n';
    }
}

// Show one player's saved sessions and their games, points, and average score.
void showPlayerHistory(const string& playerName) {
    const vector<PlayerScore> scores = loadScores();
    clearScreen();
    cout << Color::CYAN << "\nHISTORY — " << playerName << '\n' << Color::RESET;

    vector<PlayerScore> history;
    for (const PlayerScore& ps : scores) {
        if (ps.name == playerName) history.push_back(ps);
    }
    if (history.empty()) {
        cout << Color::YELLOW << "  No history for " << playerName
             << " yet.\n" << Color::RESET;
        return;
    }

    cout << left << setw(8) << "Score"
         << setw(10) << "Correct"
         << setw(10) << "Accuracy"
         << setw(12) << "Difficulty"
         << "Date\n";
    long long totalPoints = 0;
    for (const PlayerScore& ps : history) {
        const string correct = to_string(ps.correctAnswers) + "/" + to_string(ps.totalQuestions);
        ostringstream accuracy;
        accuracy << fixed << setprecision(1) << ps.percentage << '%';
        cout << left << setw(8) << ps.score
             << setw(10) << correct
             << setw(10) << accuracy.str()
             << setw(12) << ps.difficulty
             << ps.date << '\n';
        totalPoints += ps.score;
    }
    const double averagePoints = static_cast<double>(totalPoints) / history.size();
    cout << "  Games played: " << history.size()
         << "  |  Total points: " << totalPoints
         << "  |  Average points: " << fixed << setprecision(1) << averagePoints << '\n';
}

// Read a numbered menu choice, returning zero when input ends.
int selectMenu(const string& prompt, const vector<string>& options) {
    while (true) {
        cout << "\n" << Color::YELLOW << prompt << Color::RESET << '\n';
        for (size_t i = 0; i < options.size(); ++i) {
            cout << "  " << Color::YELLOW << i + 1 << Color::RESET
                 << ". " << options[i] << '\n';
        }
        cout << Color::YELLOW << "  Choice: " << Color::RESET;
        string input;
        if (!getline(cin, input)) return 0;

        istringstream parser(trimInput(input));
        int choice = 0;
        string trailing;
        if ((parser >> choice) && !(parser >> trailing) &&
            choice >= 1 && choice <= static_cast<int>(options.size())) {
            return choice;
        }
        cout << Color::RED << "  Invalid choice. Enter a number from 1 to "
             << options.size() << ".\n" << Color::RESET;
    }
}

// Read a player name while rejecting the delimiter used in saved scores.
bool readPlayerName(const string& prompt, string& playerName) {
    while (true) {
        cout << Color::YELLOW << prompt << Color::RESET;
        string input;
        if (!getline(cin, input)) return false;
        input = trimInput(input);
        if (input.find('|') != string::npos) {
            cout << Color::RED << "  Names cannot contain |. Please try again.\n"
                 << Color::RESET;
            continue;
        }
        playerName = input.empty() ? "Player" : input;
        return true;
    }
}

// Keep a completed view visible until the player presses Enter or input ends.
bool waitForContinue() {
    cout << Color::YELLOW << "\n  Press Enter to continue..." << Color::RESET;
    string ignored;
    return static_cast<bool>(getline(cin, ignored));
}

// Format the local date and time for a saved score record.
string currentDate() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (local == nullptr) return "0000-00-00 00:00";
    char buffer[17] = {};
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);
    return buffer;
}

// Escape a string so the local web adapter can include it in JSON output.
string jsonEscape(const string& value) {
    ostringstream escaped;
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (ch < 0x20) {
                    escaped << "\\u" << hex << setw(4) << setfill('0')
                            << static_cast<int>(ch) << dec << setfill(' ');
                } else {
                    escaped << static_cast<char>(ch);
                }
        }
    }
    return escaped.str();
}

// Serve the existing quiz data and scoring operations to the local web server.
int runWebCommand(int argc, char* argv[]) {
    if (argc < 3) return 2;
    const string action = argv[2];
    const auto bank = buildQuestionBank();

    if (action == "catalog") {
        cout << '[';
        bool first = true;
        for (const auto& category : bank) {
            for (size_t i = 0; i < category.second.size(); ++i) {
                const Question& q = category.second[i];
                if (!first) cout << ',';
                first = false;
                cout << "{\"id\":\"" << jsonEscape(category.first + ":" + to_string(i))
                     << "\",\"category\":\"" << jsonEscape(category.first)
                     << "\",\"text\":\"" << jsonEscape(q.text)
                     << "\",\"options\":[";
                for (size_t option = 0; option < q.options.size(); ++option) {
                    if (option) cout << ',';
                    cout << '"' << jsonEscape(q.options[option]) << '"';
                }
                cout << "],\"difficulty\":\"" << jsonEscape(q.difficulty)
                     << "\",\"timeLimit\":" << q.timeLimit
                     << ",\"points\":" << q.points << '}';
            }
        }
        cout << "]\n";
        return 0;
    }

    if (action == "grade" && argc == 6) {
        const string id = argv[3];
        const size_t separator = id.rfind(':');
        if (separator == string::npos) return 2;
        const auto category = bank.find(id.substr(0, separator));
        if (category == bank.end()) return 2;
        size_t used = 0;
        int index = 0;
        double seconds = 0.0;
        try {
            index = stoi(id.substr(separator + 1), &used);
            if (used != id.size() - separator - 1) return 2;
            seconds = stod(argv[5], &used);
            if (used != string(argv[5]).size() || !isfinite(seconds) || seconds < 0) return 2;
        } catch (const exception&) {
            return 2;
        }
        if (index < 0 || static_cast<size_t>(index) >= category->second.size()) return 2;
        const Question& q = category->second[index];
        const string answer = argv[4];
        AnswerOutcome outcome = AnswerOutcome::Invalid;
        if (answer == "S") outcome = AnswerOutcome::Skipped;
        else if (answer.size() == 1 && answer[0] >= 'A' && answer[0] <= 'D') {
            outcome = answer[0] - 'A' == q.correctIndex ? AnswerOutcome::Correct : AnswerOutcome::Wrong;
        }
        const AnswerResult result{outcome, seconds};
        const int points = calculatePoints(q, result);
        const char* label = outcome == AnswerOutcome::Correct ? "correct"
            : outcome == AnswerOutcome::Wrong ? "wrong"
            : outcome == AnswerOutcome::Skipped ? "skipped" : "invalid";
        cout << "{\"outcome\":\"" << label << "\",\"points\":" << points
             << ",\"bonus\":" << (points > q.points ? points - q.points : 0)
             << ",\"correctLetter\":\"" << static_cast<char>('A' + q.correctIndex)
             << "\",\"correctText\":\"" << jsonEscape(q.options[q.correctIndex])
             << "\"}\n";
        return 0;
    }

    if (action == "hint" && argc == 4) {
        const string id = argv[3];
        const size_t separator = id.rfind(':');
        if (separator == string::npos) return 2;
        const auto category = bank.find(id.substr(0, separator));
        if (category == bank.end()) return 2;
        size_t used = 0;
        try {
            const int index = stoi(id.substr(separator + 1), &used);
            if (used != id.size() - separator - 1 || index < 0 ||
                static_cast<size_t>(index) >= category->second.size()) return 2;
            cout << "{\"hint\":\"" << jsonEscape(category->second[index].hint) << "\"}\n";
            return 0;
        } catch (const exception&) {
            return 2;
        }
    }

    if (action == "scores") {
        const vector<PlayerScore> scores = loadScores();
        cout << '[';
        for (size_t i = 0; i < scores.size(); ++i) {
            if (i) cout << ',';
            const PlayerScore& ps = scores[i];
            cout << "{\"name\":\"" << jsonEscape(ps.name)
                 << "\",\"score\":" << ps.score
                 << ",\"totalQuestions\":" << ps.totalQuestions
                 << ",\"correctAnswers\":" << ps.correctAnswers
                 << ",\"percentage\":" << ps.percentage
                 << ",\"date\":\"" << jsonEscape(ps.date)
                 << "\",\"difficulty\":\"" << jsonEscape(ps.difficulty) << "\"}";
        }
        cout << "]\n";
        return 0;
    }

    if (action == "save" && argc == 8) {
        const string name = argv[3];
        const string difficulty = argv[7];
        if (name.empty() || name.find_first_of("|\r\n") != string::npos ||
            (difficulty != "Easy" && difficulty != "Medium" &&
             difficulty != "Hard" && difficulty != "All")) return 2;
        try {
            const int score = stoi(argv[4]);
            const int total = stoi(argv[5]);
            const int correct = stoi(argv[6]);
            if (score < 0 || total <= 0 || correct < 0 || correct > total) return 2;
            const double percentage = 100.0 * correct / total;
            const bool saved = saveScore({name, score, total, correct,
                                          percentage, currentDate(), difficulty});
            cout << "{\"saved\":" << (saved ? "true" : "false") << "}\n";
            return 0;
        } catch (const exception&) {
            return 2;
        }
    }
    return 2;
}

// Run the player menu until the user quits or input ends.
int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--web") return runWebCommand(argc, argv);
    const auto bank = buildQuestionBank();
    const vector<string> categories = {"Science", "History", "Technology", "Sports"};
    const vector<string> difficulties = {"Easy", "Medium", "Hard", "All"};
    mt19937 generator(random_device{}());

    clearScreen();
    cout << Color::CYAN << "\nINTERACTIVE QUIZ APPLICATION\n"
         << "  Test your knowledge across multiple categories!\n"
         << "  Scores are saved for replay tracking and the leaderboard.\n\n"
         << Color::RESET;
    string playerName;
    if (!readPlayerName("  Enter your name: ", playerName)) return 0;

    while (true) {
        clearScreen();
        cout << Color::CYAN << "\nMAIN MENU — " << playerName
             << '\n' << Color::RESET;
        const int choice = selectMenu("What would you like to do?",
            {"Start New Quiz", "View Leaderboard", "My History", "Change Player", "Quit"});
        if (choice == 0 || choice == 5) break;

        if (choice == 1) {
            const int categoryChoice = selectMenu("Choose a category:",
                {"Science", "History", "Technology", "Sports", "Random"});
            if (categoryChoice == 0) break;
            string category;
            if (categoryChoice == 5) {
                uniform_int_distribution<int> pick(0, static_cast<int>(categories.size()) - 1);
                category = categories[pick(generator)];
                cout << Color::CYAN << "  Random pick: " << category
                     << '\n' << Color::RESET;
            } else {
                category = categories[categoryChoice - 1];
            }

            const int difficultyChoice = selectMenu("Choose difficulty:",
                {"Easy", "Medium", "Hard", "All (Mixed)"});
            if (difficultyChoice == 0) break;
            const string& difficulty = difficulties[difficultyChoice - 1];
            const QuizResult result = runQuiz(playerName, category, difficulty, bank);
            if (!result.completed) {
                if (cin.eof()) break;
                continue;
            }
            const double percentage = result.total > 0
                ? 100.0 * result.correct / result.total : 0.0;
            if (saveScore({playerName, result.score, result.total, result.correct,
                           percentage, currentDate(), difficulty})) {
                cout << Color::GREEN << "  Score saved!\n" << Color::RESET;
            } else {
                cout << Color::RED << "  Could not save score to " << SCORE_FILE
                     << ".\n" << Color::RESET;
            }
            if (!waitForContinue()) break;
        } else if (choice == 2) {
            showLeaderboard();
            if (!waitForContinue()) break;
        } else if (choice == 3) {
            showPlayerHistory(playerName);
            if (!waitForContinue()) break;
        } else if (choice == 4) {
            if (!readPlayerName("\n  Enter new player name: ", playerName)) break;
            cout << Color::GREEN << "  Welcome, " << playerName
                 << "!\n" << Color::RESET;
        }
    }

    clearScreen();
    cout << Color::CYAN << "\nGOODBYE!\n"
         << "  Thanks for playing, " << playerName << "!\n"
         << "  Your scores are saved in: " << SCORE_FILE << "\n"
         << Color::RESET;
    return 0;
}
