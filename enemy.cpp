#include "enemy.h"
#include "config.h"
#include <cmath>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <iostream>

Enemy::Enemy(SDL_Renderer* r, SDL_Texture* mt)
    : renderer(r), missileTexture(mt), fastMissileTexture(nullptr), warningTexture(nullptr),
      spaceSharkTexture(nullptr), sharkBulletTexture(nullptr) {

    SDL_Surface* fastMissileSurface = IMG_Load(IMG_FAST_MISSILE.c_str());
    if (!fastMissileSurface) {
        std::cerr << "IMG_Load failed for " << IMG_FAST_MISSILE << ": " << IMG_GetError() << std::endl;
        exit(1);
    }
    fastMissileTexture = SDL_CreateTextureFromSurface(renderer, fastMissileSurface);
    SDL_FreeSurface(fastMissileSurface);
    if (!fastMissileTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_FAST_MISSILE << ": " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* warningSurface = IMG_Load(IMG_WARNING.c_str());
    if (!warningSurface) {
        std::cerr << "IMG_Load failed for " << IMG_WARNING << ": " << IMG_GetError() << std::endl;
        exit(1);
    }
    warningTexture = SDL_CreateTextureFromSurface(renderer, warningSurface);
    SDL_FreeSurface(warningSurface);
    if (!warningTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_WARNING << ": " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* spaceSharkSurface = IMG_Load(IMG_SPACE_SHARK.c_str());
    if (!spaceSharkSurface) {
        std::cerr << "IMG_Load failed for " << IMG_SPACE_SHARK << ": " << IMG_GetError() << std::endl;
        exit(1);
    }
    spaceSharkTexture = SDL_CreateTextureFromSurface(renderer, spaceSharkSurface);
    SDL_FreeSurface(spaceSharkSurface);
    if (!spaceSharkTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_SPACE_SHARK << ": " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* sharkBulletSurface = IMG_Load(IMG_SHARK_BULLET.c_str());
    if (!sharkBulletSurface) {
        std::cerr << "IMG_Load failed for " << IMG_SHARK_BULLET << ": " << IMG_GetError() << std::endl;
        exit(1);
    }
    sharkBulletTexture = SDL_CreateTextureFromSurface(renderer, sharkBulletSurface);
    SDL_FreeSurface(sharkBulletSurface);
    if (!sharkBulletTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_SHARK_BULLET << ": " << SDL_GetError() << std::endl;
        exit(1);
    }
}

Enemy::~Enemy() {
    if (fastMissileTexture) SDL_DestroyTexture(fastMissileTexture);
    if (warningTexture) SDL_DestroyTexture(warningTexture);
    if (spaceSharkTexture) SDL_DestroyTexture(spaceSharkTexture);
    if (sharkBulletTexture) SDL_DestroyTexture(sharkBulletTexture);
}

void Enemy::renderTarget(const Target& t) {
    if (t.active && missileTexture) {
        double angle = atan2(t.dy, t.dx) * 180.0 / PI;
        SDL_Rect missileRect = {(int)t.x - MISSILE_CENTER.x, (int)t.y - MISSILE_CENTER.y, MISSILE_WIDTH, MISSILE_HEIGHT};
        SDL_RenderCopyEx(renderer, missileTexture, NULL, &missileRect, angle, &MISSILE_CENTER, SDL_FLIP_NONE);
    }
}

void Enemy::renderFastMissile(const Target& fm) {
    if (fm.active && fastMissileTexture) {
        double angle = atan2(fm.dy, fm.dx) * 180.0 / PI;
        SDL_Rect missileRect = {(int)fm.x - FAST_MISSILE_CENTER.x, (int)fm.y - FAST_MISSILE_CENTER.y, FAST_MISSILE_WIDTH, FAST_MISSILE_HEIGHT};
        SDL_RenderCopyEx(renderer, fastMissileTexture, NULL, &missileRect, angle, &FAST_MISSILE_CENTER, SDL_FLIP_NONE);
    }
}

void Enemy::renderWarning(float warningX, float warningY, Uint32 warningStartTime, Uint32 gameStartTime, Uint32 totalPausedTime) {
    if (warningTexture) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsedTime = 0;
        if (currentTime > gameStartTime + totalPausedTime + warningStartTime) {
             elapsedTime = currentTime - gameStartTime - totalPausedTime - warningStartTime;
        }

        float alpha = WARNING_ALPHA_MIN + WARNING_ALPHA_RANGE * sin(WARNING_ALPHA_FREQ * elapsedTime);
        alpha = std::max(0.0f, std::min(255.0f, alpha));
        SDL_SetTextureAlphaMod(warningTexture, static_cast<Uint8>(alpha));

        SDL_Rect warningRect = {(int)warningX - WARNING_ICON_OFFSET_X, (int)warningY - WARNING_ICON_OFFSET_Y, WARNING_ICON_WIDTH, WARNING_ICON_HEIGHT};
        SDL_RenderCopy(renderer, warningTexture, NULL, &warningRect);

        SDL_SetTextureAlphaMod(warningTexture, 255);
    }
}

void Enemy::renderSpaceShark(const SpaceShark& ss) {
    if (ss.active && spaceSharkTexture) {
        float dr_dt = SHARK_SPIRAL_SPEED;
        float dx = dr_dt * cos(ss.angle) - ss.radius * sin(ss.angle) * ss.angularSpeed;
        float dy = dr_dt * sin(ss.angle) + ss.radius * cos(ss.angle) * ss.angularSpeed;

        double angle = atan2(dy, dx) * 180.0 / PI;
        SDL_Rect sharkRect = {(int)ss.x - SHARK_CENTER.x, (int)ss.y - SHARK_CENTER.y, SHARK_WIDTH, SHARK_HEIGHT};
        SDL_RenderCopyEx(renderer, spaceSharkTexture, NULL, &sharkRect, angle, &SHARK_CENTER, SDL_FLIP_NONE);
    }
}

void Enemy::renderSharkBullet(const SharkBullet& sb) {
    if (sb.active && sharkBulletTexture) {
        double angle = atan2(sb.dy, sb.dx) * 180.0 / PI;
        SDL_Rect bulletRect = {(int)sb.x - SHARK_BULLET_CENTER.x, (int)sb.y - SHARK_BULLET_CENTER.y, SHARK_BULLET_WIDTH, SHARK_BULLET_HEIGHT};
        SDL_RenderCopyEx(renderer, sharkBulletTexture, NULL, &bulletRect, angle, &SHARK_BULLET_CENTER, SDL_FLIP_NONE);
    }
}