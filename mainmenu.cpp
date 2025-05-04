#include "mainmenu.h"
#include "game.h"
#include "config.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem> // Cần cho create_directories
#include <algorithm>
#include <string>
#include <cctype>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdexcept>

// Constructor MainMenu
MainMenu::MainMenu(SDL_Renderer* r, TTF_Font* f, Mix_Chunk* sfxClick, Mix_Music* bgm, SDL_Texture* bgTexture)
    : renderer(r), font(f), // Lưu font được truyền vào
      // Textures
      titleTexture(nullptr), playButtonTexture(nullptr), highscoreButtonTexture(nullptr),
      settingsButtonTexture(nullptr), exitButtonTexture(nullptr), highscoreTitleTexture(nullptr),
      highscoreListTexture(nullptr), settingsTitleTexture(nullptr), backButtonTexture(nullptr),
      volumeTexture(nullptr), sensitivityTexture(nullptr), backgroundTexture(bgTexture), // Lưu con trỏ nền
      // Âm thanh
      sfxButtonClick(sfxClick), bgmMenu(bgm),
      // Rects (Lấy giá trị từ config.h)
      playButton(PLAY_BUTTON_RECT), highscoreButton(HIGHSCORE_BUTTON_RECT),
      settingsButton(SETTINGS_BUTTON_RECT), exitButton(EXIT_BUTTON_RECT),
      backButton(BACK_BUTTON_RECT), volumeSlider(VOLUME_SLIDER_RECT_SETTINGS),
      volumeKnob(VOLUME_KNOB_RECT_SETTINGS), sensitivitySlider(SENSITIVITY_SLIDER_RECT_SETTINGS),
      sensitivityKnob(SENSITIVITY_KNOB_RECT_SETTINGS),
      // Dữ liệu & Cài đặt (Khởi tạo mặc định, sẽ được ghi đè bởi loadSettings)
      volume(DEFAULT_VOLUME), sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)),
      isDraggingVolumeKnob(false), isDraggingSensitivityKnob(false),
      gameState(MENU) // Bắt đầu ở trạng thái MENU
{
    // Kiểm tra font
    if (!font) {
        std::cerr << "Error: Font passed to MainMenu constructor is null!" << std::endl;
        return;
    }
    std::cout << "MainMenu received a valid font pointer." << std::endl;

    // Nạp dữ liệu từ file (hoặc dùng default nếu file không có/lỗi)
    loadHighscores();
    loadSettings();

    // Hàm trợ giúp tạo texture chữ (sử dụng font thành viên)
    auto createTexture = [&](const char* text, SDL_Texture*& texture) {
        if (!this->font) {
             std::cerr << "Error: Attempting to create texture \"" << text << "\" with a null member font." << std::endl;
             texture = nullptr; return false;
        }
        SDL_Surface* textSurface = TTF_RenderText_Solid(this->font, text, TEXT_COLOR);
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
             texture = nullptr; return false;
        }
        if (texture) SDL_DestroyTexture(texture); // Hủy texture cũ
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
        // std::cout << "Successfully created texture for: " << text << std::endl; // Bỏ comment nếu cần debug
        return true;
    };

    // Tạo các texture chữ cố định (sẽ dùng cỡ chữ của font thành viên)
    if (!createTexture("Space Shield", titleTexture)) { /* log lỗi */ }
    if (!createTexture("Play", playButtonTexture)) { /* log lỗi */ }
    if (!createTexture("Highscore", highscoreButtonTexture)) { /* log lỗi */ }
    if (!createTexture("Settings", settingsButtonTexture)) { /* log lỗi */ }
    if (!createTexture("Exit", exitButtonTexture)) { /* log lỗi */ }
    if (!createTexture("Highscore", highscoreTitleTexture)) { /* log lỗi */ }
    if (!createTexture("Settings", settingsTitleTexture)) { /* log lỗi */ }
    if (!createTexture("Back", backButtonTexture)) { /* log lỗi */ }

    // Tạo các texture chữ động (volume, sensitivity, highscore list)
    // Các hàm update này sẽ mở font với cỡ chữ phù hợp
    updateHighscoreListTexture();
    updateVolumeTexture();
    updateSensitivityTexture();

    // Áp dụng âm lượng ban đầu vào mixer
    Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
    Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100); // Áp dụng cho SFX
}

