#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include "enemy.h"
#include "mainmenu.h"
#include "life.h"

class Game {
private:
    // --- Pointers and References ---
    SDL_Renderer* renderer;
    Enemy* enemy;
    MainMenu* menu;

    // --- Textures ---
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

    // --- Sounds ---
    Mix_Chunk* sfxShieldHit;
    Mix_Chunk* sfxPlayerHit;
    Mix_Chunk* sfxGameOver;
    Mix_Chunk* sfxWarning;
    Mix_Music* bgmGame;

    // --- Game State ---
    bool gameOver;
    bool paused;
    bool showWarning;
    bool justStarted;

    // --- Timing ---
    Uint32 startTime;
    Uint32 pauseStartTime;
    Uint32 totalPausedTime;
    Uint32 warningStartTime;
    Uint32 nextSpawnTime;
    Uint32 lastMissileSpawnTime;

    // --- Score and Waves ---
    int score;
    int missileCount;
    int waveCount;
    int wavesUntilIncrease;
    int spawnedMissilesInWave;

    // --- Position and Angle ---
    int warningX, warningY;
    float arcStartAngle;

    // --- Settings (synced from MainMenu) ---
    int volume;
    int sensitivity;

    // --- UI Elements in Game ---
    SDL_Rect chitbox;
    SDL_Rect pauseButton;
    SDL_Rect backToMenuButton;
    SDL_Rect restartButton;
    SDL_Rect giveUpButton;
    SDL_Rect volumeSlider;
    SDL_Rect volumeKnob;
    bool isDraggingVolume;

    // --- Game Objects ---
    struct Circle { int x, y, r; };
    Circle trajectory;

    std::vector<Life> lives;
    std::vector<Target> targets;
    std::vector<Target> fastMissiles;
    std::vector<SpaceShark> spaceSharks;
    std::vector<SharkBullet> sharkBullets;

    // --- Internal Methods ---
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

    void HandleHit();

public:
    // Constructor updated to receive sound pointers (without explosion)
    Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
         Mix_Chunk* sfxShieldHit, Mix_Chunk* sfxPlayerHit,
         Mix_Chunk* sfxGameOver, Mix_Chunk* sfxWarning, Mix_Music* bgmGame);
    ~Game();

    void handleInput(SDL_Event& event);
    void update(float deltaTime);
    void render();
    void reset();
    void startGame();

    // --- Getters & Setters ---
    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return paused; }
    int getVolume() const { return volume; }
    void setVolume(int vol);
    int getSensitivity() const { return sensitivity; }
    // *** FIX: Only keep the declaration ***
    void setSensitivity(int sens); // Declaration only

    // Functions for MainMenu to manage game state
    void setGameStatePlaying();
    void setGameStatePaused();
    void triggerGameOver();

};

#endif // GAME_H
