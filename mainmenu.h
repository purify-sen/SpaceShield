#ifndef MAINMENU_H
#define MAINMENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>

class MainMenu {
public:
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* titleTexture;  // Tiêu đề "Space Shield"
    SDL_Texture* playButtonTexture;   // Nút Play
    SDL_Texture* highscoreButtonTexture;  // Nút Highscore
    SDL_Texture* settingsButtonTexture;   // Nút Settings
    SDL_Texture* exitButtonTexture;   // Nút Exit
    SDL_Texture* highscoreTitleTexture;  // Tiêu đề "Highscore"
    SDL_Texture* highscoreListTexture;   // Danh sách 5 highscore
    SDL_Texture* settingsTitleTexture;   // Tiêu đề "Settings"
    SDL_Texture* backButtonTexture;      // Nút Back (trở về menu)
    SDL_Texture* volumeTexture;          // Hiển thị giá trị âm lượng
    SDL_Rect playButton = {300, 250, 200, 50};      // Nút Play
    SDL_Rect highscoreButton = {300, 320, 200, 50}; // Nút Highscore
    SDL_Rect settingsButton = {300, 390, 200, 50};  // Nút Settings
    SDL_Rect exitButton = {300, 460, 200, 50};      // Nút Exit
    SDL_Rect backButton = {300, 500, 200, 50};      // Nút Back (cho Highscore/Settings)
    SDL_Rect volumeSlider = {300, 400, 200, 10};    // Thanh trượt âm lượng
    SDL_Rect volumeKnob = {364, 395, 20, 20};       // Núm điều chỉnh âm lượng
    std::vector<int> highscores;  // Lưu 5 điểm cao nhất
    int volume = 64;
    bool isDraggingKnob = false;
    const char* playerDataFile = "playerdata/playerdata";
    enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, HIGHSCORE, SETTINGS };
    GameState gameState = MENU;  // Trạng thái trò chơi

    MainMenu(SDL_Renderer* r, TTF_Font* f);
    ~MainMenu();
    void handleInput(SDL_Event& event, bool& running);
    void render();
    void loadHighscores();
    void saveHighscores(int score);
    void updateHighscoreListTexture();
    void updateVolumeTexture();
};

#endif
