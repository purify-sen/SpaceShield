#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include "config.h"

struct Target {
    float x, y;
    float dx, dy;
    bool active;
};

struct SpaceShark {
    float x, y;
    float radius;
    float angle;
    float angularSpeed;
    Uint32 spawnTime;
    Uint32 lastBulletTime;
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

    void renderTarget(const Target& t);
    void renderFastMissile(const Target& fm);
    void renderWarning(float warningX, float warningY, Uint32 warningStartTime, Uint32 gameStartTime, Uint32 totalPausedTime);
    void renderSpaceShark(const SpaceShark& ss);
    void renderSharkBullet(const SharkBullet& sb);
};

#endif
