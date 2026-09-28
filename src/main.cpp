#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <windows.h>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

struct SoundItem {
    char name[64] = "";
    char path[256] = "";
    int hotkey = 0;
    bool wasPressed = false;
    std::unique_ptr<sf::Music> music;
    bool error = false;
};

std::vector<std::shared_ptr<SoundItem>> sounds;
std::mutex soundsMutex;
std::atomic<bool> running{true};

void AudioWorker() {
    while (running) {
        std::lock_guard<std::mutex> lock(soundsMutex);
        for (auto& s : sounds) {
            if (s->hotkey > 0) {
                bool isPressed = (GetAsyncKeyState(s->hotkey) & 0x8000) != 0;
                if (isPressed && !s->wasPressed) {
                    if (s->music && s->music->getStatus() != sf::SoundSource::Playing) {
                        s->music->play();
                    }
                }
                s->wasPressed = isPressed;
            }
        }
        Sleep(10);
    }
}

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.21f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    sf::RenderWindow window(sf::VideoMode(850, 500), "SL", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);
    SetupImGuiStyle();

    std::thread audioThread(AudioWorker);
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(850, 500));
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "SL");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| Author: touchme");
        ImGui::Separator();

        if (ImGui::Button("Add Sound", ImVec2(120, 30))) {
            std::lock_guard<std::mutex> lock(soundsMutex);
            sounds.push_back(std::make_shared<SoundItem>());
        }

        ImGui::Separator();

        std::lock_guard<std::mutex> lock(soundsMutex);
        for (size_t i = 0; i < sounds.size(); ++i) {
            auto& s = sounds[i];
            ImGui::PushID(static_cast<int>(i));

            ImGui::SetNextItemWidth(120);
            ImGui::InputText("Name", s->name, sizeof(s->name));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(250);
            ImGui::InputText("Path", s->path, sizeof(s->path));
            ImGui::SameLine();

            if (ImGui::Button("Load")) {
                s->music = std::make_unique<sf::Music>();
                if (!s->music->openFromFile(s->path)) {
                    s->error = true;
                } else {
                    s->error = false;
                }
            }
            ImGui::SameLine();

            if (s->error) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ERR");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
            }
            ImGui::SameLine();

            ImGui::SetNextItemWidth(80);
            ImGui::InputInt("VK Code", &s->hotkey, 0);

            ImGui::SameLine();
            if (ImGui::Button("Play") && s->music) s->music->play();
            ImGui::SameLine();
            if (ImGui::Button("Stop") && s->music) s->music->stop();
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                sounds.erase(sounds.begin() + i);
                ImGui::PopID();
                break;
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::End();

        window.clear(sf::Color(20, 20, 22));
        ImGui::SFML::Render(window);
        window.display();
    }

    running = false;
    if (audioThread.joinable()) {
        audioThread.join();
    }

    ImGui::SFML::Shutdown();
    return 0;
}