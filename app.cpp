#include "app.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

const std::string SAVE_FILE = "notes.json";

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

void saveNotes(const std::vector<TextNote>& notes) {
    json arr = json::array();
    for (const TextNote& n : notes)
        arr.push_back({
            {"text", n.text},
            {"date_created", n.date_created},
            {"date_changed", n.date_changed},
            {"tags", n.tags}
        });
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
