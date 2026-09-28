cmake_minimum_required(VERSION 3.16)
project(SL VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# 1. Завантажуємо SFML
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)
FetchContent_MakeAvailable(SFML)

# 2. Завантажуємо оригінальний ImGui
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(ImGui)

# 3. Налаштовуємо та завантажуємо ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE) # Вимикаємо пошук системного SFML

FetchContent_Declare(
    ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.6
)
FetchContent_MakeAvailable(ImGui-SFML)

add_executable(SL WIN32 src/main.cpp)

target_link_libraries(SL PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-audio 
    sfml-system 
    ImGui-SFML::ImGui-SFML
    opengl32
    winmm
)