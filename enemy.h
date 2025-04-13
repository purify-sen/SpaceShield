#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>

struct Target {
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
    Enemy(SDL_Renderer* r, SDL_Texture* mt);
    ~Enemy();
    void renderTarget(Target& t);
    void renderFastMissile(Target& fm);
    void renderWarning(float warningX, float warningY, Uint32 warningStartTime, Uint32 startTime, Uint32 totalPausedTime);
};

#endif
