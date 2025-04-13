#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include "enemy.h"
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
    SDL_Texture* volumeLabelTexture;
    SDL_Texture* giveUpTexture; // Texture cho nút Give Up
    bool gameOver;
    bool paused;
    bool showWarning;
    bool isFirstFastMissile;
    bool justStarted;
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
    int volume;
    int sensitivity; // Độ nhạy cho phím A/D
    SDL_Rect volumeSlider;
    SDL_Rect volumeKnob;
    SDL_Rect giveUpButton; // Nút Give Up trong Pause
    bool isDraggingVolume;

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
    std::vector<Target> targets;
    std::vector<Target> fastMissiles;
    std::vector<Life> lives;

    void initTextures();
    void updateScoreTexture();
    void updateHighscoreTexture();
    void updatePausedTexture();
    void updateGameOverTextTexture();
    void updateVolumeLabelTexture();
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
    bool isGameOver() const { return gameOver; }
    int getVolume() const { return volume; }
    void setVolume(int vol) {
        if (vol >= 0 && vol <= 100) {
            volume = vol;
            volumeKnob.x = volumeSlider.x + (volume * volumeSlider.w / 100) - volumeKnob.w / 2;
            Mix_VolumeMusic(volume * 128 / 100);
        }
    }
    int getSensitivity() const { return sensitivity; }
    void setSensitivity(int sens) {
        if (sens >= 0 && sens <= 100) {
            sensitivity = sens;
        }
    }
    void startGame() {
        startTime = SDL_GetTicks();
        justStarted = true;
    }
};

#endif
