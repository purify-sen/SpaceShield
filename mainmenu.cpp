#include "mainmenu.h"
#include "game.h" // Cần để gọi applySettingsToGame, truy cập gameState
#include "config.h" // Bao gồm config.h
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem> // Cho create_directories (vẫn cần nếu saveSettings tạo thư mục)
#include <algorithm> // Required for std::sort
#include <string>    // Required for std::string, std::stoi
#include <cctype>    // Required for ::isdigit
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdexcept> // Required for exception handling

// Constructor MainMenu (đã cập nhật để nhận âm thanh)
MainMenu::MainMenu(SDL_Renderer* r, TTF_Font* f, Mix_Chunk* sfxClick, Mix_Music* bgm)
    : renderer(r), font(f),
      titleTexture(nullptr), playButtonTexture(nullptr), highscoreButtonTexture(nullptr),
      settingsButtonTexture(nullptr), exitButtonTexture(nullptr), highscoreTitleTexture(nullptr),
      highscoreListTexture(nullptr), settingsTitleTexture(nullptr), backButtonTexture(nullptr),
      volumeTexture(nullptr), sensitivityTexture(nullptr),
      sfxButtonClick(sfxClick), bgmMenu(bgm),
      playButton(PLAY_BUTTON_RECT), highscoreButton(HIGHSCORE_BUTTON_RECT),
      settingsButton(SETTINGS_BUTTON_RECT), exitButton(EXIT_BUTTON_RECT),
      backButton(BACK_BUTTON_RECT), volumeSlider(VOLUME_SLIDER_RECT_SETTINGS),
      volumeKnob(VOLUME_KNOB_RECT_SETTINGS), sensitivitySlider(SENSITIVITY_SLIDER_RECT_SETTINGS),
      sensitivityKnob(SENSITIVITY_KNOB_RECT_SETTINGS),
      volume(DEFAULT_VOLUME), sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)), // Vẫn khởi tạo giá trị mặc định ban đầu
      isDraggingVolumeKnob(false), isDraggingSensitivityKnob(false),
      gameState(MENU)
{
    if (!font) {
        std::cerr << "Error: Font passed to MainMenu constructor is null!" << std::endl;
        return;
    }
    std::cout << "MainMenu received a valid font pointer." << std::endl;

    // *** FIX: Gọi các hàm load để đọc từ file (nếu có) ***
    loadHighscores(); // Sẽ đọc file hoặc khởi tạo nếu file không có
    loadSettings();   // Sẽ đọc file hoặc dùng default nếu file không có

    // Helper to create textures (uses member font)
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
        if (texture) SDL_DestroyTexture(texture);
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
         // std::cout << "Successfully created texture for: " << text << std::endl;
        return true;
    };

    // Create textures
    if (!createTexture("Space Shield", titleTexture)) { /* Error log */ }
    if (!createTexture("Play", playButtonTexture)) { /* Error log */ }
    if (!createTexture("Highscore", highscoreButtonTexture)) { /* Error log */ }
    if (!createTexture("Settings", settingsButtonTexture)) { /* Error log */ }
    if (!createTexture("Exit", exitButtonTexture)) { /* Error log */ }
    if (!createTexture("Highscore", highscoreTitleTexture)) { /* Error log */ }
    if (!createTexture("Settings", settingsTitleTexture)) { /* Error log */ }
    if (!createTexture("Back", backButtonTexture)) { /* Error log */ }

    // Create dynamic textures (sẽ dùng giá trị đã load hoặc default)
    updateHighscoreListTexture();
    updateVolumeTexture();
    updateSensitivityTexture();

    // Apply initial volume to mixer (dựa trên giá trị đã load hoặc default)
    Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
    Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100);
}

MainMenu::~MainMenu() {
    // Free textures
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
}

