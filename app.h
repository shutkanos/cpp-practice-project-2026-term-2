#pragma once
#include <string>
#include <vector>

struct TextNote {
    std::string text;
    std::string date_created;
    std::string date_changed;
    std::vector<std::string> tags;
};

std::string getCurrentDateTime();
std::string tagsToString(const std::vector<std::string>& tags);
std::string previewText(const std::string& text, std::size_t maxLen = 45);

void saveNotes(const std::vector<TextNote>& notes);
void loadNotes(std::vector<TextNote>& notes);

int  addNote   (std::vector<TextNote>& notes);
void deleteNote(std::vector<TextNote>& notes, int idx);
void updateText(std::vector<TextNote>& notes, int idx, const std::string& text);
void addTag    (std::vector<TextNote>& notes, int idx, const std::string& tag);
void removeTag (std::vector<TextNote>& notes, int idx, int tagIdx);
std::vector<int> filterNotes(const std::vector<TextNote>& notes, const std::string& query);

void RunApp();
