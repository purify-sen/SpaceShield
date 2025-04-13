#ifndef MAINMENU_H
#define MAINMENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>

class Game;

class MainMenu {
public:
    SDL_Renderer* renderer;
    TTF_Font* font;
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
    SDL_Rect playButton = {300, 250, 200, 50};
    SDL_Rect highscoreButton = {300, 320, 200, 50};
    SDL_Rect settingsButton = {300, 390, 200, 50};
    SDL_Rect exitButton = {300, 460, 200, 50};
    SDL_Rect backButton = {300, 500, 200, 50};
    SDL_Rect volumeSlider = {300, 420, 200, 10};
    SDL_Rect volumeKnob = {300 + (100 * 200 / 100) - 5, 415, 10, 20};
    SDL_Rect sensitivitySlider = {300, 470, 200, 10};
    SDL_Rect sensitivityKnob = {300 + (50 * 200 / 100) - 5, 465, 10, 20};
    std::vector<int> highscores;
    int volume = 100;
    int sensitivity = 50;
    bool isDraggingKnob = false;
    bool isDraggingSensitivityKnob = false;
    const char* playerDataFile = "playerdata/playerdata";
    enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, HIGHSCORE, SETTINGS };
    GameState gameState = MENU;

    MainMenu(SDL_Renderer* r, TTF_Font* f);
    ~MainMenu();
    void handleInput(SDL_Event& event, bool& running, Game& game);
    void render();
    void loadHighscores();
    void saveHighscores(int score);
    void updateHighscoreListTexture();
    void updateVolumeTexture();
    void updateSensitivityTexture();
};

#endif
