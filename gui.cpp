#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "app.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

static const int kTextBuf = 8192;
static const int kTagBuf = 128;

struct AppState {
    std::vector<TextNote> notes;
    int sel = -1;
    bool edit_mode = false;

    char search[256] = {};
    char edit_buf[kTextBuf] = {};
    char new_tag[kTagBuf] = {};

    // Новая заметка
    bool show_new = false;
    char new_text[kTextBuf] = {};
    char new_tags[512] = {};  // теги через пробел

    // Подтверждение удаления
    bool show_del = false;
} st;

// ── Фильтрация (поиск по тексту и тегам) ─────────────────────────────────────
static std::vector<int> filtered() {
    std::string q = st.search;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    std::vector<int> r;
    for (int i = 0; i < (int)st.notes.size(); ++i) {
        if (q.empty()) {
            r.push_back(i);
            continue;
        }
        auto lo = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        };
        if (lo(st.notes[i].text).find(q) != std::string::npos) {
            r.push_back(i);
            continue;
        }
        for (auto& t : st.notes[i].tags)
            if (lo(t).find(q) != std::string::npos) {
                r.push_back(i);
                break;
            }
    }
    return r;
}

// ── Левая панель: список заметок ──────────────────────────────────────────────
static void left_panel(float w, float h) {
    ImGui::BeginChild("##left", ImVec2(w, h), true);

    // Поиск
    ImGui::Text("Поиск:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(w - 80);
    ImGui::InputText("##srch", st.search, sizeof(st.search));
    ImGui::SameLine();
    if (ImGui::SmallButton("X"))
        memset(st.search, 0, sizeof(st.search));

    ImGui::Spacing();
    if (ImGui::Button("+ Новая заметка", ImVec2(-1, 0))) {
        memset(st.new_text, 0, sizeof(st.new_text));
        memset(st.new_tags, 0, sizeof(st.new_tags));
        st.show_new = true;
    }
    ImGui::Separator();

    // Список
    ImGui::BeginChild("##list");
    auto idxs = filtered();
    if (idxs.empty())
        ImGui::TextDisabled("Нет заметок");

    for (int idx : idxs) {
        auto& n = st.notes[idx];
        std::string lbl = "#" + std::to_string(idx + 1) + "  " + previewText(n.text, 28);
        if (!n.tags.empty())
            lbl += "\n  [" + tagsToString(n.tags) + "]";
        lbl += "##n" + std::to_string(idx);

        if (ImGui::Selectable(lbl.c_str(), st.sel == idx, ImGuiSelectableFlags_None, ImVec2(0, 36))) {
            st.sel = idx;
            st.edit_mode = false;
            strncpy(st.edit_buf, n.text.c_str(), kTextBuf - 1);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", previewText(n.text, 80).c_str());
        ImGui::Separator();
    }
    ImGui::EndChild();
    ImGui::EndChild();
}

// ── Правая панель: детали заметки ─────────────────────────────────────────────
static void right_panel(float w, float h) {
    ImGui::BeginChild("##right", ImVec2(w, h), true);

    if (st.sel < 0 || st.sel >= (int)st.notes.size()) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPos(ImVec2(avail.x / 2 - 110, avail.y / 2 - 8));
        ImGui::TextDisabled("Выберите заметку из списка");
        ImGui::EndChild();
        return;
    }

    TextNote& n = st.notes[st.sel];

    // Заголовок
    ImGui::Text("Заметка #%d", st.sel + 1);
    ImGui::SameLine(w - 100);
    if (ImGui::SmallButton("Удалить"))
        st.show_del = true;
    ImGui::Separator();

    // Даты
    ImGui::TextDisabled("Создана: %s", n.date_created.c_str());
    ImGui::TextDisabled("Изменена: %s", n.date_changed.c_str());
    ImGui::Spacing();

    // Теги: кнопка [x tag] для каждого
    ImGui::Text("Теги:");
    bool tag_removed = false;
    for (int i = 0; i < (int)n.tags.size() && !tag_removed; ++i) {
        ImGui::SameLine();
        ImGui::PushID(i);
        std::string btn_lbl = "[x] " + n.tags[i];
        if (ImGui::SmallButton(btn_lbl.c_str())) {
            n.tags.erase(n.tags.begin() + i);
            n.date_changed = getCurrentDateTime();
            saveNotes(st.notes);
            tag_removed = true;
        }
        ImGui::PopID();
    }
    if (n.tags.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(нет)");
    }

    // Добавить тег
    ImGui::SetNextItemWidth(140);
    ImGui::InputText("##ntag", st.new_tag, sizeof(st.new_tag));
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Тег") && st.new_tag[0]) {
        n.tags.emplace_back(st.new_tag);
        n.date_changed = getCurrentDateTime();
        saveNotes(st.notes);
        memset(st.new_tag, 0, sizeof(st.new_tag));
    }

    ImGui::Separator();

    // Текст
    ImGui::Text("Текст:");
    ImGui::SameLine();
    if (!st.edit_mode) {
        if (ImGui::SmallButton("Редактировать")) {
            st.edit_mode = true;
            strncpy(st.edit_buf, n.text.c_str(), kTextBuf - 1);
        }
    } else {
        if (ImGui::SmallButton("Сохранить")) {
            n.text = st.edit_buf;
            n.date_changed = getCurrentDateTime();
            saveNotes(st.notes);
            st.edit_mode = false;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Отмена"))
            st.edit_mode = false;
    }

    float text_h = ImGui::GetContentRegionAvail().y - 5.0f;
    if (st.edit_mode) {
        ImGui::InputTextMultiline("##ed", st.edit_buf, kTextBuf, ImVec2(-1, text_h));
    } else {
        ImGui::BeginChild("##view", ImVec2(-1, text_h), true);
        ImGui::TextWrapped("%s", n.text.c_str());
        ImGui::EndChild();
    }

    ImGui::EndChild();

    // Модал удаления
    if (st.show_del) {
        ImGui::OpenPopup("del##popup");
        st.show_del = false;
    }
    if (ImGui::BeginPopupModal("del##popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Удалить заметку #%d?", st.sel + 1);
        ImGui::Text("Это действие нельзя отменить.");
        ImGui::Spacing();
        if (ImGui::Button("Удалить", ImVec2(110, 0))) {
            st.notes.erase(st.notes.begin() + st.sel);
            saveNotes(st.notes);
            st.sel = -1;
            st.edit_mode = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Отмена", ImVec2(110, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ── Модал новой заметки ───────────────────────────────────────────────────────
static void new_note_modal() {
    if (st.show_new) {
        ImGui::OpenPopup("new##popup");
        st.show_new = false;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620, 450), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("new##popup", nullptr, 0)) {
        ImGui::Text("Текст заметки:");
        ImGui::InputTextMultiline("##nt", st.new_text, kTextBuf, ImVec2(-1, 300));
        ImGui::Spacing();
        ImGui::Text("Теги (через пробел):");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##ntags", st.new_tags, sizeof(st.new_tags));
        ImGui::Separator();

        bool can_create = st.new_text[0] != '\0';
        if (!can_create)
            ImGui::BeginDisabled();
        if (ImGui::Button("Создать", ImVec2(120, 0))) {
            TextNote note;
            note.text = st.new_text;
            note.date_created = note.date_changed = getCurrentDateTime();
            std::istringstream iss(st.new_tags);
            std::string tag;
            while (iss >> tag)
                note.tags.push_back(tag);
            st.notes.push_back(std::move(note));
            saveNotes(st.notes);
            st.sel = (int)st.notes.size() - 1;
            ImGui::CloseCurrentPopup();
        }
        if (!can_create)
            ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Отмена", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

// ── Главный цикл ──────────────────────────────────────────────────────────────
void RunApp() {
    loadNotes(st.notes);

    if (!glfwInit())
        return;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1100, 720, "Записная книжка", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);

    // Шрифт с поддержкой кириллицы (путь к Arial на Windows)
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    if (!io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 16.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic()))
        io.Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Корневое окно на весь экран
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("##root", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        float avail_w = ImGui::GetContentRegionAvail().x;
        float avail_h = ImGui::GetContentRegionAvail().y;
        float left_w = 300.0f;
        float right_w = avail_w - left_w - ImGui::GetStyle().ItemSpacing.x;

        left_panel(left_w, avail_h);
        ImGui::SameLine();
        right_panel(right_w, avail_h);
        new_note_modal();

        ImGui::End();
        ImGui::Render();

        int fw, fh;
        glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0.13f, 0.13f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
}
