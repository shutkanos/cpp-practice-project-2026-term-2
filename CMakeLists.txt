cmake_minimum_required(VERSION 3.15)
project(notes_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)

set(SOURCES
    app.cpp
    gui.cpp
    main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(notes ${SOURCES})

target_include_directories(notes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann/include
)

# ── Платформо-зависимые библиотеки ───────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # GLFW prebuilt для Windows + MinGW
    set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw-windows)
    target_include_directories(notes PRIVATE ${GLFW_DIR}/include)
    target_link_libraries(notes PRIVATE
        ${GLFW_DIR}/lib-mingw-w64/libglfw3.a
        opengl32 gdi32 imm32 dwmapi
    )
    # -mwindows скрывает консольное окно; -static убирает зависимость от DLL
    target_link_options(notes PRIVATE
        -mwindows -static -static-libgcc -static-libstdc++
    )
else()
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(notes PRIVATE glfw OpenGL::GL dl)

    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        target_link_libraries(notes PRIVATE nlohmann_json::nlohmann_json)
    endif()
endif()