// *** FIX: Khôi phục logic đọc file cho loadHighscores ***
void MainMenu::loadHighscores() {
    std::ifstream file(PLAYER_DATA_FILE);
    highscores.clear(); // Xóa điểm cũ trong bộ nhớ

    if (!file.is_open()) {
        std::cerr << "Could not open " << PLAYER_DATA_FILE << " for reading highscores. Initializing with defaults." << std::endl;
        // Khởi tạo với điểm 0 nếu file không tồn tại hoặc không mở được
        while (highscores.size() < MAX_HIGHSCORES_DISPLAY) {
            highscores.push_back(0);
        }
        // Không cần sắp xếp vì chỉ có số 0
        return; // Thoát khỏi hàm
    }

    std::cout << "Loading highscores from " << PLAYER_DATA_FILE << std::endl;
    std::string line;
    int score;
    // Đọc từng dòng, chỉ lấy các dòng chứa số (điểm)
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY && std::getline(file, line)) {
        // Cố gắng chuyển đổi dòng thành số nguyên
        try {
            size_t processedChars = 0;
            // Bỏ qua khoảng trắng đầu dòng
            size_t first_digit = line.find_first_not_of(" \t");
            if (first_digit == std::string::npos) continue; // Dòng trắng

            score = std::stoi(line.substr(first_digit), &processedChars);
            // Kiểm tra xem có ký tự thừa sau số không
             size_t last_digit = first_digit + processedChars;
             size_t first_nondigit_after = line.find_first_not_of(" \t", last_digit);

            if (first_nondigit_after == std::string::npos) { // Chỉ có số và khoảng trắng
                 highscores.push_back(score);
            }
        } catch (const std::invalid_argument& ia) {
            // Bỏ qua các dòng không phải số (như "Volume:", "Sensitivity:")
        } catch (const std::out_of_range& oor) {
             std::cerr << "Skipping out-of-range highscore line: " << line << std::endl;
        }
    }
    file.close(); // Đóng file sau khi đọc xong

    // Đảm bảo luôn có đủ 5 điểm (thêm 0 nếu thiếu sau khi đọc file)
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY) {
        highscores.push_back(0);
    }

    // Sắp xếp điểm cao giảm dần sau khi đã đọc từ file
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    // Giữ lại tối đa 5 điểm cao nhất (đã làm trong vòng lặp while)
     if (highscores.size() > MAX_HIGHSCORES_DISPLAY) {
        highscores.resize(MAX_HIGHSCORES_DISPLAY);
    }
}

// Lưu điểm cao mới và cài đặt vào file
void MainMenu::saveHighscores(int newScore) {
    // Thêm điểm mới vào danh sách (danh sách này đã được load hoặc reset)
    highscores.push_back(newScore);
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    if (highscores.size() > MAX_HIGHSCORES_DISPLAY) {
        highscores.resize(MAX_HIGHSCORES_DISPLAY);
    }
    updateHighscoreListTexture();
    saveSettings(); // Lưu cả điểm mới và cài đặt hiện tại
}

// *** FIX: Khôi phục logic đọc file cho loadSettings ***
void MainMenu::loadSettings() {
    std::ifstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "Could not open " << PLAYER_DATA_FILE << " for reading settings. Using defaults." << std::endl;
        // Nếu không mở được file, dùng giá trị mặc định đã có và cập nhật knob
        volume = DEFAULT_VOLUME; // Đảm bảo giá trị mặc định được dùng
        sensitivity = static_cast<int>(DEFAULT_SENSITIVITY);
        int knobRangeVol = volumeSlider.w - volumeKnob.w;
        volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRangeVol));
        int knobRangeSens = sensitivitySlider.w - sensitivityKnob.w;
        sensitivityKnob.x = sensitivitySlider.x + static_cast<int>(round(((float)sensitivity / 100.0f) * knobRangeSens));
        return; // Thoát khỏi hàm
    }

    std::cout << "Loading settings from " << PLAYER_DATA_FILE << std::endl;
    std::string line;
    // Đọc file để tìm cài đặt, nếu không tìm thấy sẽ giữ giá trị mặc định đã khởi tạo
    while (std::getline(file, line)) {
        size_t separatorPos;
        if ((separatorPos = line.find("Volume: ")) != std::string::npos) {
            try {
                volume = std::stoi(line.substr(separatorPos + 8));
                volume = std::max(0, std::min(volume, 100)); // Giới hạn 0-100
            } catch (...) {
                std::cerr << "Error reading Volume from playerdata. Using default." << std::endl;
                volume = DEFAULT_VOLUME; // Quay về default nếu đọc lỗi
            }
        } else if ((separatorPos = line.find("Sensitivity: ")) != std::string::npos) {
            try {
                sensitivity = std::stoi(line.substr(separatorPos + 13));
                sensitivity = std::max(0, std::min(sensitivity, 100)); // Giới hạn 0-100
            } catch (...) {
                 std::cerr << "Error reading Sensitivity from playerdata. Using default." << std::endl;
                 sensitivity = static_cast<int>(DEFAULT_SENSITIVITY); // Quay về default nếu đọc lỗi
            }
        }
    }
    file.close(); // Đóng file

    // Cập nhật vị trí knob dựa trên giá trị đã load (hoặc default nếu đọc lỗi/không có)
    int knobRangeVol = volumeSlider.w - volumeKnob.w;
    volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRangeVol));
    int knobRangeSens = sensitivitySlider.w - sensitivityKnob.w;
    sensitivityKnob.x = sensitivitySlider.x + static_cast<int>(round(((float)sensitivity / 100.0f) * knobRangeSens));

    // Không cần gọi update texture ở đây vì constructor sẽ gọi sau
}


