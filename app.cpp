#include "app.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>

using namespace std;

static const string SAVE_FILE = "notes.txt";
static const int    PAGE_SIZE = 5;

// ═══════════════════════════ helpers ════════════════════════════

static string getCurrentDateTime() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    return buf;
}

static string tagsToString(const vector<string>& tags) {
    string s;
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i) s += ' ';
        s += tags[i];
    }
    return s;
}

static string previewText(const string& text, size_t maxLen = 45) {
    size_t nl = text.find('\n');
    string first = (nl != string::npos) ? text.substr(0, nl) : text;
    if (first.size() > maxLen)
        return first.substr(0, maxLen - 3) + "...";
    return first;
}

static void clearScr() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Reads a validated integer; loops until one is entered.
static int readInt(const string& prompt = "") {
    while (true) {
        if (!prompt.empty()) cout << prompt;
        string line;
        if (!getline(cin, line)) return 0;
        if (line.empty()) { cout << "Введите число: "; continue; }
        try { return stoi(line); }
        catch (...) { cout << "Ошибка: введите целое число. "; }
    }
}

// Reads a raw text line with optional prompt.
static string readLine(const string& prompt = "") {
    if (!prompt.empty()) cout << prompt;
    string line;
    getline(cin, line);
    return line;
}

static void pause() {
    cout << "\nНажмите Enter для продолжения...";
    string dummy;
    getline(cin, dummy);
}

// ═══════════════════════════ save / load ════════════════════════

static void saveNotes(const vector<TextNote>& notes) {
    ofstream f(SAVE_FILE);
    if (!f) { cerr << "Не удалось открыть файл для записи.\n"; return; }

    for (const auto& n : notes) {
        f << "===NOTE===\n";
        f << "DATE_CREATED|" << n.date_created << '\n';
        f << "DATE_CHANGED|" << n.date_changed << '\n';
        f << "TAGS|"         << tagsToString(n.tags) << '\n';

        // Split text into lines to count them before writing.
        istringstream ss(n.text);
        vector<string> lines;
        string l;
        while (getline(ss, l)) lines.push_back(l);
        if (lines.empty()) lines.emplace_back("");

        f << "TEXT_LINES|" << lines.size() << '\n';
        for (const auto& tl : lines) f << tl << '\n';
        f << "===END===\n";
    }
}

static void loadNotes(vector<TextNote>& notes) {
    notes.clear();
    ifstream f(SAVE_FILE);
    if (!f) return;

    // Lambda: returns everything after the first '|'.
    auto field = [](const string& s) -> string {
        auto p = s.find('|');
        return (p == string::npos) ? s : s.substr(p + 1);
    };

    string line;
    while (getline(f, line)) {
        if (line != "===NOTE===") continue;

        TextNote n;
        getline(f, line); n.date_created = field(line);
        getline(f, line); n.date_changed = field(line);

        getline(f, line);
        {
            istringstream ss(field(line));
            string t;
            while (ss >> t) n.tags.push_back(t);
        }

        getline(f, line);
        int cnt = 0;
        try { cnt = stoi(field(line)); } catch (...) {}

        string text;
        for (int i = 0; i < cnt; ++i) {
            getline(f, line);
            if (i) text += '\n';
            text += line;
        }
        n.text = text;

        getline(f, line); // ===END===
        notes.push_back(n);
    }
}

// ═══════════════════════════ single note view ═══════════════════

