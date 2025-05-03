#include "mainmenu.h"
#include "game.h"
#include "config.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdexcept>

// Constructor MainMenu (đã cập nhật để nhận âm thanh)
MainMenu::MainMenu(SDL_Renderer* r, TTF_Font* f, Mix_Chunk* sfxClick, Mix_Music* bgm)
    : renderer(r), font(f), // *** Lưu font được truyền vào ***
      // Textures
      titleTexture(nullptr), playButtonTexture(nullptr), highscoreButtonTexture(nullptr),
      settingsButtonTexture(nullptr), exitButtonTexture(nullptr), highscoreTitleTexture(nullptr),
      highscoreListTexture(nullptr), settingsTitleTexture(nullptr), backButtonTexture(nullptr),
      volumeTexture(nullptr), sensitivityTexture(nullptr),
      // Âm thanh
      sfxButtonClick(sfxClick), bgmMenu(bgm),
      // Rects
      playButton(PLAY_BUTTON_RECT), highscoreButton(HIGHSCORE_BUTTON_RECT),
      settingsButton(SETTINGS_BUTTON_RECT), exitButton(EXIT_BUTTON_RECT),
      backButton(BACK_BUTTON_RECT), volumeSlider(VOLUME_SLIDER_RECT_SETTINGS),
      volumeKnob(VOLUME_KNOB_RECT_SETTINGS), sensitivitySlider(SENSITIVITY_SLIDER_RECT_SETTINGS),
      sensitivityKnob(SENSITIVITY_KNOB_RECT_SETTINGS),
      // Dữ liệu & Cài đặt
      volume(DEFAULT_VOLUME), sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)),
      isDraggingVolumeKnob(false), isDraggingSensitivityKnob(false),
      gameState(MENU)
{
    // Kiểm tra xem font được truyền vào có hợp lệ không
    if (!font) {
        std::cerr << "Error: Font passed to MainMenu constructor is null!" << std::endl;
        return; // Không thể tạo texture chữ nếu không có font
    }
    std::cout << "MainMenu received a valid font pointer." << std::endl;

    loadHighscores();
    loadSettings();

    // *** FIX: Đơn giản hóa createTexture - Luôn dùng font thành viên ***
    auto createTexture = [&](const char* text, SDL_Texture*& texture) {
        if (!this->font) { // Kiểm tra lại font thành viên
             std::cerr << "Error: Attempting to create texture \"" << text << "\" with a null member font." << std::endl;
             texture = nullptr;
             return false;
        }
        // Sử dụng font thành viên trực tiếp (đã được nạp ở main với FONT_SIZE_LARGE)
        SDL_Surface* textSurface = TTF_RenderText_Solid(this->font, text, TEXT_COLOR);
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
             texture = nullptr;
            return false;
        }
        if (texture) SDL_DestroyTexture(texture); // Hủy texture cũ nếu có
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
         std::cout << "Successfully created texture for: " << text << std::endl;
        return true;
    };

    // Gọi hàm tạo texture (tất cả sẽ dùng cỡ chữ của this->font)
    if (!createTexture("Space Shield", titleTexture)) { std::cerr << "Error creating title texture." << std::endl; }
    if (!createTexture("Play", playButtonTexture)) { std::cerr << "Error creating play button texture." << std::endl; }
    if (!createTexture("Highscore", highscoreButtonTexture)) { std::cerr << "Error creating highscore button texture." << std::endl; }
    if (!createTexture("Settings", settingsButtonTexture)) { std::cerr << "Error creating settings button texture." << std::endl; }
    if (!createTexture("Exit", exitButtonTexture)) { std::cerr << "Error creating exit button texture." << std::endl; }
    if (!createTexture("Highscore", highscoreTitleTexture)) { std::cerr << "Error creating highscore title texture." << std::endl; }
    if (!createTexture("Settings", settingsTitleTexture)) { std::cerr << "Error creating settings title texture." << std::endl; }
    if (!createTexture("Back", backButtonTexture)) { std::cerr << "Error creating back button texture." << std::endl; }

    // Tạo các texture động (cũng sẽ dùng cỡ chữ của this->font)
    updateHighscoreListTexture();
    updateVolumeTexture();
    updateSensitivityTexture();

    // Áp dụng âm lượng ban đầu
    Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
    Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100);
}