// Lưu cài đặt hiện tại và điểm cao vào file (Hàm này giữ nguyên logic ghi file)
void MainMenu::saveSettings() {
    // Đảm bảo thư mục tồn tại
    if (!PLAYER_DATA_DIR.empty()) {
        try {
            std::filesystem::create_directories(PLAYER_DATA_DIR);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory " << PLAYER_DATA_DIR << ": " << e.what() << std::endl;
        } catch (...) {
             std::cerr << "Unknown error creating directory " << PLAYER_DATA_DIR << std::endl;
        }
    }

    // Mở file để ghi (ghi đè nội dung cũ)
    std::ofstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open player data file for saving: " << PLAYER_DATA_FILE << std::endl;
        return;
    }

    // Ghi điểm số trước
    for (int s : highscores) {
        file << s << "\n";
    }

    // Ghi cài đặt sau
    file << "Volume: " << volume << "\n";
    file << "Sensitivity: " << sensitivity << "\n";

    file.close();
    std::cout << "Saved scores and settings to " << PLAYER_DATA_FILE << std::endl;
}

// Cập nhật texture hiển thị danh sách điểm cao
void MainMenu::updateHighscoreListTexture() {
    if (!font) return;

    std::stringstream ss;
    bool hasScores = false;
    for (size_t i = 0; i < highscores.size(); ++i) {
        if (highscores[i] > 0 || i == 0) {
            hasScores = true;
            ss << (i + 1) << ". " << highscores[i];
            bool isLastMeaningfulScore = true;
            for(size_t j = i + 1; j < highscores.size(); ++j) {
                if (highscores[j] > 0) {
                    isLastMeaningfulScore = false;
                    break;
                }
            }
             if (!isLastMeaningfulScore && i < MAX_HIGHSCORES_DISPLAY - 1) {
                 ss << "\n";
            }
        }
         if (i >= MAX_HIGHSCORES_DISPLAY - 1) break;
    }

    std::string highscoreListStr;
    if (!hasScores && highscores.empty()) {
         highscoreListStr = "No scores yet!";
    } else if (!hasScores && !highscores.empty() && highscores[0] == 0) {
        highscoreListStr = "1. 0";
    }
    else {
        highscoreListStr = ss.str();
    }

    TTF_Font* listFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);
     if (!listFont) { /* Error log */ if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture); highscoreListTexture = nullptr; return; }
    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(listFont, highscoreListStr.c_str(), TEXT_COLOR, SCREEN_WIDTH - 100);
    TTF_CloseFont(listFont);

    if (!textSurface) { /* Error log */ if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture); highscoreListTexture = nullptr; return; }
    if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
    highscoreListTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!highscoreListTexture) { /* Error log */ }
}

// Cập nhật texture hiển thị Volume
void MainMenu::updateVolumeTexture() {
     if (!font) return;
    std::stringstream ss;
    ss << "Volume: " << volume;
    std::string volumeStr = ss.str();

    TTF_Font* volFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);
    if (!volFont) { /* Error log */ if (volumeTexture) SDL_DestroyTexture(volumeTexture); volumeTexture = nullptr; return; }
    SDL_Surface* textSurface = TTF_RenderText_Solid(volFont, volumeStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(volFont);

    if (!textSurface) { /* Error log */ if (volumeTexture) SDL_DestroyTexture(volumeTexture); volumeTexture = nullptr; return; }
    if (volumeTexture) SDL_DestroyTexture(volumeTexture);
    volumeTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!volumeTexture) { /* Error log */ }
}