MainMenu::~MainMenu() {
    // Giải phóng textures đã tạo trong MainMenu
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (playButtonTexture) SDL_DestroyTexture(playButtonTexture);
    if (highscoreButtonTexture) SDL_DestroyTexture(highscoreButtonTexture);
    if (settingsButtonTexture) SDL_DestroyTexture(settingsButtonTexture);
    if (exitButtonTexture) SDL_DestroyTexture(exitButtonTexture);
    if (highscoreTitleTexture) SDL_DestroyTexture(highscoreTitleTexture);
    if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
    if (settingsTitleTexture) SDL_DestroyTexture(settingsTitleTexture);
    if (backButtonTexture) SDL_DestroyTexture(backButtonTexture);
    if (volumeTexture) SDL_DestroyTexture(volumeTexture);
    if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
    // backgroundTexture, font, và âm thanh được quản lý bên ngoài (main.cpp)
}

// Nạp điểm cao từ file hoặc khởi tạo mặc định
void MainMenu::loadHighscores() {
    std::ifstream file(PLAYER_DATA_FILE);
    highscores.clear();

    if (!file.is_open()) {
        std::cerr << "Could not open " << PLAYER_DATA_FILE << " for reading highscores. Initializing with defaults." << std::endl;
        while (highscores.size() < MAX_HIGHSCORES_DISPLAY) {
            highscores.push_back(0);
        }
        return;
    }

    std::cout << "Loading highscores from " << PLAYER_DATA_FILE << std::endl;
    std::string line;
    int score;
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY && std::getline(file, line)) {
        try {
            size_t processedChars = 0;
            size_t first_digit = line.find_first_not_of(" \t");
            if (first_digit == std::string::npos) continue;
            score = std::stoi(line.substr(first_digit), &processedChars);
            size_t last_digit = first_digit + processedChars;
            size_t first_nondigit_after = line.find_first_not_of(" \t", last_digit);
            if (first_nondigit_after == std::string::npos) {
                 highscores.push_back(score);
            }
        } catch (const std::invalid_argument& ia) { /* Bỏ qua dòng không phải số */ }
        catch (const std::out_of_range& oor) { std::cerr << "Skipping out-of-range highscore line: " << line << std::endl; }
    }
    file.close();

    while (highscores.size() < MAX_HIGHSCORES_DISPLAY) {
        highscores.push_back(0);
    }
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    if (highscores.size() > MAX_HIGHSCORES_DISPLAY) {
        highscores.resize(MAX_HIGHSCORES_DISPLAY);
    }
}

// Lưu điểm cao mới (đã được sắp xếp) và cài đặt vào file
void MainMenu::saveHighscores(int newScore) {
    highscores.push_back(newScore);
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    if (highscores.size() > MAX_HIGHSCORES_DISPLAY) {
        highscores.resize(MAX_HIGHSCORES_DISPLAY);
    }
    updateHighscoreListTexture(); // Cập nhật texture hiển thị
    saveSettings(); // Gọi hàm lưu chung
}

