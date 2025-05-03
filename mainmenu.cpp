#include "mainmenu.h"
#include "game.h"   // Cần để gọi applySettingsToGame, truy cập gameState
#include "config.h" // Bao gồm config.h
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem> // Cho create_directories
#include <algorithm>
#include <SDL2/SDL_image.h> // Có thể không cần nếu không load ảnh trong menu
#include <SDL2/SDL_mixer.h>
#include <filesystem>

// Hàm khởi tạo MainMenu
MainMenu::MainMenu(SDL_Renderer* r, TTF_Font* f)
    : renderer(r), font(f), // Lưu font được truyền vào
      titleTexture(nullptr), playButtonTexture(nullptr), highscoreButtonTexture(nullptr),
      settingsButtonTexture(nullptr), exitButtonTexture(nullptr), highscoreTitleTexture(nullptr),
      highscoreListTexture(nullptr), settingsTitleTexture(nullptr), backButtonTexture(nullptr),
      volumeTexture(nullptr), sensitivityTexture(nullptr),
      // Khởi tạo Rects từ config.h
      playButton(PLAY_BUTTON_RECT),
      highscoreButton(HIGHSCORE_BUTTON_RECT),
      settingsButton(SETTINGS_BUTTON_RECT),
      exitButton(EXIT_BUTTON_RECT),
      backButton(BACK_BUTTON_RECT),
      volumeSlider(VOLUME_SLIDER_RECT_SETTINGS), // Lấy rect cho màn hình settings
      volumeKnob(VOLUME_KNOB_RECT_SETTINGS),
      sensitivitySlider(SENSITIVITY_SLIDER_RECT_SETTINGS),
      sensitivityKnob(SENSITIVITY_KNOB_RECT_SETTINGS),
      // Giá trị mặc định, sẽ được ghi đè bởi loadSettings()
      volume(DEFAULT_VOLUME),
      sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)),
      isDraggingVolumeKnob(false), isDraggingSensitivityKnob(false),
      gameState(MENU) // Bắt đầu ở trạng thái MENU
      // playerDataFile(PLAYER_DATA_FILE) // Lấy từ config.h
{
    // Nạp dữ liệu từ file trước khi tạo textures
    loadHighscores(); // Nạp điểm cao
    loadSettings();   // Nạp cài đặt (ghi đè volume, sensitivity mặc định)

    // Tạo các texture chữ cố định
    // Hàm trợ giúp tạo texture
    auto createTexture = [&](const char* text, SDL_Texture*& texture, int fontSize = FONT_SIZE_LARGE) {
         TTF_Font* currentFont = TTF_OpenFont(FONT_PATH.c_str(), fontSize);
         if (!currentFont) {
             std::cerr << "TTF_OpenFont failed (" << fontSize << "): " << TTF_GetError() << std::endl;
             return false;
         }
        SDL_Surface* textSurface = TTF_RenderText_Solid(currentFont, text, TEXT_COLOR);
         TTF_CloseFont(currentFont); // Đóng font ngay sau khi dùng
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
            return false;
        }
        if (texture) SDL_DestroyTexture(texture); // Hủy texture cũ nếu có
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    };

    createTexture("Space Shield", titleTexture, FONT_SIZE_XLARGE); // Font lớn hơn cho tiêu đề
    createTexture("Play", playButtonTexture);
    createTexture("Highscore", highscoreButtonTexture);
    createTexture("Settings", settingsButtonTexture);
    createTexture("Exit", exitButtonTexture);
    createTexture("Highscore", highscoreTitleTexture); // Tiêu đề màn hình highscore
    createTexture("Settings", settingsTitleTexture);   // Tiêu đề màn hình settings
    createTexture("Back", backButtonTexture);

    // Tạo các texture động (sẽ được cập nhật)
    updateHighscoreListTexture();
    updateVolumeTexture();
    updateSensitivityTexture();

    // Áp dụng âm lượng ban đầu (sau khi loadSettings)
    Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
    // Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100); // Áp dụng cho cả sound effects nếu muốn
}

