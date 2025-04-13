#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "enemy.h"  // Includes Target definition
#include "mainmenu.h"

#define PI 3.14159265358979323846

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
    SDL_Texture* volumeLabelTexture; // Texture cho nhãn "Volume"
    bool gameOver;
    bool paused;
    bool showWarning;
    bool isFirstFastMissile;
    Uint32 startTime;
    Uint32 pauseStartTime;
    Uint32 totalPausedTime;
    Uint32 warningStartTime;
    Uint32 nextSpawnTime;
    Uint32 lastMissileSpawnTime;
    int score;
    int warningX;
    int warningY;
    int missileCount;
    int waveCount;
    int wavesUntilIncrease;
    int spawnedMissilesInWave;
    float arcStartAngle;
    float defaultMissileSpeed;
    float maxMissileSpeed;
    int volume; // Giá trị âm lượng (0-128)
    SDL_Rect volumeSlider; // Thanh trượt âm lượng
    SDL_Rect volumeKnob; // Nút trượt trên thanh âm lượng
    bool isDraggingVolume; // Trạng thái kéo thanh trượt

    struct Circle {
        int x, y, r;
    };
    struct Life {
        int x, y;
        bool isRed;
    };

    Circle trajectory;
    SDL_Rect chitbox;
    SDL_Rect pauseButton;
    SDL_Rect backToMenuButton;
    SDL_Rect restartButton;
    std::vector<Target> targets;      // Use Target from enemy.h
    std::vector<Target> fastMissiles; // Use Target from enemy.h
    std::vector<Life> lives;

    void initTextures();
    void updateScoreTexture();
    void updateHighscoreTexture();
    void updatePausedTexture();
    void updateGameOverTextTexture();
    void updateVolumeLabelTexture(); // Hàm mới để tạo nhãn "Volume"
    void DrawCircle(SDL_Renderer* renderer, Circle& c);
    void DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle);
    bool CheckCollisionWithArc(Target& t);
    bool CheckCollisionWithChitbox(Target& t);

public:
    Game(SDL_Renderer* r, Enemy* e, MainMenu* m);
    ~Game();
    void handleInput(SDL_Event& event, MainMenu& menu);
    void update(float deltaTime);
    void render();
    void reset();
    bool isGameOver() const { return gameOver; } // Getter for gameOver
};

#endif
