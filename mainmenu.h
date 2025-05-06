#ifndef MAINMENU_H
#define MAINMENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <string>
#include "config.h"

class Game;

class MainMenu {
public:
    enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, HIGHSCORE, SETTINGS };
    GameState gameState; 

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
    SDL_Texture* backgroundTexture;    


    Mix_Chunk* sfxButtonClick; 
    Mix_Music* bgmMenu;       

    SDL_Rect playButton;
    SDL_Rect highscoreButton;
    SDL_Rect settingsButton;
    SDL_Rect exitButton;
    SDL_Rect backButton;
    SDL_Rect volumeSlider;      
    SDL_Rect volumeKnob;       
    SDL_Rect sensitivitySlider; 
    SDL_Rect sensitivityKnob;   

    std::vector<int> highscores; 
    int volume;                  
    int sensitivity;            

    bool isDraggingVolumeKnob;
    bool isDraggingSensitivityKnob;

    MainMenu(SDL_Renderer* r, TTF_Font* f, Mix_Chunk* sfxClick, Mix_Music* bgm, SDL_Texture* bgTexture);
    ~MainMenu(); 

    void handleInput(SDL_Event& event, bool& running, Game& game); 
    void render(); 

    void loadHighscores();              
    void saveHighscores(int score);   
    void updateHighscoreListTexture();  

    void loadSettings();            
    void saveSettings();               
    void updateVolumeTexture();     
    void updateSensitivityTexture();    
    void applySettingsToGame(Game& game); 
};

#endif 
