#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <string> 
#include "enemy.h"
#include "mainmenu.h"
#include "life.h"

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);

struct AllyShip {
    float x, y;         
    float speed;        
    bool active;        
    bool droppingHeal;  
};

struct HealItem {
    float x, y;      
    float speed;      
    bool active;     
};

class Game {
private:
    SDL_Renderer* renderer;
    Enemy* enemy;
    MainMenu* menu;

    SDL_Texture* mspaceshipTexture;
    SDL_Texture* pauseButtonTexture;
    SDL_Texture* scoreTexture;
    SDL_Texture* highscoreTexture;
    SDL_Texture* pausedTexture;
    SDL_Texture* backToMenuTexture;
    SDL_Texture* restartTexture;
    SDL_Texture* gameOverTextTexture;
    SDL_Texture* volumeLabelTexture;
    SDL_Texture* giveUpTexture;
    SDL_Texture* backgroundTexture; 
    SDL_Texture* allyShipTexture;  
    SDL_Texture* healItemTexture;  

    Mix_Chunk* sfxShieldHit;
    Mix_Chunk* sfxPlayerHit;
    Mix_Chunk* sfxGameOver;
    Mix_Chunk* sfxWarning;
    Mix_Chunk* sfxHealCollect;      
    Mix_Music* bgmGame;

    bool gameOver;
    bool paused;
    bool showWarning;
    bool justStarted;

    Uint32 startTime;
    Uint32 pauseStartTime;
    Uint32 totalPausedTime;
    Uint32 warningStartTime;
    Uint32 nextSpawnTime;
    Uint32 lastMissileSpawnTime;
    Uint32 lastAllySpawnTime;       

    int score;
    int missileCount;
    int waveCount;
    int wavesUntilIncrease;
    int spawnedMissilesInWave;

    int warningX, warningY;
    float arcStartAngle;

    int volume;
    int sensitivity;

    SDL_Rect chitbox;
    SDL_Rect pauseButton;
    SDL_Rect backToMenuButton;
    SDL_Rect restartButton;
    SDL_Rect giveUpButton;
    SDL_Rect volumeSlider;
    SDL_Rect volumeKnob;
    bool isDraggingVolume; 

    struct Circle { int x, y, r; };
    Circle trajectory;

    std::vector<Life> lives;
    std::vector<Target> targets;
    std::vector<Target> fastMissiles;
    std::vector<SpaceShark> spaceSharks;
    std::vector<SharkBullet> sharkBullets;
    std::vector<AllyShip> allies;      
    std::vector<HealItem> healItems; 

    void initTextures(); 
    void updateScoreTexture();
    void updateHighscoreTexture();
    void updatePausedTexture();
    void updateGameOverTextTexture();
    void updateVolumeLabelTexture();
    void DrawCircle(SDL_Renderer* renderer, const Circle& c);
    void DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle);

    bool CheckCollisionWithArc(const Target& t);
    bool CheckCollisionWithChitbox(const Target& t);
    bool CheckCollisionWithArc(const SpaceShark& ss);
    bool CheckCollisionWithChitbox(const SpaceShark& ss);
    bool CheckCollisionWithArc(const SharkBullet& sb);
    bool CheckCollisionWithChitbox(const SharkBullet& sb);
    bool CheckCollisionWithChitbox(const HealItem& hi);

    void HandleHit(); 
    void SpawnAlly(); 
    void HandleHealCollection(HealItem& heal); 


public:
    Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
         Mix_Chunk* sfxShieldHit, Mix_Chunk* sfxPlayerHit,
         Mix_Chunk* sfxGameOver, Mix_Chunk* sfxWarning,
         Mix_Chunk* sfxHealCollect, 
         Mix_Music* bgmGame,
         SDL_Texture* bgTexture);
    ~Game();

    void handleInput(SDL_Event& event);
    void update(float deltaTime);
    void render();
    void reset();
    void startGame();

    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return paused; }
    int getVolume() const { return volume; }
    void setVolume(int vol);
    int getSensitivity() const { return sensitivity; }
    void setSensitivity(int sens);
    bool isDraggingVolumeSlider() const; 

    void setGameStatePlaying();
    void setGameStatePaused();
    void triggerGameOver();

};

#endif 