MainMenu::~MainMenu() {
    // Giải phóng textures
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

// Nạp điểm cao từ file
void MainMenu::loadHighscores() {
    std::ifstream file(PLAYER_DATA_FILE);
    highscores.clear();
    if (!file.is_open()) { /* ... xử lý lỗi ... */ return; }
    std::string line; int score;
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY && std::getline(file, line)) {
        try { /* ... xử lý đọc điểm ... */ }
        catch (...) { /* ... xử lý lỗi ... */ }
    }
    file.close();
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY) { highscores.push_back(0); }
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    if (highscores.size() > MAX_HIGHSCORES_DISPLAY) { highscores.resize(MAX_HIGHSCORES_DISPLAY); }
}

// Lưu điểm cao mới và cài đặt vào file
void MainMenu::saveHighscores(int newScore) {
    highscores.push_back(newScore);
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    if (highscores.size() > MAX_HIGHSCORES_DISPLAY) { highscores.resize(MAX_HIGHSCORES_DISPLAY); }
    updateHighscoreListTexture();
    saveSettings();
}

// Nạp cài đặt (volume, sensitivity) từ file
void MainMenu::loadSettings() {
    std::ifstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) { /* ... xử lý lỗi ... */ return; }
    std::string line;
    while (std::getline(file, line)) {
        size_t separatorPos;
        if ((separatorPos = line.find("Volume: ")) != std::string::npos) { /* ... đọc volume ... */ }
        else if ((separatorPos = line.find("Sensitivity: ")) != std::string::npos) { /* ... đọc sensitivity ... */ }
    }
    file.close();
    // Cập nhật vị trí knob
    int knobRangeVol = volumeSlider.w - volumeKnob.w;
    volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRangeVol));
    int knobRangeSens = sensitivitySlider.w - sensitivityKnob.w;
    sensitivityKnob.x = sensitivitySlider.x + static_cast<int>(round(((float)sensitivity / 100.0f) * knobRangeSens));
    // Không cần gọi update texture ở đây vì constructor sẽ gọi sau
}

// Lưu cài đặt hiện tại và điểm cao vào file
void MainMenu::saveSettings() {
    try { /* ... tạo thư mục ... */ }
    catch (...) { /* ... xử lý lỗi ... */ }
    std::ofstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) { /* ... xử lý lỗi ... */ return; }
    for (int s : highscores) { file << s << "\n"; }
    file << "Volume: " << volume << "\n";
    file << "Sensitivity: " << sensitivity << "\n";
    file.close();
}

// Cập nhật texture hiển thị danh sách điểm cao
void MainMenu::updateHighscoreListTexture() {
    // *** FIX: Luôn dùng font thành viên ***
    if (!font) {
        if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
        highscoreListTexture = nullptr;
        return;
    }

    std::stringstream ss;
    // ... (tạo chuỗi highscoreListStr như cũ) ...
    for (size_t i = 0; i < highscores.size(); ++i) {
        if (i >= MAX_HIGHSCORES_DISPLAY) break;
        ss << (i + 1) << ". " << highscores[i];
        if (i < highscores.size() - 1 && i < MAX_HIGHSCORES_DISPLAY - 1) ss << "\n";
    }
    std::string highscoreListStr = ss.str();
    if (highscoreListStr.empty()) {
        highscoreListStr = "No scores yet!";
    }

    // Sử dụng RenderText_Blended_Wrapped với font thành viên
    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font, highscoreListStr.c_str(), TEXT_COLOR, BUTTON_WIDTH);

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
    // *** FIX: Luôn dùng font thành viên ***
     if (!font) {
        if (volumeTexture) SDL_DestroyTexture(volumeTexture);
        volumeTexture = nullptr;
        return;
     }

    std::stringstream ss;
    ss << "Volume: " << volume;
    std::string volumeStr = ss.str();

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, volumeStr.c_str(), TEXT_COLOR);

    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for volume text: " << TTF_GetError() << std::endl;
        if (volumeTexture) SDL_DestroyTexture(volumeTexture);
        volumeTexture = nullptr;
        return;
    }

    if (volumeTexture) SDL_DestroyTexture(volumeTexture);
    volumeTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!volumeTexture) {
         std::cerr << "SDL_CreateTextureFromSurface failed for volume text: " << SDL_GetError() << std::endl;
    }
}

// Cập nhật texture hiển thị Sensitivity
void MainMenu::updateSensitivityTexture() {
    // *** FIX: Luôn dùng font thành viên ***
     if (!font) {
        if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
        sensitivityTexture = nullptr;
        return;
     }

    std::stringstream ss;
    ss << "Sensitivity: " << sensitivity;
    std::string sensitivityStr = ss.str();

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, sensitivityStr.c_str(), TEXT_COLOR);

    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for sensitivity text: " << TTF_GetError() << std::endl;
        if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
        sensitivityTexture = nullptr;
        return;
    }

    if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
    sensitivityTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!sensitivityTexture) {
         std::cerr << "SDL_CreateTextureFromSurface failed for sensitivity text: " << SDL_GetError() << std::endl;
    }
}

