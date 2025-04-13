#include "enemy.h"
#include <cmath>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <iostream>

#define PI M_PI

Enemy::Enemy(SDL_Renderer* r, SDL_Texture* mt) 
    : renderer(r), missileTexture(mt), fastMissileTexture(nullptr), warningTexture(nullptr) {
    SDL_Surface* fastMissileSurface = IMG_Load("images/fmissile.png");
    if (!fastMissileSurface) {
        std::cerr << "IMG_Load failed for fmissile.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    fastMissileTexture = SDL_CreateTextureFromSurface(renderer, fastMissileSurface);
    SDL_FreeSurface(fastMissileSurface);
    if (!fastMissileTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for fmissile.png: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* warningSurface = IMG_Load("images/fwarning.png");
    if (!warningSurface) {
        std::cerr << "IMG_Load failed for fwarning.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    warningTexture = SDL_CreateTextureFromSurface(renderer, warningSurface);
    SDL_FreeSurface(warningSurface);
    if (!warningTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for fwarning.png: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

Enemy::~Enemy() {
    if (fastMissileTexture) SDL_DestroyTexture(fastMissileTexture);
    if (warningTexture) SDL_DestroyTexture(warningTexture);
}

void Enemy::renderTarget(Target& t) {
    if (t.active && missileTexture) {
        double angle = atan2(t.dy, t.dx) * 180.0 / PI;
        SDL_Rect missileRect = {(int)t.x - 15, (int)t.y - 30, 45, 30};
        SDL_Point center = {15, 30};
        SDL_RenderCopyEx(renderer, missileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE);
    }
}

void Enemy::renderFastMissile(Target& fm) {
    if (fm.active && fastMissileTexture) {
        double angle = atan2(fm.dy, fm.dx) * 180.0 / PI;
        SDL_Rect missileRect = {(int)fm.x - 15, (int)fm.y - 30, 45, 30};
        SDL_Point center = {15, 30};
        SDL_RenderCopyEx(renderer, fastMissileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE);
    }
}

void Enemy::renderWarning(float warningX, float warningY, Uint32 warningStartTime, Uint32 startTime, Uint32 totalPausedTime) {
    if (warningTexture) {
        Uint32 elapsedTime = SDL_GetTicks() - startTime - totalPausedTime - warningStartTime;
        float alpha = 152.5f + 102.5f * sin(2 * PI * 2 * elapsedTime / 1000.0f);
        SDL_SetTextureAlphaMod(warningTexture, static_cast<Uint8>(alpha));

        SDL_Rect warningRect = {(int)warningX - 15, (int)warningY - 22, 30, 45};
        SDL_RenderCopy(renderer, warningTexture, NULL, &warningRect);

        SDL_SetTextureAlphaMod(warningTexture, 255);
    }
}
