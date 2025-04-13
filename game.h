#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "enemy.h"
#include "life.h"

#define PI M_PI

struct Circle { int x, y, r; };

class Game {
public:
    SDL_Renderer* renderer;
    SDL_Texture* missileTexture;
    SDL_Texture* fastMissileTexture;
    SDL_Texture* warningTexture;
    SDL_Texture* mspaceshipTexture;
    SDL_Texture* gameOverTexture;
    SDL_Texture* pauseTexture;
    SDL_Texture* pauseButtonTexture;
    SDL_Texture* scoreTexture;
    SDL_Texture* highscoreTexture;
    SDL_Texture* pausedTexture;
    SDL_Texture* volumeTexture;
    TTF_Font* font;
    Circle trajectory = {400, 300, 50};
    SDL_Rect chitbox = {390, 270, 20, 60};
    SDL_Rect pauseButton = {10, 10, 40, 40};
    SDL_Rect volumeSlider = {300, 400, 200, 10};
    SDL_Rect volumeKnob = {364, 395, 20, 20};
    std::vector<Target> targets;
    std::vector<Target> fastMissiles;
    std::vector<Life> lives = {{700, 550, false}, {730, 550, false}, {760, 550, false}};
    double arcStartAngle = -PI / 10.3;
    bool gameOver = false;
    bool paused = false;
    bool isDraggingKnob = false;
    bool showWarning = false;
    Uint32 warningStartTime = 0;
    // Lưu vị trí cảnh báo
    float warningX = 0;
    float warningY = 0;
    Uint32 startTime;
    int missileCount = 1;
    int waveCount = 0;
    Uint32 nextSpawnTime = 2000;
    Uint32 lastMissileSpawnTime = 0;
    int spawnedMissilesInWave = 0;
    int wavesUntilIncrease;
    float defaultMissileSpeed = 150.0f;
    float maxMissileSpeed = 150.0f;
    int score = 0;
    int highscore = 0;
    int volume = 64;
    const char* playerDataFile = "playerdata/playerdata";
    Uint32 pauseStartTime = 0;
    Uint32 totalPausedTime = 0;

    Game(SDL_Renderer* r, SDL_Texture* mt);
    ~Game();
    void handleInput(SDL_Event& event);
    void update(float deltaTime);
    void render();
    void reset();
    void updateScoreTexture();
    void updateHighscoreTexture();
    void updatePausedTexture();
    void updateVolumeTexture();
    void loadHighscore();
    void saveHighscore();

private:
    bool CheckCollisionWithArc(Target& t);
    bool CheckCollisionWithChitbox(Target& t);
    void DrawCircle(SDL_Renderer* renderer, Circle& c);
    void DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle);
};

#endif
