#pragma once
#include <nlohmann/json.hpp>
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
void clearScr();
int readInt(const std::string& prompt = "");
std::string readLine(const std::string& prompt = "");
void waitEnter();

void saveNotes(const std::vector<TextNote>& notes);
void loadNotes(std::vector<TextNote>& notes);

bool viewNote(std::vector<TextNote>& notes, int idx);
void showNotesList(std::vector<TextNote>& notes, const std::vector<int>& indices);

void viewAllNotes(std::vector<TextNote>& notes);
void searchNotes(std::vector<TextNote>& notes);
void newNote(std::vector<TextNote>& notes);

void RunApp();