// Xử lý input cho MainMenu
void MainMenu::handleInput(SDL_Event& event, bool& running, Game& game) {
    // ... (Phần xử lý input giữ nguyên như trước) ...
     if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        bool buttonClicked = false;

        // Xử lý khi ở MENU chính
        if (gameState == MENU) {
            if (SDL_PointInRect(&mousePoint, &playButton)) {
                buttonClicked = true;
                gameState = PLAYING;
                game.reset();
                game.startGame();
                applySettingsToGame(game);
            } else if (SDL_PointInRect(&mousePoint, &highscoreButton)) {
                buttonClicked = true;
                gameState = HIGHSCORE;
                loadHighscores();
                updateHighscoreListTexture();
            } else if (SDL_PointInRect(&mousePoint, &settingsButton)) {
                buttonClicked = true;
                gameState = SETTINGS;
            } else if (SDL_PointInRect(&mousePoint, &exitButton)) {
                buttonClicked = true;
                running = false;
            }
        }
        // Xử lý khi ở HIGHSCORE hoặc SETTINGS
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
                } else {
                    Mix_HaltMusic();
                    if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
                }
            }
        }

        // Xử lý click vào slider trong SETTINGS
        if (gameState == SETTINGS) {
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                buttonClicked = true;
                isDraggingVolumeKnob = true;
            }
            else if (SDL_PointInRect(&mousePoint, &sensitivityKnob)) {
                 buttonClicked = true;
                isDraggingSensitivityKnob = true;
            }
            else if (SDL_PointInRect(&mousePoint, &volumeSlider)) {
                 buttonClicked = true;
                 int newKnobX = mouseX - volumeKnob.w / 2;
                 newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
                 volumeKnob.x = newKnobX;
                 int knobRange = volumeSlider.w - volumeKnob.w;
                 volume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / knobRange) * 100.0f));
                 volume = std::max(0, std::min(volume, 100));
                 updateVolumeTexture();
                 Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
                 Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100);
                 isDraggingVolumeKnob = true;
            }
             else if (SDL_PointInRect(&mousePoint, &sensitivitySlider)) {
                 buttonClicked = true;
                 int newKnobX = mouseX - sensitivityKnob.w / 2;
                 newKnobX = std::max(sensitivitySlider.x, std::min(newKnobX, sensitivitySlider.x + sensitivitySlider.w - sensitivityKnob.w));
                 sensitivityKnob.x = newKnobX;
                 int knobRange = sensitivitySlider.w - sensitivityKnob.w;
                 sensitivity = static_cast<int>(round(((float)(newKnobX - sensitivitySlider.x) / knobRange) * 100.0f));
                 sensitivity = std::max(0, std::min(sensitivity, 100));
                 updateSensitivityTexture();
                 isDraggingSensitivityKnob = true;
            }
        }

         if (buttonClicked && sfxButtonClick) {
             Mix_PlayChannel(CHANNEL_SFX, sfxButtonClick, 0);
         }

    }
    // Xử lý thả chuột
    else if (event.type == SDL_MOUSEBUTTONUP) {
        isDraggingVolumeKnob = false;
        isDraggingSensitivityKnob = false;
    }
    // Xử lý kéo chuột
    else if (event.type == SDL_MOUSEMOTION) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        if (isDraggingVolumeKnob) {
             int newKnobX = mouseX - volumeKnob.w / 2;
            newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
            volumeKnob.x = newKnobX;
            int knobRange = volumeSlider.w - volumeKnob.w;
            volume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / knobRange) * 100.0f));
            volume = std::max(0, std::min(volume, 100));
            updateVolumeTexture();
            Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
            Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100);
        }
        else if (isDraggingSensitivityKnob) {
             int newKnobX = mouseX - sensitivityKnob.w / 2;
            newKnobX = std::max(sensitivitySlider.x, std::min(newKnobX, sensitivitySlider.x + sensitivitySlider.w - sensitivityKnob.w));
            sensitivityKnob.x = newKnobX;
             int knobRange = sensitivitySlider.w - sensitivityKnob.w;
            sensitivity = static_cast<int>(round(((float)(newKnobX - sensitivitySlider.x) / knobRange) * 100.0f));
            sensitivity = std::max(0, std::min(sensitivity, 100));
            updateSensitivityTexture();
        }
    }
     // Xử lý phím ESC
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (gameState == HIGHSCORE || gameState == SETTINGS) {
             if (sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, sfxButtonClick, 0);
             if (gameState == SETTINGS) {
                saveSettings();
                applySettingsToGame(game);
            }
            gameState = MENU;
            isDraggingVolumeKnob = false;
            isDraggingSensitivityKnob = false;
             if (Mix_PlayingMusic() == 0 || Mix_PausedMusic() == 1) {
                  if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
             } else {
                 Mix_HaltMusic();
                 if (bgmMenu) Mix_PlayMusic(bgmMenu, -1);
             }
        }
    }
}

