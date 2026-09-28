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
    char name[64] = "New Sound";
    char path[256] = "";
    int hotkey = 0;
    bool wasPressed = false;
    std::unique_ptr<sf::Music> music;
    std::atomic<bool> isLoaded{false};
    bool error = false;
};

std::vector<std::shared_ptr<SoundItem>> sounds;
std::mutex soundsMutex;
std::atomic<bool> running{true};

void AudioWorker() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(soundsMutex);
            for (auto& s : sounds) {
                if (s->hotkey > 0 && s->isLoaded) {
                    bool isPressed = (GetAsyncKeyState(s->hotkey) & 0x8000) != 0;
                    if (isPressed && !s->wasPressed) {
                        if (s->music && s->music->getStatus() != sf::SoundSource::Playing) {
                            s->music->play();
                        }
                    }
                    s->wasPressed = isPressed;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void SetupCyberpunkStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(10, 10);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.90f, 0.60f, 0.30f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.25f, 0.33f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.14f, 0.60f, 0.45f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.75f, 0.56f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.45f, 0.34f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.14f, 0.60f, 0.45f, 0.40f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.14f, 0.60f, 0.45f, 0.70f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.14f, 0.60f, 0.45f, 1.00f);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    sf::RenderWindow window(sf::VideoMode(950, 600), "SL Soundpad", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);
    SetupCyberpunkStyle();

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
        ImGui::SetNextWindowSize(ImVec2(950, 600));
        ImGui::Begin("MainPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.60f, 1.0f), "S L");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), " // SOUNDPAD v1.0 [Author: touchme]");
        
        ImGui::Spacing();
        if (ImGui::Button("+ ADD NEW SOUND SLOT", ImVec2(200, 35))) {
            std::lock_guard<std::mutex> lock(soundsMutex);
            sounds.push_back(std::make_shared<SoundItem>());
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("SoundListChild", ImVec2(0, -10), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        
        std::lock_guard<std::mutex> lock(soundsMutex);
        for (size_t i = 0; i < sounds.size(); ++i) {
            auto& s = sounds[i];
            ImGui::PushID(static_cast<int>(i));

            ImGui::BeginGroup();
            {
                ImGui::PushItemWidth(140);
                ImGui::InputText("##Name", s->name, sizeof(s->name));
                ImGui::PopItemWidth();
                ImGui::SameLine();

                ImGui::PushItemWidth(280);
                ImGui::InputText("##Path", s->path, sizeof(s->path));
                ImGui::PopItemWidth();
                ImGui::SameLine();

                if (ImGui::Button("LOAD")) {
                    std::string filePath(s->path);
                    std::thread([s, filePath]() {
                        auto tempMusic = std::make_unique<sf::Music>();
                        if (tempMusic->openFromFile(filePath)) {
                            std::lock_guard<std::mutex> innerLock(soundsMutex);
                            s->music = std::move(tempMusic);
                            s->isLoaded = true;
                            s->error = false;
                        } else {
                            s->error = true;
                            s->isLoaded = false;
                        }
                    }).detach();
                }
                ImGui::SameLine();

                if (s->error) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[ERR]");
                } else if (s->isLoaded) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "[OK]");
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[--]");
                }
                ImGui::SameLine();

                ImGui::PushItemWidth(80);
                ImGui::InputInt("##VK", &s->hotkey, 0);
                ImGui::PopItemWidth();
                ImGui::SameLine();

                if (ImGui::Button("PLAY") && s->isLoaded && s->music) {
                    s->music->play();
                }
                ImGui::SameLine();

                if (ImGui::Button("STOP") && s->isLoaded && s->music) {
                    s->music->stop();
                }
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.00f));
                if (ImGui::Button("DELETE")) {
                    sounds.erase(sounds.begin() + i);
                    ImGui::PopStyleColor(2);
                    ImGui::PopID();
                    ImGui::EndGroup();
                    break;
                }
                ImGui::PopStyleColor(2);
            }
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::EndChild();
        ImGui::End();

        window.clear(sf::Color(15, 15, 20));
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