#include "enemy.h"
#include <cmath>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <iostream>

#define PI M_PI

Enemy::Enemy(SDL_Renderer* r, SDL_Texture* mt) 
    : renderer(r), missileTexture(mt), fastMissileTexture(nullptr), warningTexture(nullptr),
      spaceSharkTexture(nullptr), sharkBulletTexture(nullptr) {
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

    SDL_Surface* spaceSharkSurface = IMG_Load("images/spaceshark.png");
    if (!spaceSharkSurface) {
        std::cerr << "IMG_Load failed for spaceshark.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    spaceSharkTexture = SDL_CreateTextureFromSurface(renderer, spaceSharkSurface);
    SDL_FreeSurface(spaceSharkSurface);
    if (!spaceSharkTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for spaceshark.png: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* sharkBulletSurface = IMG_Load("images/sharkbullet.png");
    if (!sharkBulletSurface) {
        std::cerr << "IMG_Load failed for sharkbullet.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    sharkBulletTexture = SDL_CreateTextureFromSurface(renderer, sharkBulletSurface);
    SDL_FreeSurface(sharkBulletSurface);
    if (!sharkBulletTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for sharkbullet.png: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

Enemy::~Enemy() {
    if (fastMissileTexture) SDL_DestroyTexture(fastMissileTexture);
    if (warningTexture) SDL_DestroyTexture(warningTexture);
    if (spaceSharkTexture) SDL_DestroyTexture(spaceSharkTexture);
    if (sharkBulletTexture) SDL_DestroyTexture(sharkBulletTexture);
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

void Enemy::renderSpaceShark(SpaceShark& ss) {
    if (ss.active && spaceSharkTexture) {
        // Tính vector vận tốc
        // dx/dt = -radius * sin(angle) * angularSpeed - cos(angle) * dr/dt
        // dy/dt = radius * cos(angle) * angularSpeed - sin(angle) * dr/dt
        // dr/dt = -20.0f (từ Game::update)
        float dr_dt = -20.0f;
        float dx = -ss.radius * sin(ss.angle) * ss.angularSpeed - cos(ss.angle) * dr_dt;
        float dy = ss.radius * cos(ss.angle) * ss.angularSpeed - sin(ss.angle) * dr_dt;

        // Tính góc xoay từ vector vận tốc
        double angle = atan2(dy, dx) * 180.0 / PI;
        SDL_Rect sharkRect = {(int)ss.x - 25, (int)ss.y - 15, 50, 30};
        SDL_Point center = {25, 15};
        SDL_RenderCopyEx(renderer, spaceSharkTexture, NULL, &sharkRect, angle, &center, SDL_FLIP_NONE);
    }
}

void Enemy::renderSharkBullet(SharkBullet& sb) {
    if (sb.active && sharkBulletTexture) {
        double angle = atan2(sb.dy, sb.dx) * 180.0 / PI;
        SDL_Rect bulletRect = {(int)sb.x - 10, (int)sb.y - 5, 20, 10};
        SDL_Point center = {10, 5};
        SDL_RenderCopyEx(renderer, sharkBulletTexture, NULL, &bulletRect, angle, &center, SDL_FLIP_NONE);
    }
}