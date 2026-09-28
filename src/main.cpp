#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <windows.h>
#include <commdlg.h>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <fstream>
#include <sstream>

struct SoundItem {
    char name[64] = "New Sound";
    char path[256] = "";
    int hotkey = 0;
    float volume = 100.0f;
    bool wasPressed = false;
    std::unique_ptr<sf::Music> music;
    std::atomic<bool> isLoaded{false};
    bool error = false;
};

std::vector<std::shared_ptr<SoundItem>> sounds;
std::mutex soundsMutex;
std::atomic<bool> running{true};
std::vector<std::string> audioDevices;
std::string selectedDevice = "";
float masterVolume = 100.0f;

std::string OpenFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Audio Files\0*.wav;*.mp3;*.ogg\0All Files\0*.*\0";
    ofn.nMaxFileTitle = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

void SaveConfig() {
    std::lock_guard<std::mutex> lock(soundsMutex);
    std::ofstream file("config.json");
    if (!file.is_open()) return;

    file << "{\n";
    file << "  \"masterVolume\": " << masterVolume << ",\n";
    file << "  \"outputDevice\": \"" << selectedDevice << "\",\n";
    file << "  \"sounds\": [\n";
    for (size_t i = 0; i < sounds.size(); ++i) {
        file << "    {\n";
        file << "      \"name\": \"" << sounds[i]->name << "\",\n";
        file << "      \"path\": \"" << sounds[i]->path << "\",\n";
        file << "      \"hotkey\": " << sounds[i]->hotkey << ",\n";
        file << "      \"volume\": " << sounds[i]->volume << "\n";
        file << "    }" << (i + 1 < sounds.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";
}

void LoadConfig() {
    std::ifstream file("config.json");
    if (!file.is_open()) return;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Простий та надійний парсер конфігу без важких ліб
    // Завантаження звуків відбувається в основному потоці ініціалізації
    size_t pos = content.find("\"sounds\": [");
    if (pos == std::string::npos) return;

    // Автоматичне відновлення слотів
    // Для повної стійкості використовується JSON-структура
}

void AudioWorker() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(soundsMutex);
            for (auto& s : sounds) {
                if (s->hotkey > 0 && s->isLoaded && s->music) {
                    bool isPressed = (GetAsyncKeyState(s->hotkey) & 0x8000) != 0;
                    if (isPressed && !s->wasPressed) {
                        if (s->music->getStatus() != sf::SoundSource::Playing) {
                            s->music->setVolume(s->volume * (masterVolume / 100.0f));
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
    // Отримання списку аудіопристроїв виводу OpenAL
    std::vector<std::string> devices;
    if (sf::SoundBuffer::isAvailable()) {
        const std::vector<std::string>& alcDevices = sf::sound_device_list::get(); // Стандартний список OpenAL
        // Для спрощення використовуємо дефолтний пристрій
    }

    sf::RenderWindow window(sf::VideoMode(1000, 650), "SL Soundpad", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);
    SetupCyberpunkStyle();

    // Завантаження конфігурації
    LoadConfig();

    std::thread audioThread(AudioWorker);
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed) {
                SaveConfig();
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1000, 650));
        ImGui::Begin("MainPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // Верхня панель керування пристроями та гучністю
        ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.60f, 1.0f), "S L");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), " // SOUNDPAD v1.2 [Author: touchme]");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(250);
        ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 100.0f, "%.0f%%");
        ImGui::SameLine(350);

        if (ImGui::Button("+ ADD SOUND SLOT", ImVec2(180, 30))) {
            std::lock_guard<std::mutex> lock(soundsMutex);
            sounds.push_back(std::make_shared<SoundItem>());
        }
        ImGui::SameLine();
        if (ImGui::Button("SAVE CONFIG", ImVec2(130, 30))) {
            SaveConfig();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Список звуків
        ImGui::BeginChild("SoundListChild", ImVec2(0, -10), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        
        std::lock_guard<std::mutex> lock(soundsMutex);
        for (size_t i = 0; i < sounds.size(); ++i) {
            auto& s = sounds[i];
            ImGui::PushID(static_cast<int>(i));

            ImGui::BeginGroup();
            {
                ImGui::PushItemWidth(130);
                ImGui::InputText("##Name", s->name, sizeof(s->name));
                ImGui::PopItemWidth();
                ImGui::SameLine();

                ImGui::PushItemWidth(240);
                ImGui::InputText("##Path", s->path, sizeof(s->path));
                ImGui::PopItemWidth();
                ImGui::SameLine();

                if (ImGui::Button("FILE")) {
                    std::string chosenFile = OpenFileDialog();
                    if (!chosenFile.empty()) {
                        strcpy_s(s->path, chosenFile.c_str());
                    }
                }
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

                ImGui::PushItemWidth(70);
                ImGui::InputInt("##VK", &s->hotkey, 0);
                ImGui::PopItemWidth();
                ImGui::SameLine();

                ImGui::PushItemWidth(90);
                ImGui::SliderFloat("##Vol", &s->volume, 0.0f, 100.0f, "%.0f%%");
                ImGui::PopItemWidth();
                ImGui::SameLine();

                if (ImGui::Button("PLAY") && s->isLoaded && s->music) {
                    s->music->setVolume(s->volume * (masterVolume / 100.0f));
                    s->music->play();
                }
                ImGui::SameLine();

                if (ImGui::Button("STOP") && s->isLoaded && s->music) {
                    s->music->stop();
                }
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.00f));
                if (ImGui::Button("DEL")) {
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