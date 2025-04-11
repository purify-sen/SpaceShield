#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "enemy.h"  // Thay target.h thành enemy.h
#include "life.h"

#define PI M_PI

struct Circle { int x, y, r; };

class Game {
public:
    SDL_Renderer* renderer;
    SDL_Texture* missileTexture;
    SDL_Texture* mspaceshipTexture;
    SDL_Texture* gameOverTexture;
    SDL_Texture* pauseTexture;
    SDL_Texture* pauseButtonTexture;
    SDL_Texture* scoreTexture;
    SDL_Texture* highscoreTexture;
    TTF_Font* font;
    Circle trajectory = {400, 300, 50};
    SDL_Rect chitbox = {390, 270, 20, 60};
    SDL_Rect pauseButton = {10, 10, 40, 40};
    std::vector<Target> targets;
    std::vector<Laser> lasers;  // Vẫn dùng Laser, nhưng được định nghĩa trong enemy.h
    std::vector<Life> lives = {{700, 550, false}, {730, 550, false}, {760, 550, false}};
    double arcStartAngle = -PI / 10.3;
    bool gameOver = false;
    bool paused = false;
    Uint32 startTime;
    int missileCount = 1;
    int laserCount = 0;
    int waveCount = 0;
    Uint32 nextSpawnTime = 2000;
    Uint32 lastMissileSpawnTime = 0;
    Uint32 lastLaserSpawnTime = 0;
    int spawnedMissilesInWave = 0;
    int spawnedLasersInWave = 0;
    float defaultMissileSpeed = 150.0f;
    float maxMissileSpeed = 150.0f;
    int score = 0;
    int highscore = 0;
    const char* playerDataFile = "playerdata/playerdata";

    Game(SDL_Renderer* r, SDL_Texture* mt);
    ~Game();
    void handleInput(SDL_Event& event);
    void update(float deltaTime);
    void render();
    void reset();
    void updateScoreTexture();
    void updateHighscoreTexture();
    void loadHighscore();
    void saveHighscore();

private:
    bool CheckCollisionWithArc(Target& t);
    bool CheckCollisionWithChitbox(Target& t);
    bool CheckCollisionWithLaser(Laser& l);
    bool CheckCollisionArcWithLaser(Laser& l); // Đã có từ yêu cầu trước
    void DrawCircle(SDL_Renderer* renderer, Circle& c);
    void DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle);
    void DrawLaser(SDL_Renderer* renderer, Laser& l);
};

#endif
