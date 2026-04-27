#include "app.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <string>
#include <vector>

static const int kTextBuf = 8192;
static const int kTagBuf  = 128;

struct AppState {
    std::vector<TextNote> notes;
    int  sel = -1;

    char search  [256]    = {};
    char edit_buf[kTextBuf] = {};
    char new_tag [kTagBuf]  = {};
} st;

static void syncBuf() {
    if (st.sel >= 0 && st.sel < (int)st.notes.size())
        strncpy(st.edit_buf, st.notes[st.sel].text.c_str(), kTextBuf - 1);
    else
        memset(st.edit_buf, 0, sizeof(st.edit_buf));
}

static void leftPanel(float w, float h) {
    ImGui::BeginChild("##left", ImVec2(w, h), true);

    ImGui::Text("Поиск:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(w - 90);
    ImGui::InputText("##srch", st.search, sizeof(st.search));
    ImGui::SameLine();

    ImGui::Spacing();
    if (ImGui::Button("+ Новая заметка", ImVec2(-1, 0))) {
        int idx = addNote(st.notes);
        st.sel = idx;
        syncBuf();
    }
    ImGui::Separator();

    ImGui::BeginChild("##list");
    auto idxs = filterNotes(st.notes, st.search);

    for (int idx : idxs) {
        const auto& n = st.notes[idx];
        std::string preview = "#" + std::to_string(idx + 1) + "  " + previewText(n.text, 28);
        if (!n.tags.empty())
            preview += "\n  [" + tagsToString(n.tags) + "]";
        preview += "##n" + std::to_string(idx);

        if (ImGui::Selectable(preview.c_str(), st.sel == idx,
                              ImGuiSelectableFlags_None, ImVec2(0, 36))) {
            if (st.sel != idx) {
                st.sel = idx;
                syncBuf();
            }
        }
        ImGui::Separator();
    }
    ImGui::EndChild();
    ImGui::EndChild();
}

static void rightPanel(float w, float h) {
    ImGui::BeginChild("##right", ImVec2(w, h), true);

    if (st.sel < 0 || st.sel >= (int)st.notes.size()) {
        ImGui::EndChild();
        return;
    }

    TextNote& n = st.notes[st.sel];

    ImGui::Text("Заметка #%d", st.sel + 1);
    ImGui::SameLine(w - 110);
    if (ImGui::SmallButton("Удалить")) {
        deleteNote(st.notes, st.sel);
        st.sel = (st.notes.empty()) ? -1 : std::min(st.sel, (int)st.notes.size() - 1);
        syncBuf();
        ImGui::EndChild();
        return;
    }
    ImGui::Separator();

    ImGui::TextDisabled("Создана:  %s", n.date_created.c_str());
    ImGui::TextDisabled("Изменена: %s", n.date_changed.c_str());
    ImGui::Spacing();

    ImGui::Text("Теги:");
    for (int i = 0; i < (int)n.tags.size(); ++i) {
        ImGui::SameLine();
        ImGui::PushID(i);
        std::string btn = n.tags[i];
        if (ImGui::SmallButton(btn.c_str())) {
            removeTag(st.notes, st.sel, i);
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }

    ImGui::SetNextItemWidth(140);
    ImGui::InputText("##ntag", st.new_tag, sizeof(st.new_tag));
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Тег") && st.new_tag[0]) {
        addTag(st.notes, st.sel, st.new_tag);
        memset(st.new_tag, 0, sizeof(st.new_tag));
    }

    ImGui::Separator();

    if (ImGui::Button("Сохранить")) {
        updateText(st.notes, st.sel, st.edit_buf);
    }

    float text_h = ImGui::GetContentRegionAvail().y - 5.0f;
    ImGui::InputTextMultiline("##ed", st.edit_buf, kTextBuf, ImVec2(-1, text_h));

    ImGui::EndChild();
}

void RunApp() {
    loadNotes(st.notes);

    if (!glfwInit()) return;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1100, 720, "Записная книжка", nullptr, nullptr);
    if (!win) { glfwTerminate(); return; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style    = ImGui::GetStyle();
    style.WindowRounding = style.ChildRounding = style.FrameRounding = 4.0f;
    style.WindowPadding  = ImVec2(10, 10);
    style.FramePadding   = ImVec2(6, 4);

    ImFontConfig cfg; cfg.OversampleH = 2;
    if (!ImGui::GetIO().Fonts->AddFontFromFileTTF(
            "C:/Windows/Fonts/arial.ttf", 16.0f, &cfg,
            ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()))
        ImGui::GetIO().Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        float avail_w = ImGui::GetContentRegionAvail().x;
        float avail_h = ImGui::GetContentRegionAvail().y;
        float left_w  = 300.0f;

        leftPanel(left_w, avail_h);
        ImGui::SameLine();
        rightPanel(avail_w - left_w - ImGui::GetStyle().ItemSpacing.x, avail_h);

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
