#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>

struct Target {
    float x, y;
    float dx, dy;
    bool active;
};

struct SpaceShark {
    float x, y;           // Vị trí
    float radius;         // Bán kính xoắn ốc
    float angle;          // Góc xoắn ốc
    float angularSpeed;   // Tốc độ góc
    Uint32 spawnTime;     // Thời gian sinh
    Uint32 lastBulletTime;// Thời gian bắn đạn cuối
    bool active;
};

struct SharkBullet {
    float x, y;
    float dx, dy;
    bool active;
};

class Enemy {
public:
    SDL_Renderer* renderer;
    SDL_Texture* missileTexture;
    SDL_Texture* fastMissileTexture;
    SDL_Texture* warningTexture;
    SDL_Texture* spaceSharkTexture;
    SDL_Texture* sharkBulletTexture;
    Enemy(SDL_Renderer* r, SDL_Texture* mt);
    ~Enemy();
    void renderTarget(Target& t);
    void renderFastMissile(Target& fm);
    void renderWarning(float warningX, float warningY, Uint32 warningStartTime, Uint32 startTime, Uint32 totalPausedTime);
    void renderSpaceShark(SpaceShark& ss);
    void renderSharkBullet(SharkBullet& sb);
};

#endif