// Returns true if the note was deleted (caller must discard stale indices).
static bool viewNote(vector<TextNote>& notes, int idx) {
    while (true) {
        clearScr();

        // Re-read through reference because the note may have changed.
        const TextNote& n = notes[idx];

        cout << "══════════ Заметка #" << (idx + 1) << " ══════════\n";
        cout << "Теги:     "
             << (n.tags.empty() ? "(нет)" : tagsToString(n.tags)) << '\n';
        cout << "Создана:  " << n.date_created << '\n';
        cout << "Изменена: " << n.date_changed << '\n';
        cout << "──────────────────────────────────────\n";
        cout << n.text << '\n';
        cout << "──────────────────────────────────────\n\n";
        cout << "1. Удалить заметку\n";
        cout << "2. Изменить текст\n";
        cout << "3. Добавить тег\n";
        cout << "4. Удалить тег\n";
        cout << "0. Назад\n";

        int ch = readInt("Выбор: ");

        switch (ch) {

        // ── Назад ──
        case 0:
            return false;

        // ── Удалить ──
        case 1: {
            int c = readInt("Подтвердить удаление? (1 - да, 0 - нет): ");
            if (c == 1) {
                notes.erase(notes.begin() + idx);
                saveNotes(notes);
                cout << "\nЗаметка удалена.\n";
                pause();
                return true;
            }
            break;
        }

        // ── Изменить текст ──
        case 2: {
            int nL = readInt("Количество строк нового текста (0 - отмена): ");
            if (nL == 0) break;
            string text;
            cout << "Введите текст:\n";
            for (int i = 0; i < nL; ++i) {
                string l = readLine(to_string(i + 1) + "> ");
                if (i) text += '\n';
                text += l;
            }
            notes[idx].text         = text;
            notes[idx].date_changed = getCurrentDateTime();
            saveNotes(notes);
            cout << "\nТекст обновлён.\n";
            pause();
            break;
        }

        // ── Добавить тег ──
        case 3: {
            string tag = readLine("Введите тег (0 - отмена): ");
            if (tag == "0" || tag.empty()) break;
            notes[idx].tags.push_back(tag);
            notes[idx].date_changed = getCurrentDateTime();
            saveNotes(notes);
            cout << "\nТег добавлен.\n";
            pause();
            break;
        }

        // ── Удалить тег ──
        case 4: {
            if (notes[idx].tags.empty()) {
                cout << "\nТегов нет.\n"; pause(); break;
            }
            cout << "\nТеги:\n";
            for (size_t i = 0; i < notes[idx].tags.size(); ++i)
                cout << "  " << (i + 1) << ". " << notes[idx].tags[i] << '\n';
            int tn = readInt("Номер тега для удаления (0 - отмена): ");
            if (tn == 0) break;
            if (tn >= 1 && tn <= (int)notes[idx].tags.size()) {
                notes[idx].tags.erase(notes[idx].tags.begin() + tn - 1);
                notes[idx].date_changed = getCurrentDateTime();
                saveNotes(notes);
                cout << "\nТег удалён.\n";
                pause();
            } else {
                cout << "\nНеверный номер.\n"; pause();
            }
            break;
        }

        default:
            cout << "\nНеверный выбор.\n"; pause();
        }
    }
}

// ═══════════════════════════ paginated list ═════════════════════

// indices — позиции заметок в векторе notes, которые нужно показать.
static void showNotesList(vector<TextNote>& notes,
                          const vector<int>& indices)
{
    if (indices.empty()) {
        cout << "\nСписок заметок пуст.\n";
        pause();
        return;
    }

    int total      = (int)indices.size();
    int totalPages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    int page       = 0;

    while (true) {
        clearScr();
        cout << "══════════ Заметки (стр. "
             << (page + 1) << " / " << totalPages
             << ") ══════════\n\n";

        int start = page * PAGE_SIZE;
        int end   = min(start + PAGE_SIZE, total);

        for (int i = start; i < end; ++i) {
            const TextNote& n   = notes[indices[i]];
            string tags_str = n.tags.empty()
                ? "(нет тегов)"
                : "[" + tagsToString(n.tags) + "]";
            cout << "  " << (i + 1) << ". "
                 << tags_str << "  "
                 << previewText(n.text) << '\n';
        }

        cout << "\n1. Вперёд\n2. Назад\n3. Выбрать заметку\n0. Выход\n";

        int ch = readInt("Выбор: ");

        switch (ch) {
        case 0:
            return;

        case 1:
            if (page < totalPages - 1) ++page;
            else { cout << "\nЭто последняя страница.\n"; pause(); }
            break;

        case 2:
            if (page > 0) --page;
            else { cout << "\nЭто первая страница.\n"; pause(); }
            break;

        case 3: {
            // Номер в общем списке (1..total), не только с текущей страницы.
            int num = readInt(
                "Номер заметки из списка 1–" + to_string(total)
                + " (0 - отмена): ");
            if (num == 0) break;
            if (num >= 1 && num <= total) {
                bool deleted = viewNote(notes, indices[num - 1]);
                if (deleted) return; // indices устарели — выходим
            } else {
                cout << "\nНеверный номер. Доступны 1–" << total << ".\n";
                pause();
            }
            break;
        }

        default:
            cout << "\nНеверный выбор.\n"; pause();
        }
    }
}

