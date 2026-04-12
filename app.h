#pragma once
#include <string>
#include <vector>

struct TextNote {
    std::string              text;
    std::string              date_created;
    std::string              date_changed;
    std::vector<std::string> tags;
};

void RunApp();