// Nạp cài đặt từ file hoặc dùng mặc định
void MainMenu::loadSettings() {
    std::ifstream file(PLAYER_DATA_FILE);
    // Đặt giá trị mặc định trước, sẽ bị ghi đè nếu đọc file thành công
    volume = DEFAULT_VOLUME;
    sensitivity = static_cast<int>(DEFAULT_SENSITIVITY);

    if (!file.is_open()) {
        std::cerr << "Could not open " << PLAYER_DATA_FILE << " for reading settings. Using defaults." << std::endl;
    } else {
        std::cout << "Loading settings from " << PLAYER_DATA_FILE << std::endl;
        std::string line;
        while (std::getline(file, line)) {
            size_t separatorPos;
            if ((separatorPos = line.find("Volume: ")) != std::string::npos) {
                try {
                    volume = std::stoi(line.substr(separatorPos + 8));
                    volume = std::max(0, std::min(volume, 100));
                } catch (...) { volume = DEFAULT_VOLUME; } // Reset về default nếu lỗi
            } else if ((separatorPos = line.find("Sensitivity: ")) != std::string::npos) {
                try {
                    sensitivity = std::stoi(line.substr(separatorPos + 13));
                    sensitivity = std::max(0, std::min(sensitivity, 100));
                } catch (...) { sensitivity = static_cast<int>(DEFAULT_SENSITIVITY); } // Reset về default nếu lỗi
            }
        }
        file.close();
    }

    // Cập nhật vị trí knob dựa trên giá trị cuối cùng (đã load hoặc default)
    int knobRangeVol = volumeSlider.w - volumeKnob.w;
    volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRangeVol));
    int knobRangeSens = sensitivitySlider.w - sensitivityKnob.w;
    sensitivityKnob.x = sensitivitySlider.x + static_cast<int>(round(((float)sensitivity / 100.0f) * knobRangeSens));
}

// Lưu cài đặt hiện tại và điểm cao vào file
void MainMenu::saveSettings() {
    if (!PLAYER_DATA_DIR.empty()) {
        try {
            if (!std::filesystem::exists(PLAYER_DATA_DIR)) {
                 std::filesystem::create_directories(PLAYER_DATA_DIR);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error creating directory: " << e.what() << std::endl;
        } catch (...) {
             std::cerr << "Unknown error creating directory " << PLAYER_DATA_DIR << std::endl;
        }
    }

    std::ofstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open player data file for saving: " << PLAYER_DATA_FILE << std::endl;
        return;
    }

    for (int s : highscores) { file << s << "\n"; }
    file << "Volume: " << volume << "\n";
    file << "Sensitivity: " << sensitivity << "\n";
    file.close();
    std::cout << "Saved scores and settings to " << PLAYER_DATA_FILE << std::endl;
}

// Cập nhật texture hiển thị danh sách điểm cao
void MainMenu::updateHighscoreListTexture() {
    // Mở font với cỡ chữ phù hợp
    TTF_Font* listFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);
     if (!listFont) {
        std::cerr << "TTF_OpenFont failed for highscore list: " << TTF_GetError() << std::endl;
        if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
        highscoreListTexture = nullptr;
        return;
    }

    std::stringstream ss;
    bool hasScores = false;
    for (size_t i = 0; i < highscores.size() && i < MAX_HIGHSCORES_DISPLAY; ++i) {
        // Chỉ hiển thị nếu điểm > 0 hoặc là dòng đầu tiên (để hiển thị ít nhất "1. 0")
        if (highscores[i] > 0 || i == 0) {
            hasScores = true;
            ss << (i + 1) << ". " << highscores[i];
            // Kiểm tra xem có điểm nào lớn hơn 0 ở các dòng sau không để quyết định xuống dòng
             bool isLastMeaningfulScore = true;
             for(size_t j = i + 1; j < highscores.size() && j < MAX_HIGHSCORES_DISPLAY; ++j) {
                 if (highscores[j] > 0) {
                     isLastMeaningfulScore = false;
                     break;
                 }
             }
              if (!isLastMeaningfulScore) { // Chỉ xuống dòng nếu không phải dòng cuối cùng có ý nghĩa
                  ss << "\n";
             }
        }
    }

    std::string highscoreListStr = ss.str();
    if (!hasScores) { // Nếu vector rỗng hoặc toàn số 0 (trừ trường hợp chỉ có 1 số 0)
        highscoreListStr = "No scores yet!";
        if (!highscores.empty() && highscores[0] == 0 && highscores.size() == 1) {
             highscoreListStr = "1. 0";
        }
    }


    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(listFont, highscoreListStr.c_str(), TEXT_COLOR, SCREEN_WIDTH - 100); // Wrap text
    TTF_CloseFont(listFont); // Đóng font sau khi dùng

    if (!textSurface) {
        std::cerr << "TTF_RenderText_Blended_Wrapped failed for highscore list: " << TTF_GetError() << std::endl;
        if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
        highscoreListTexture = nullptr;
        return;
    }

    if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
    highscoreListTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!highscoreListTexture) {
         std::cerr << "SDL_CreateTextureFromSurface failed for highscore list: " << SDL_GetError() << std::endl;
    }
}

