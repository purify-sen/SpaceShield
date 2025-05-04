#ifndef MAINMENU_H
#define MAINMENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <string>
#include "config.h"

// Forward declaration để tránh phụ thuộc vòng tròn
class Game;

class MainMenu {
public:
    // --- Trạng thái của Game ---
    enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, HIGHSCORE, SETTINGS };
    GameState gameState; // Trạng thái hiện tại của game

    // --- Con trỏ và Font ---
    SDL_Renderer* renderer; // Con trỏ đến renderer chính
    TTF_Font* font;         // Font chính được truyền từ main.cpp

    // --- Textures cho các thành phần Menu ---
    SDL_Texture* titleTexture;
    SDL_Texture* playButtonTexture;
    SDL_Texture* highscoreButtonTexture;
    SDL_Texture* settingsButtonTexture;
    SDL_Texture* exitButtonTexture;
    SDL_Texture* highscoreTitleTexture;
    SDL_Texture* highscoreListTexture; // Texture hiển thị danh sách điểm cao
    SDL_Texture* settingsTitleTexture;
    SDL_Texture* backButtonTexture;
    SDL_Texture* volumeTexture;        // Texture chữ "Volume: xxx"
    SDL_Texture* sensitivityTexture;   // Texture chữ "Sensitivity: xxx"
    SDL_Texture* backgroundTexture;    // Texture nền cho menu

    // --- Âm thanh Menu ---
    Mix_Chunk* sfxButtonClick; // Âm thanh click nút
    Mix_Music* bgmMenu;        // Nhạc nền menu

    // --- Rects cho các nút và slider (Vị trí và kích thước) ---
    SDL_Rect playButton;
    SDL_Rect highscoreButton;
    SDL_Rect settingsButton;
    SDL_Rect exitButton;
    SDL_Rect backButton;
    SDL_Rect volumeSlider;      // Slider trong màn hình Settings
    SDL_Rect volumeKnob;        // Knob trong màn hình Settings
    SDL_Rect sensitivitySlider; // Slider trong màn hình Settings
    SDL_Rect sensitivityKnob;   // Knob trong màn hình Settings

    // --- Dữ liệu người chơi và cài đặt ---
    std::vector<int> highscores; // Danh sách điểm cao (tối đa 5)
    int volume;                  // Âm lượng hiện tại (0-100)
    int sensitivity;             // Độ nhạy hiện tại (0-100)

    // --- Trạng thái kéo thả slider ---
    bool isDraggingVolumeKnob;
    bool isDraggingSensitivityKnob;

    // --- Hàm khởi tạo và hủy ---
    // Constructor nhận renderer, font, âm thanh và texture nền
    MainMenu(SDL_Renderer* r, TTF_Font* f, Mix_Chunk* sfxClick, Mix_Music* bgm, SDL_Texture* bgTexture);
    ~MainMenu(); // Hàm hủy để giải phóng textures

    // --- Xử lý Input và Render ---
    void handleInput(SDL_Event& event, bool& running, Game& game); // Xử lý input cho menu
    void render(); // Render menu dựa trên gameState

    // --- Quản lý dữ liệu người chơi ---
    void loadHighscores();              // Nạp điểm cao từ file
    void saveHighscores(int score);     // Lưu điểm cao mới vào danh sách và file
    void updateHighscoreListTexture();  // Cập nhật texture hiển thị điểm cao

    // --- Quản lý cài đặt ---
    void loadSettings();                // Nạp cài đặt (volume, sensitivity) từ file
    void saveSettings();                // Lưu cài đặt hiện tại vào file
    void updateVolumeTexture();         // Cập nhật texture hiển thị volume
    void updateSensitivityTexture();    // Cập nhật texture hiển thị sensitivity
    void applySettingsToGame(Game& game); // Áp dụng cài đặt cho đối tượng Game
};

#endif // MAINMENU_H