// Hàm hủy MainMenu
MainMenu::~MainMenu() {
    // Giải phóng tất cả textures đã tạo
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
    // Font được quản lý bên ngoài, không hủy ở đây
}

// Nạp điểm cao từ file
void MainMenu::loadHighscores() {
    std::ifstream file(PLAYER_DATA_FILE);
    highscores.clear(); // Xóa danh sách cũ

    if (!file.is_open()) {
        std::cerr << "Could not open " << PLAYER_DATA_FILE << " for reading highscores. Initializing with defaults." << std::endl;
        // Khởi tạo với điểm 0 nếu file không tồn tại hoặc không mở được
        while (highscores.size() < MAX_HIGHSCORES_DISPLAY) {
            highscores.push_back(0);
        }
        return;
    }

    std::string line;
    int score;
    // Đọc từng dòng, chỉ lấy các dòng chứa số (điểm)
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY && std::getline(file, line)) {
        // Cố gắng chuyển đổi dòng thành số nguyên
        try {
            size_t processedChars;
            score = std::stoi(line, &processedChars);
            // Chỉ chấp nhận nếu toàn bộ dòng là số (hoặc có khoảng trắng thừa)
            if (processedChars == line.find_last_not_of(" \t\n\r\f\v") + 1) {
                 highscores.push_back(score);
            }
        } catch (const std::invalid_argument& ia) {
            // Bỏ qua các dòng không phải số (như "Volume:", "Sensitivity:")
        } catch (const std::out_of_range& oor) {
            // Bỏ qua nếu số quá lớn
        }
    }
    file.close();

    // Đảm bảo luôn có đủ 5 điểm (thêm 0 nếu thiếu)
    while (highscores.size() < MAX_HIGHSCORES_DISPLAY) {
        highscores.push_back(0);
    }

    // Sắp xếp điểm cao giảm dần
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    // Giữ lại tối đa 5 điểm cao nhất
     if (highscores.size() > MAX_HIGHSCORES_DISPLAY) {
        highscores.resize(MAX_HIGHSCORES_DISPLAY);
    }
}

// Lưu điểm cao mới và cài đặt vào file
void MainMenu::saveHighscores(int newScore) {
    // Thêm điểm mới vào danh sách
    highscores.push_back(newScore);
    // Sắp xếp giảm dần
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    // Giữ lại top 5
    if (highscores.size() > MAX_HIGHSCORES_DISPLAY) {
        highscores.resize(MAX_HIGHSCORES_DISPLAY);
    }

    // Cập nhật texture hiển thị điểm cao
    updateHighscoreListTexture();

    // Lưu cả điểm cao và cài đặt vào file
    saveSettings(); // Gọi hàm lưu chung
}

// Nạp cài đặt (volume, sensitivity) từ file
void MainMenu::loadSettings() {
    std::ifstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "Could not open " << PLAYER_DATA_FILE << " for reading settings. Using defaults." << std::endl;
        // Sử dụng giá trị mặc định đã khởi tạo
        // Cập nhật vị trí knob dựa trên giá trị mặc định
        int knobRangeVol = volumeSlider.w - volumeKnob.w;
        volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRangeVol));
        int knobRangeSens = sensitivitySlider.w - sensitivityKnob.w;
        sensitivityKnob.x = sensitivitySlider.x + static_cast<int>(round(((float)sensitivity / 100.0f) * knobRangeSens));
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t separatorPos;
        if ((separatorPos = line.find("Volume: ")) != std::string::npos) {
            try {
                volume = std::stoi(line.substr(separatorPos + 8));
                volume = std::max(0, std::min(volume, 100)); // Giới hạn 0-100
            } catch (...) { /* Bỏ qua lỗi */ }
        } else if ((separatorPos = line.find("Sensitivity: ")) != std::string::npos) {
            try {
                sensitivity = std::stoi(line.substr(separatorPos + 13));
                sensitivity = std::max(0, std::min(sensitivity, 100)); // Giới hạn 0-100
            } catch (...) { /* Bỏ qua lỗi */ }
        }
    }
    file.close();

    // Cập nhật vị trí knob dựa trên giá trị đã load
    int knobRangeVol = volumeSlider.w - volumeKnob.w;
    volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRangeVol));
    int knobRangeSens = sensitivitySlider.w - sensitivityKnob.w;
    sensitivityKnob.x = sensitivitySlider.x + static_cast<int>(round(((float)sensitivity / 100.0f) * knobRangeSens));

    // Cập nhật textures hiển thị
    updateVolumeTexture();
    updateSensitivityTexture();
}