// Cập nhật texture hiển thị Volume
void MainMenu::updateVolumeTexture() {
    // Mở font với cỡ chữ phù hợp
    TTF_Font* volFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);
    if (!volFont) { /* Error log */ if (volumeTexture) SDL_DestroyTexture(volumeTexture); volumeTexture = nullptr; return; }

    std::stringstream ss;
    ss << "Volume: " << volume;
    std::string volumeStr = ss.str();

    SDL_Surface* textSurface = TTF_RenderText_Solid(volFont, volumeStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(volFont); // Đóng font

    if (!textSurface) { /* Error log */ if (volumeTexture) SDL_DestroyTexture(volumeTexture); volumeTexture = nullptr; return; }
    if (volumeTexture) SDL_DestroyTexture(volumeTexture);
    volumeTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!volumeTexture) { /* Error log */ }
}

// Cập nhật texture hiển thị Sensitivity
void MainMenu::updateSensitivityTexture() {
    // Mở font với cỡ chữ phù hợp
    TTF_Font* sensFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);
     if (!sensFont) { /* Error log */ if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture); sensitivityTexture = nullptr; return; }

    std::stringstream ss;
    ss << "Sensitivity: " << sensitivity;
    std::string sensitivityStr = ss.str();

    SDL_Surface* textSurface = TTF_RenderText_Solid(sensFont, sensitivityStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(sensFont); // Đóng font

    if (!textSurface) { /* Error log */ if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture); sensitivityTexture = nullptr; return; }
    if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
    sensitivityTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!sensitivityTexture) { /* Error log */ }
}

// Xử lý input cho MainMenu
void MainMenu::handleInput(SDL_Event& event, bool& running, Game& game) {
     if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};
        bool buttonClicked = false;

        if (gameState == MENU) {
            if (SDL_PointInRect(&mousePoint, &playButton)) {
                buttonClicked = true;
                gameState = PLAYING;
                game.reset();
                applySettingsToGame(game);
                game.startGame();
            } else if (SDL_PointInRect(&mousePoint, &highscoreButton)) {
                buttonClicked = true;
                gameState = HIGHSCORE;
                loadHighscores(); // Load lại khi vào màn hình highscore
                updateHighscoreListTexture();
            } else if (SDL_PointInRect(&mousePoint, &settingsButton)) {
                buttonClicked = true;
                gameState = SETTINGS;
            } else if (SDL_PointInRect(&mousePoint, &exitButton)) {
                buttonClicked = true;
                running = false;
            }
        }
        else if (gameState == HIGHSCORE || gameState == SETTINGS) {
            if (SDL_PointInRect(&mousePoint, &backButton)) {
                buttonClicked = true;
                if (gameState == SETTINGS) {
                    saveSettings();
                    applySettingsToGame(game);
                }
                gameState = MENU;
                isDraggingVolumeKnob = false;
                isDraggingSensitivityKnob = false;
                // Bật lại nhạc menu nếu cần
                if (Mix_PlayingMusic() == 0 || Mix_PausedMusic() == 1) {
                     if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
                } else if (Mix_PlayingMusic() == 1) {
                    Mix_HaltMusic();
                    if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
                }
            }
        }
        if (gameState == SETTINGS) {
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) { buttonClicked = true; isDraggingVolumeKnob = true; }
            else if (SDL_PointInRect(&mousePoint, &sensitivityKnob)) { buttonClicked = true; isDraggingSensitivityKnob = true; }
            else if (SDL_PointInRect(&mousePoint, &volumeSlider)) { /* ... xử lý click slider volume ... */ buttonClicked = true; isDraggingVolumeKnob = true; }
            else if (SDL_PointInRect(&mousePoint, &sensitivitySlider)) { /* ... xử lý click slider sensitivity ... */ buttonClicked = true; isDraggingSensitivityKnob = true; }
        }
         if (buttonClicked && sfxButtonClick) { Mix_PlayChannel(CHANNEL_SFX, sfxButtonClick, 0); }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) { isDraggingVolumeKnob = false; isDraggingSensitivityKnob = false; }
    else if (event.type == SDL_MOUSEMOTION) { /* ... xử lý kéo slider ... */ }
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) { /* ... xử lý ESC ... */ }
}