// Cập nhật texture hiển thị Sensitivity
void MainMenu::updateSensitivityTexture() {
     if (!font) return;
    std::stringstream ss;
    ss << "Sensitivity: " << sensitivity;
    std::string sensitivityStr = ss.str();

    TTF_Font* sensFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);
     if (!sensFont) { /* Error log */ if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture); sensitivityTexture = nullptr; return; }
    SDL_Surface* textSurface = TTF_RenderText_Solid(sensFont, sensitivityStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(sensFont);

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
                applySettingsToGame(game); // Áp dụng cài đặt (đã load hoặc default)
                game.startGame();
            } else if (SDL_PointInRect(&mousePoint, &highscoreButton)) {
                buttonClicked = true;
                gameState = HIGHSCORE;
                loadHighscores(); // *** Load lại khi vào màn hình highscore ***
                updateHighscoreListTexture();
            } else if (SDL_PointInRect(&mousePoint, &settingsButton)) {
                buttonClicked = true;
                gameState = SETTINGS;
                // Không cần load lại settings ở đây, vì nó đã được load khi khởi tạo
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
                if (Mix_PlayingMusic() == 0 || Mix_PausedMusic() == 1) {
                     if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
                } else if (Mix_PlayingMusic() == 1) { // Nếu đang phát nhạc game
                    Mix_HaltMusic();
                    if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
                }
            }
        }
        if (gameState == SETTINGS) { /* ... xử lý click slider ... */ }
         if (buttonClicked && sfxButtonClick) { Mix_PlayChannel(CHANNEL_SFX, sfxButtonClick, 0); }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) { /* ... như cũ ... */ }
    else if (event.type == SDL_MOUSEMOTION) { /* ... như cũ ... */ }
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) { /* ... như cũ ... */ }
}

// Render MainMenu
void MainMenu::render() {
    // Xóa màn hình
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Hàm trợ giúp render texture vào giữa một Rect
    auto renderTextureCentered = [&](SDL_Texture* texture, const SDL_Rect& rect) {
        if (!texture) return; // Bỏ qua nếu texture null
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
    };

     // Hàm trợ giúp render texture tại vị trí cụ thể
    auto renderTextureAt = [&](SDL_Texture* texture, int x, int y, bool centerAlign = true) {
        if (!texture) return; // Bỏ qua nếu texture null
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect destRect = { x, y, w, h };
        if (centerAlign) {
            destRect.x = x - w / 2;
            destRect.y = y - h / 2; // Canh giữa cả chiều dọc nếu muốn
        }
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
    };


    // --- Render dựa trên trạng thái ---
    if (gameState == MENU) {
        renderTextureAt(titleTexture, SCREEN_WIDTH / 2, 100); // Canh giữa tiêu đề

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &playButton);
        renderTextureCentered(playButtonTexture, playButton);

        SDL_RenderFillRect(renderer, &highscoreButton);
        renderTextureCentered(highscoreButtonTexture, highscoreButton);

        SDL_RenderFillRect(renderer, &settingsButton);
        renderTextureCentered(settingsButtonTexture, settingsButton);

        SDL_RenderFillRect(renderer, &exitButton);
        renderTextureCentered(exitButtonTexture, exitButton);
    }
    else if (gameState == HIGHSCORE) {
        renderTextureAt(highscoreTitleTexture, SCREEN_WIDTH / 2, HIGHSCORE_TITLE_Y_MENU); // Canh giữa tiêu đề
        int listW = 0, listH = 0;
        if(highscoreListTexture) SDL_QueryTexture(highscoreListTexture, NULL, NULL, &listW, &listH);
        // Vẽ danh sách điểm cao, canh lề trái tại giữa màn hình theo chiều ngang
        renderTextureAt(highscoreListTexture, (SCREEN_WIDTH - listW) / 2, HIGHSCORE_LIST_Y, false);

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton);
        renderTextureCentered(backButtonTexture, backButton);
    }
    else if (gameState == SETTINGS) {
        renderTextureAt(settingsTitleTexture, SCREEN_WIDTH / 2, SETTINGS_TITLE_Y); // Canh giữa tiêu đề

        renderTextureAt(volumeTexture, volumeSlider.x, VOLUME_LABEL_Y_SETTINGS, false); // Canh trái label
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider);
        const SDL_Color& volKnobColor = isDraggingVolumeKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, volKnobColor.r, volKnobColor.g, volKnobColor.b, volKnobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob);

        renderTextureAt(sensitivityTexture, sensitivitySlider.x, SENSITIVITY_LABEL_Y_SETTINGS, false); // Canh trái label
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &sensitivitySlider);
         const SDL_Color& sensKnobColor = isDraggingSensitivityKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, sensKnobColor.r, sensKnobColor.g, sensKnobColor.b, sensKnobColor.a);
        SDL_RenderFillRect(renderer, &sensitivityKnob);

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton);
        renderTextureCentered(backButtonTexture, backButton);
    }

    SDL_RenderPresent(renderer);
}

// Áp dụng cài đặt từ Menu vào Game
void MainMenu::applySettingsToGame(Game& game) {
    game.setVolume(volume);
    game.setSensitivity(sensitivity);
}