// Render MainMenu
void MainMenu::render() {
    // Xóa màn hình
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Hàm trợ giúp render texture vào giữa một Rect
    auto renderTextureCentered = [&](SDL_Texture* texture, const SDL_Rect& rect) {
        if (texture) {
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
        } else {
            // Vẽ hình chữ nhật màu đỏ nếu texture bị lỗi để dễ debug
             SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100); // Red placeholder
             SDL_RenderDrawRect(renderer, &rect);
        }
    };

     // Hàm trợ giúp render texture tại vị trí cụ thể
    auto renderTextureAt = [&](SDL_Texture* texture, int x, int y, bool centerAlign = true) {
        if (texture) {
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            SDL_Rect destRect = { x, y, w, h };
            if (centerAlign) {
                destRect.x = x - w / 2;
            }
            SDL_RenderCopy(renderer, texture, NULL, &destRect);
        } else {
             // Có thể thêm log lỗi ở đây nếu muốn
             // std::cerr << "Attempting to render a null texture at " << x << "," << y << std::endl;
        }
    };


    // --- Render dựa trên trạng thái ---
    if (gameState == MENU) {
        renderTextureAt(titleTexture, SCREEN_WIDTH / 2, 100); // Render title

        // Render buttons
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &playButton);
        renderTextureCentered(playButtonTexture, playButton); // Render text on button

        SDL_RenderFillRect(renderer, &highscoreButton);
        renderTextureCentered(highscoreButtonTexture, highscoreButton);

        SDL_RenderFillRect(renderer, &settingsButton);
        renderTextureCentered(settingsButtonTexture, settingsButton);

        SDL_RenderFillRect(renderer, &exitButton);
        renderTextureCentered(exitButtonTexture, exitButton);
    }
    else if (gameState == HIGHSCORE) {
        renderTextureAt(highscoreTitleTexture, SCREEN_WIDTH / 2, HIGHSCORE_TITLE_Y_MENU);
        renderTextureAt(highscoreListTexture, SCREEN_WIDTH / 2, HIGHSCORE_LIST_Y, false); // Render highscore list (canh trái)

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton);
        renderTextureCentered(backButtonTexture, backButton); // Render back button
    }
    else if (gameState == SETTINGS) {
        renderTextureAt(settingsTitleTexture, SCREEN_WIDTH / 2, SETTINGS_TITLE_Y);

        // Render Volume slider
        renderTextureAt(volumeTexture, volumeSlider.x, VOLUME_LABEL_Y_SETTINGS, false); // Render volume label (left-aligned)
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider); // Render slider background
        const SDL_Color& volKnobColor = isDraggingVolumeKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, volKnobColor.r, volKnobColor.g, volKnobColor.b, volKnobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob); // Render slider knob

        // Render Sensitivity slider
        renderTextureAt(sensitivityTexture, sensitivitySlider.x, SENSITIVITY_LABEL_Y_SETTINGS, false); // Render sensitivity label (left-aligned)
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &sensitivitySlider); // Render slider background
         const SDL_Color& sensKnobColor = isDraggingSensitivityKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, sensKnobColor.r, sensKnobColor.g, sensKnobColor.b, sensKnobColor.a);
        SDL_RenderFillRect(renderer, &sensitivityKnob); // Render slider knob

        // Render Back button
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton);
        renderTextureCentered(backButtonTexture, backButton);
    }
    // Các trạng thái khác (PLAYING, PAUSED, GAME_OVER) được render bởi lớp Game

    // Hiển thị lên màn hình
    SDL_RenderPresent(renderer);
}

// Áp dụng cài đặt từ Menu vào Game
void MainMenu::applySettingsToGame(Game& game) {
    game.setVolume(volume);
    game.setSensitivity(sensitivity);
}
