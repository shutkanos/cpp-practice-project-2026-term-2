#include "app.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const std::string SAVE_FILE = "notes.json";
const int PAGE_SIZE = 5;

using json = nlohmann::json;

std::string getCurrentDateTime() {
    std::time_t now = std::time(nullptr);
    struct tm* t = std::localtime(&now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    return buf;
}

std::string tagsToString(const std::vector<std::string>& tags) {
    std::string s;
    for (std::size_t i = 0; i < tags.size(); ++i) {
        if (i)
            s += ' ';
        s += tags[i];
    }
    return s;
}

std::string previewText(const std::string& text, std::size_t maxLen) {
    std::size_t nl = text.find('\n');
    std::string first = (nl != std::string::npos) ? text.substr(0, nl) : text;
    if (first.size() > maxLen)
        return first.substr(0, maxLen - 3) + "...";
    return first;
}

void clearScr() {
    std::system("clear");
}

int readInt(const std::string& prompt) {
    while (true) {
        if (!prompt.empty())
            std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line))
            return 0;
        if (line.empty()) {
            std::cout << "Введите число: ";
            continue;
        }
        try {
            return std::stoi(line);
        } catch (...) {
            std::cout << "Ошибка: введите целое число. ";
        }
    }
}

std::string readLine(const std::string& prompt) {
    if (!prompt.empty())
        std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void waitEnter() {
    std::cout << "\nНажмите Enter для продолжения...";
    std::string temp;
    std::getline(std::cin, temp);
}

void saveNotes(const std::vector<TextNote>& notes) {
    json arr = json::array();
    for (const TextNote& n : notes) {
        arr.push_back({
            {"text",         n.text        },
            {"date_created", n.date_created},
            {"date_changed", n.date_changed},
            {"tags",         n.tags        }
        });
    }

    std::ofstream f(SAVE_FILE);
    if (!f) {
        std::cerr << "Не удалось открыть файл для записи.\n";
        return;
    }
    f << arr.dump(2);
}

void loadNotes(std::vector<TextNote>& notes) {
    notes.clear();
    std::ifstream f(SAVE_FILE);
    if (!f)
        return;

    json arr;
    try {
        f >> arr;
    } catch (const json::exception& e) {
        std::cerr << "Ошибка чтения JSON: " << e.what() << '\n';
        return;
    }

    for (const auto& obj : arr) {
        TextNote n;
        n.text = obj.value("text", "");
        n.date_created = obj.value("date_created", "");
        n.date_changed = obj.value("date_changed", "");
        n.tags = obj.value("tags", std::vector<std::string>{});
        notes.push_back(std::move(n));
    }
}

bool viewNote(std::vector<TextNote>& notes, int idx) {
    while (true) {
        clearScr();

        const TextNote& n = notes[idx];
        std::cout << "══════════ Заметка #" << (idx + 1) << " ══════════\n";
        std::cout << "Теги:     " << (n.tags.empty() ? "(нет)" : tagsToString(n.tags)) << '\n';
        std::cout << "Создана:  " << n.date_created << '\n';
        std::cout << "Изменена: " << n.date_changed << '\n';
        std::cout << "──────────────────────────────────────\n";
        std::cout << n.text << '\n';
        std::cout << "──────────────────────────────────────\n\n";
        std::cout << "1. Удалить заметку\n";
        std::cout << "2. Изменить текст\n";
        std::cout << "3. Добавить тег\n";
        std::cout << "4. Удалить тег\n";
        std::cout << "0. Назад\n";

        int ch = readInt("Выбор: ");

        switch (ch) {
            case 0:
                return false;

            case 1: {
                int c = readInt("Подтвердить удаление? (1 - да, 0 - нет): ");
                if (c == 1) {
                    notes.erase(notes.begin() + idx);
                    saveNotes(notes);
                    std::cout << "\nЗаметка удалена.\n";
                    waitEnter();
                    return true;
                }
                break;
            }

            case 2: {
                int nL = readInt("Количество строк нового текста (0 - отмена): ");
                if (nL == 0)
                    break;
                std::string text;
                std::cout << "Введите текст:\n";
                for (int i = 0; i < nL; ++i) {
                    std::string l = readLine(std::to_string(i + 1) + "> ");
                    if (i)
                        text += '\n';
                    text += l;
                }
                notes[idx].text = text;
                notes[idx].date_changed = getCurrentDateTime();
                saveNotes(notes);
                std::cout << "\nТекст обновлён.\n";
                waitEnter();
                break;
            }

            case 3: {
                std::string tag = readLine("Введите тег (0 - отмена): ");
                if (tag == "0" || tag.empty())
                    break;
                notes[idx].tags.push_back(tag);
                notes[idx].date_changed = getCurrentDateTime();
                saveNotes(notes);
                std::cout << "\nТег добавлен.\n";
                waitEnter();
                break;
            }

            case 4: {
                if (notes[idx].tags.empty()) {
                    std::cout << "\nТегов нет.\n";
                    waitEnter();
                    break;
                }
                std::cout << "\nТеги:\n";
                for (std::size_t i = 0; i < notes[idx].tags.size(); ++i)
                    std::cout << "  " << (i + 1) << ". " << notes[idx].tags[i] << '\n';
                int tn = readInt("Номер тега для удаления (0 - отмена): ");
                if (tn == 0)
                    break;
                if (tn >= 1 && tn <= (int)notes[idx].tags.size()) {
                    notes[idx].tags.erase(notes[idx].tags.begin() + tn - 1);
                    notes[idx].date_changed = getCurrentDateTime();
                    saveNotes(notes);
                    std::cout << "\nТег удалён.\n";
                    waitEnter();
                } else {
                    std::cout << "\nНеверный номер.\n";
                    waitEnter();
                }
                break;
            }

            default:
                std::cout << "\nНеверный выбор.\n";
                waitEnter();
        }
    }
}

void showNotesList(std::vector<TextNote>& notes, const std::vector<int>& indices) {
    if (indices.empty()) {
        std::cout << "\nСписок заметок пуст.\n";
        waitEnter();
        return;
    }

    int total = (int)indices.size();
    int totalPages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    int page = 0;

    while (true) {
        clearScr();
        std::cout << "══════════ Заметки (стр. " << (page + 1) << " / " << totalPages << ") ══════════\n\n";

        int start = page * PAGE_SIZE;
        int end = std::min(start + PAGE_SIZE, total);

        for (int i = start; i < end; ++i) {
            const TextNote& n = notes[indices[i]];
            std::string tags_str = n.tags.empty() ? "(нет тегов)" : "[" + tagsToString(n.tags) + "]";
            std::cout << "  " << (i + 1) << ". " << tags_str << "  " << previewText(n.text) << '\n';
        }

        std::cout << "\n1. Вперёд\n2. Назад\n3. Выбрать заметку\n0. Выход\n";

        int ch = readInt("Выбор: ");

        switch (ch) {
            case 0:
                return;

            case 1:
                if (page < totalPages - 1)
                    ++page;
                else {
                    std::cout << "\nЭто последняя страница.\n";
                    waitEnter();
                }
                break;

            case 2:
                if (page > 0)
                    --page;
                else {
                    std::cout << "\nЭто первая страница.\n";
                    waitEnter();
                }
                break;

            case 3: {
                int num = readInt("Номер заметки из списка 1–" + std::to_string(total) + " (0 - отмена): ");
                if (num == 0)
                    break;
                if (num >= 1 && num <= total) {
                    bool deleted = viewNote(notes, indices[num - 1]);
                    if (deleted)
                        return;
                } else {
                    std::cout << "\nНеверный номер. Доступны 1–" << total << ".\n";
                    waitEnter();
                }
                break;
            }

            default:
                std::cout << "\nНеверный выбор.\n";
                waitEnter();
        }
    }
}

void viewAllNotes(std::vector<TextNote>& notes) {
    if (notes.empty()) {
        clearScr();
        std::cout << "\nЗаметок пока нет.\n";
        waitEnter();
        return;
    }
    std::vector<int> idx;
    idx.reserve(notes.size());
    for (int i = 0; i < (int)notes.size(); ++i)
        idx.push_back(i);
    showNotesList(notes, idx);
}

void searchNotes(std::vector<TextNote>& notes) {
    clearScr();
    std::cout << "══════════ Поиск по тегам ══════════\n\n";

    int n = readInt("Количество тегов для поиска (0 - отмена): ");
    if (n == 0)
        return;

    std::vector<std::string> query;
    std::cout << "Введите теги:\n";
    for (int i = 0; i < n; ++i) {
        std::string t = readLine(std::to_string(i + 1) + "> ");
        if (!t.empty())
            query.push_back(t);
    }
    if (query.empty())
        return;

    std::vector<int> found;
    for (int i = 0; i < (int)notes.size(); ++i) {
        bool hit = false;
        for (const std::string& qt : query) {
            for (const std::string& nt : notes[i].tags)
                if (nt == qt) {
                    hit = true;
                    break;
                }
            if (hit)
                break;
        }
        if (hit)
            found.push_back(i);
    }

    if (found.empty()) {
        std::cout << "\nЗаметок с такими тегами не найдено.\n";
        waitEnter();
        return;
    }

    showNotesList(notes, found);
}

void newNote(std::vector<TextNote>& notes) {
    clearScr();
    std::cout << "══════════ Новая заметка ══════════\n\n";

    int nLines = readInt("Количество строк текста (0 - отмена): ");
    if (nLines == 0)
        return;

    std::string text;
    std::cout << "Введите текст:\n";
    for (int i = 0; i < nLines; ++i) {
        std::string l = readLine(std::to_string(i + 1) + "> ");
        if (i)
            text += '\n';
        text += l;
    }

    int nTags = readInt("\nКоличество тегов (0 - без тегов): ");
    std::vector<std::string> tags;
    if (nTags > 0) {
        std::cout << "Введите теги:\n";
        for (int i = 0; i < nTags; ++i) {
            std::string t = readLine(std::to_string(i + 1) + "> ");
            if (!t.empty())
                tags.push_back(t);
        }
    }

    TextNote note;
    note.text = text;
    note.date_created = getCurrentDateTime();
    note.date_changed = getCurrentDateTime();
    note.tags = tags;

    notes.push_back(note);
    saveNotes(notes);

    std::cout << "\nЗаметка #" << notes.size() << " создана!\n";
    waitEnter();
}

void RunApp() {
    std::vector<TextNote> texts;
    loadNotes(texts);

    while (true) {
        clearScr();
        std::cout << "╔══════════════════════════════════╗\n";
        std::cout << "║        Записная книжка           ║\n";
        std::cout << "╚══════════════════════════════════╝\n\n";
        std::cout << "  1. Просмотреть все заметки\n";
        std::cout << "  2. Поиск заметки по тегам\n";
        std::cout << "  3. Новая заметка\n";
        std::cout << "  0. Выход\n\n";

        int ch = readInt("Выбор: ");

        switch (ch) {
            case 0:
                std::cout << "\nДо свидания!\n";
                return;
            case 1:
                viewAllNotes(texts);
                break;
            case 2:
                searchNotes(texts);
                break;
            case 3:
                newNote(texts);
                break;
            default:
                std::cout << "\nНеверный выбор.\n";
                waitEnter();
        }
    }
}