// ═══════════════════════════ main operations ════════════════════

static void viewAllNotes(vector<TextNote>& notes) {
    if (notes.empty()) {
        clearScr();
        cout << "\nЗаметок пока нет.\n";
        pause();
        return;
    }
    vector<int> idx;
    idx.reserve(notes.size());
    for (int i = 0; i < (int)notes.size(); ++i) idx.push_back(i);
    showNotesList(notes, idx);
}

static void searchNotes(vector<TextNote>& notes) {
    clearScr();
    cout << "══════════ Поиск по тегам ══════════\n\n";

    int n = readInt("Количество тегов для поиска (0 - отмена): ");
    if (n == 0) return;

    vector<string> query;
    cout << "Введите теги:\n";
    for (int i = 0; i < n; ++i) {
        string t = readLine(to_string(i + 1) + "> ");
        if (!t.empty()) query.push_back(t);
    }
    if (query.empty()) return;

    // Заметка попадает в результат, если хотя бы один её тег совпал
    // хотя бы с одним тегом запроса.
    vector<int> found;
    for (int i = 0; i < (int)notes.size(); ++i) {
        bool hit = false;
        for (const auto& qt : query) {
            for (const auto& nt : notes[i].tags)
                if (nt == qt) { hit = true; break; }
            if (hit) break;
        }
        if (hit) found.push_back(i);
    }

    if (found.empty()) {
        cout << "\nЗаметок с такими тегами не найдено.\n";
        pause();
        return;
    }

    showNotesList(notes, found);
}

static void newNote(vector<TextNote>& notes) {
    clearScr();
    cout << "══════════ Новая заметка ══════════\n\n";

    int nLines = readInt("Количество строк текста (0 - отмена): ");
    if (nLines == 0) return;

    string text;
    cout << "Введите текст:\n";
    for (int i = 0; i < nLines; ++i) {
        string l = readLine(to_string(i + 1) + "> ");
        if (i) text += '\n';
        text += l;
    }

    int nTags = readInt("\nКоличество тегов (0 - без тегов): ");
    vector<string> tags;
    if (nTags > 0) {
        cout << "Введите теги:\n";
        for (int i = 0; i < nTags; ++i) {
            string t = readLine(to_string(i + 1) + "> ");
            if (!t.empty()) tags.push_back(t);
        }
    }

    TextNote note;
    note.text         = text;
    note.date_created = getCurrentDateTime();
    note.date_changed = getCurrentDateTime();
    note.tags         = tags;

    notes.push_back(note);
    saveNotes(notes);

    cout << "\nЗаметка #" << notes.size() << " создана!\n";
    pause();
}

// ═══════════════════════════ entry point ════════════════════════

void RunApp() {
    vector<TextNote> texts;
    loadNotes(texts);

    while (true) {
        clearScr();
        cout << "╔══════════════════════════════════╗\n";
        cout << "║        Записная книжка           ║\n";
        cout << "╚══════════════════════════════════╝\n\n";
        cout << "  1. Просмотреть все заметки\n";
        cout << "  2. Поиск заметки по тегам\n";
        cout << "  3. Новая заметка\n";
        cout << "  0. Выход\n\n";

        int ch = readInt("Выбор: ");

        switch (ch) {
        case 0:
            cout << "\nДо свидания!\n";
            return;
        case 1: viewAllNotes(texts); break;
        case 2: searchNotes(texts);  break;
        case 3: newNote(texts);      break;
        default:
            cout << "\nНеверный выбор.\n"; pause();
        }
    }
}