// Lưu cài đặt hiện tại và điểm cao vào file
void MainMenu::saveSettings() {
    // Đảm bảo thư mục tồn tại
    try {
        std::filesystem::create_directories(PLAYER_DATA_DIR);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory " << PLAYER_DATA_DIR << ": " << e.what() << std::endl;
        // Có thể không lưu được nếu không tạo được thư mục
    }

    std::ofstream file(PLAYER_DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << PLAYER_DATA_FILE << " for writing" << std::endl;
        return;
    }

    // Ghi điểm cao trước
    for (int s : highscores) {
        file << s << "\n";
    }
    // Ghi cài đặt sau
    file << "Volume: " << volume << "\n";
    file << "Sensitivity: " << sensitivity << "\n";

    file.close();
}

// Cập nhật texture hiển thị danh sách điểm cao
void MainMenu::updateHighscoreListTexture() {
    std::stringstream ss;
    for (size_t i = 0; i < highscores.size(); ++i) {
        // Đảm bảo chỉ hiển thị tối đa MAX_HIGHSCORES_DISPLAY
        if (i >= MAX_HIGHSCORES_DISPLAY) break;
        ss << (i + 1) << ". " << highscores[i];
        // Thêm newline trừ dòng cuối
        if (i < highscores.size() - 1 && i < MAX_HIGHSCORES_DISPLAY - 1) ss << "\n";
    }
    std::string highscoreListStr = ss.str();
    if (highscoreListStr.empty()) {
        highscoreListStr = "No scores yet!"; // Hoặc để trống
    }

    // Sử dụng font kích thước thường hoặc lớn tùy ý
    TTF_Font* listFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL); // Font thường
     if (!listFont) {
        std::cerr << "TTF_OpenFont failed for highscore list: " << TTF_GetError() << std::endl;
        return;
    }
    // Sử dụng RenderText_Blended_Wrapped hoặc tương tự nếu muốn xuống dòng tự động
    // Ở đây dùng Solid, cần đảm bảo các dòng đủ ngắn hoặc xử lý xuống dòng thủ công
    SDL_Surface* textSurface = TTF_RenderText_Solid(listFont, highscoreListStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(listFont);

    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for highscore list: " << TTF_GetError() << std::endl;
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
    std::stringstream ss;
    ss << "Volume: " << volume; // Hiển thị giá trị volume hiện tại
    std::string volumeStr = ss.str();

    TTF_Font* volFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL); // Font thường
    if (!volFont) {
        std::cerr << "TTF_OpenFont failed for volume text: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(volFont, volumeStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(volFont);

    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for volume text: " << TTF_GetError() << std::endl;
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
    std::stringstream ss;
    ss << "Sensitivity: " << sensitivity; // Hiển thị giá trị sensitivity hiện tại
    std::string sensitivityStr = ss.str();

    TTF_Font* sensFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL); // Font thường
     if (!sensFont) {
        std::cerr << "TTF_OpenFont failed for sensitivity text: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(sensFont, sensitivityStr.c_str(), TEXT_COLOR);
    TTF_CloseFont(sensFont);

    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for sensitivity text: " << TTF_GetError() << std::endl;
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
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        // --- Xử lý khi ở MENU chính ---
        if (gameState == MENU) {
            if (SDL_PointInRect(&mousePoint, &playButton)) {
                gameState = PLAYING; // Chuyển trạng thái
                game.reset();        // Reset game trước khi bắt đầu
                game.startGame();    // Bắt đầu game
                applySettingsToGame(game); // Áp dụng cài đặt từ menu
            } else if (SDL_PointInRect(&mousePoint, &highscoreButton)) {
                gameState = HIGHSCORE; // Chuyển sang màn hình highscore
                loadHighscores(); // Nạp lại điểm cao phòng trường hợp file thay đổi
                updateHighscoreListTexture(); // Cập nhật hiển thị
            } else if (SDL_PointInRect(&mousePoint, &settingsButton)) {
                gameState = SETTINGS; // Chuyển sang màn hình settings
            } else if (SDL_PointInRect(&mousePoint, &exitButton)) {
                running = false; // Tín hiệu thoát game
            }
        }
        // --- Xử lý khi ở HIGHSCORE hoặc SETTINGS ---
        else if (gameState == HIGHSCORE || gameState == SETTINGS) {
            if (SDL_PointInRect(&mousePoint, &backButton)) {
                if (gameState == SETTINGS) {
                    saveSettings(); // Lưu cài đặt khi thoát khỏi màn hình Settings
                    applySettingsToGame(game); // Áp dụng lại cài đặt cho game
                }
                gameState = MENU; // Quay về menu chính
                isDraggingVolumeKnob = false; // Reset trạng thái kéo thả
                isDraggingSensitivityKnob = false;
            }
        }

        // --- Xử lý click vào slider trong SETTINGS ---
        if (gameState == SETTINGS) {
            // Click vào volume knob
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                isDraggingVolumeKnob = true;
            }
            // Click vào sensitivity knob
            else if (SDL_PointInRect(&mousePoint, &sensitivityKnob)) {
                isDraggingSensitivityKnob = true;
            }
            // Click trực tiếp vào thanh slider (di chuyển knob đến vị trí click) - Tùy chọn
            else if (SDL_PointInRect(&mousePoint, &volumeSlider)) {
                 int newKnobX = mouseX - volumeKnob.w / 2;
                 newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
                 volumeKnob.x = newKnobX;
                 int knobRange = volumeSlider.w - volumeKnob.w;
                 volume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / knobRange) * 100.0f));
                 volume = std::max(0, std::min(volume, 100));
                 updateVolumeTexture();
                 Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
                 isDraggingVolumeKnob = true; // Bắt đầu kéo từ vị trí mới
            }
             else if (SDL_PointInRect(&mousePoint, &sensitivitySlider)) {
                 int newKnobX = mouseX - sensitivityKnob.w / 2;
                 newKnobX = std::max(sensitivitySlider.x, std::min(newKnobX, sensitivitySlider.x + sensitivitySlider.w - sensitivityKnob.w));
                 sensitivityKnob.x = newKnobX;
                 int knobRange = sensitivitySlider.w - sensitivityKnob.w;
                 sensitivity = static_cast<int>(round(((float)(newKnobX - sensitivitySlider.x) / knobRange) * 100.0f));
                 sensitivity = std::max(0, std::min(sensitivity, 100));
                 updateSensitivityTexture();
                 isDraggingSensitivityKnob = true; // Bắt đầu kéo
            }
        }
    }
    // --- Xử lý thả chuột ---
    else if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingVolumeKnob || isDraggingSensitivityKnob) {
            // Không cần lưu ở đây nữa, lưu khi nhấn Back
            // saveSettings();
            // applySettingsToGame(game);
        }
        isDraggingVolumeKnob = false;
        isDraggingSensitivityKnob = false;
    }
    // --- Xử lý kéo chuột ---
    else if (event.type == SDL_MOUSEMOTION) {
        int mouseX, mouseY; // Lấy tọa độ chuột
        SDL_GetMouseState(&mouseX, &mouseY);

        // Kéo volume knob
        if (isDraggingVolumeKnob) {
            int newKnobX = mouseX - volumeKnob.w / 2;
            newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
            volumeKnob.x = newKnobX;
            int knobRange = volumeSlider.w - volumeKnob.w; // Phạm vi di chuyển của tâm knob
            volume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / knobRange) * 100.0f));
            volume = std::max(0, std::min(volume, 100)); // Giới hạn 0-100
            updateVolumeTexture(); // Cập nhật text hiển thị
            Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100); // Áp dụng âm lượng ngay lập tức
            // Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100); // Cho sound effects
        }
        // Kéo sensitivity knob
        else if (isDraggingSensitivityKnob) {
            int newKnobX = mouseX - sensitivityKnob.w / 2;
            newKnobX = std::max(sensitivitySlider.x, std::min(newKnobX, sensitivitySlider.x + sensitivitySlider.w - sensitivityKnob.w));
            sensitivityKnob.x = newKnobX;
             int knobRange = sensitivitySlider.w - sensitivityKnob.w;
            sensitivity = static_cast<int>(round(((float)(newKnobX - sensitivitySlider.x) / knobRange) * 100.0f));
            sensitivity = std::max(0, std::min(sensitivity, 100)); // Giới hạn 0-100
            updateSensitivityTexture(); // Cập nhật text hiển thị
            // Không cần áp dụng ngay cho game, sẽ áp dụng khi thoát Settings hoặc bắt đầu game mới
        }
    }
     // --- Xử lý phím ESC ---
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (gameState == HIGHSCORE || gameState == SETTINGS) {
             if (gameState == SETTINGS) {
                saveSettings();
                applySettingsToGame(game);
            }
            gameState = MENU;
            isDraggingVolumeKnob = false;
            isDraggingSensitivityKnob = false;
        }
        // Nếu ở MENU chính, ESC không làm gì (hoặc có thể thêm xác nhận thoát?)
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
            // Canh giữa texture trong rect
            SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
        }
    };

     // Hàm trợ giúp render texture tại vị trí cụ thể
    auto renderTextureAt = [&](SDL_Texture* texture, int x, int y, bool centerAlign = true) {
        if (texture) {
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            SDL_Rect destRect = { x, y, w, h };
            if (centerAlign) {
                destRect.x = x - w / 2; // Canh giữa theo chiều ngang
            }
            SDL_RenderCopy(renderer, texture, NULL, &destRect);
        }
    };


    // --- Render dựa trên trạng thái ---
    if (gameState == MENU) {
        // Render tiêu đề game
        renderTextureAt(titleTexture, SCREEN_WIDTH / 2, 100);

        // Render các nút
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
        // Render tiêu đề Highscore
        renderTextureAt(highscoreTitleTexture, SCREEN_WIDTH / 2, HIGHSCORE_TITLE_Y_MENU);

        // Render danh sách điểm cao
        renderTextureAt(highscoreListTexture, SCREEN_WIDTH / 2, HIGHSCORE_LIST_Y);

        // Render nút Back
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton);
        renderTextureCentered(backButtonTexture, backButton);
    }
    else if (gameState == SETTINGS) {
        // Render tiêu đề Settings
        renderTextureAt(settingsTitleTexture, SCREEN_WIDTH / 2, SETTINGS_TITLE_Y);

        // Render Volume slider
        renderTextureAt(volumeTexture, volumeSlider.x, VOLUME_LABEL_Y_SETTINGS, false); // Canh trái
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider);
        const SDL_Color& volKnobColor = isDraggingVolumeKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, volKnobColor.r, volKnobColor.g, volKnobColor.b, volKnobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob);

        // Render Sensitivity slider
        renderTextureAt(sensitivityTexture, sensitivitySlider.x, SENSITIVITY_LABEL_Y_SETTINGS, false); // Canh trái
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &sensitivitySlider);
         const SDL_Color& sensKnobColor = isDraggingSensitivityKnob ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, sensKnobColor.r, sensKnobColor.g, sensKnobColor.b, sensKnobColor.a);
        SDL_RenderFillRect(renderer, &sensitivityKnob);

        // Render nút Back
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backButton);
        renderTextureCentered(backButtonTexture, backButton);
    }
    // Các trạng thái khác (PLAYING, PAUSED, GAME_OVER) sẽ được render bởi lớp Game

    // Hiển thị lên màn hình
    SDL_RenderPresent(renderer);
}

// Áp dụng cài đặt từ Menu vào Game
void MainMenu::applySettingsToGame(Game& game) {
    game.setVolume(volume);
    game.setSensitivity(sensitivity);
}
