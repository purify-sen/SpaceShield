#ifndef MAINMENU_H
#define MAINMENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h> // *** Thêm header mixer ***
#include <vector>
#include <string>
#include "config.h"

class Game;

class MainMenu {
public:
    // --- Trạng thái của Game ---
    enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, HIGHSCORE, SETTINGS };
    GameState gameState;

    // --- Con trỏ và Font ---
    SDL_Renderer* renderer;
    TTF_Font* font;

    // --- Textures cho các thành phần Menu ---
    SDL_Texture* titleTexture;
    SDL_Texture* playButtonTexture;
    SDL_Texture* highscoreButtonTexture;
    SDL_Texture* settingsButtonTexture;
    SDL_Texture* exitButtonTexture;
    SDL_Texture* highscoreTitleTexture;
    SDL_Texture* highscoreListTexture;
    SDL_Texture* settingsTitleTexture;
    SDL_Texture* backButtonTexture;
    SDL_Texture* volumeTexture;
    SDL_Texture* sensitivityTexture;

    // *** Thêm: Con trỏ đến âm thanh menu ***
    Mix_Chunk* sfxButtonClick;
    Mix_Music* bgmMenu;

    // --- Rects cho các nút và slider ---
    SDL_Rect playButton;
    SDL_Rect highscoreButton;
    SDL_Rect settingsButton;
    SDL_Rect exitButton;
    SDL_Rect backButton;
    SDL_Rect volumeSlider;
    SDL_Rect volumeKnob;
    SDL_Rect sensitivitySlider;
    SDL_Rect sensitivityKnob;

    // --- Dữ liệu người chơi và cài đặt ---
    std::vector<int> highscores;
    int volume;
    int sensitivity;

    // --- Trạng thái kéo thả slider ---
    bool isDraggingVolumeKnob;
    bool isDraggingSensitivityKnob;

    // *** Thay đổi: Thêm tham số âm thanh vào constructor ***
    MainMenu(SDL_Renderer* r, TTF_Font* f, Mix_Chunk* sfxClick, Mix_Music* bgm);
    ~MainMenu();

    // --- Xử lý Input và Render ---
    void handleInput(SDL_Event& event, bool& running, Game& game);
    void render();

    // --- Quản lý dữ liệu người chơi ---
    void loadHighscores();
    void saveHighscores(int score);
    void updateHighscoreListTexture();

    // --- Quản lý cài đặt ---
    void loadSettings();
    void saveSettings();
    void updateVolumeTexture();
    void updateSensitivityTexture();
    void applySettingsToGame(Game& game);
};

#endif // MAINMENU_H