// Render MainMenu
void MainMenu::render() {
    // Vẽ nền menu trước
    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    // Hàm trợ giúp render texture vào giữa Rect
    auto renderTextureCentered = [&](SDL_Texture* texture, const SDL_Rect& rect) {
        if (!texture) return;
        int w, h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
    };

    // Hàm trợ giúp render texture tại vị trí
    auto renderTextureAt = [&](SDL_Texture* texture, int x, int y, bool centerAlign = true) {
        if (!texture) return;
        int w, h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect destRect = { x, y, w, h };
        if (centerAlign) { destRect.x = x - w / 2; destRect.y = y - h / 2; }
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
    };

    // Render dựa trên trạng thái
    if (gameState == MENU) {
        renderTextureAt(titleTexture, SCREEN_WIDTH / 2, 100);
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &playButton); renderTextureCentered(playButtonTexture, playButton);
        SDL_RenderFillRect(renderer, &highscoreButton); renderTextureCentered(highscoreButtonTexture, highscoreButton);
        SDL_RenderFillRect(renderer, &settingsButton); renderTextureCentered(settingsButtonTexture, settingsButton);
        SDL_RenderFillRect(renderer, &exitButton); renderTextureCentered(exitButtonTexture, exitButton);
    }
    else if (gameState == HIGHSCORE) {
        renderTextureAt(highscoreTitleTexture, SCREEN_WIDTH / 2, HIGHSCORE_TITLE_Y_MENU);
        int listW = 0, listH = 0; if(highscoreListTexture) SDL_QueryTexture(highscoreListTexture, NULL, NULL, &listW, &listH);
        renderTextureAt(highscoreListTexture, (SCREEN_WIDTH - listW) / 2, HIGHSCORE_LIST_Y, false); // Canh trái
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton); renderTextureCentered(backButtonTexture, backButton);
    }
    else if (gameState == SETTINGS) {
        renderTextureAt(settingsTitleTexture, SCREEN_WIDTH / 2, SETTINGS_TITLE_Y);
        renderTextureAt(volumeTexture, volumeSlider.x, VOLUME_LABEL_Y_SETTINGS, false);
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a); SDL_RenderFillRect(renderer, &volumeSlider);
        const SDL_Color& volKnobColor = isDraggingVolumeKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR; SDL_SetRenderDrawColor(renderer, volKnobColor.r, volKnobColor.g, volKnobColor.b, volKnobColor.a); SDL_RenderFillRect(renderer, &volumeKnob);
        renderTextureAt(sensitivityTexture, sensitivitySlider.x, SENSITIVITY_LABEL_Y_SETTINGS, false);
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a); SDL_RenderFillRect(renderer, &sensitivitySlider);
        const SDL_Color& sensKnobColor = isDraggingSensitivityKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR; SDL_SetRenderDrawColor(renderer, sensKnobColor.r, sensKnobColor.g, sensKnobColor.b, sensKnobColor.a); SDL_RenderFillRect(renderer, &sensitivityKnob);
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a); SDL_RenderFillRect(renderer, &backButton); renderTextureCentered(backButtonTexture, backButton);
    }

    SDL_RenderPresent(renderer);
}

// Áp dụng cài đặt từ Menu vào Game
void MainMenu::applySettingsToGame(Game& game) {
    game.setVolume(volume);
    game.setSensitivity(sensitivity);
}
