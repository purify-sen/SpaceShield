#include "game.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <SDL2/SDL_image.h>

Game::Game(SDL_Renderer* r, SDL_Texture* mt) 
    : renderer(r), missileTexture(mt), gameOverTexture(nullptr) {
    startTime = SDL_GetTicks();  // Lấy thời gian bắt đầu game
}

void Game::update(float deltaTime) {
    if (gameOver) {
        // Kiểm tra phím R để khởi động lại khi Game Over
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_R]) {
            reset();
        }
        return;
    }

    // Điều khiển cung (nhân vật)
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= 2 * PI * deltaTime;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += 2 * PI * deltaTime;

    // Sinh đạn địch theo thời gian
    Uint32 currentTime = SDL_GetTicks() - startTime; // Thời gian đã trôi qua
    if (currentTime >= nextSpawnTime && spawnedInWave < missileCount) {
        // Sinh từng tên lửa cách nhau 0.3 giây
        if (currentTime - lastMissileSpawnTime >= 300 || spawnedInWave == 0) { // 300 ms = 0.3 giây
            Target t;
            int side = rand() % 4;
            switch (side) {
                case 0: t.x = 0; t.y = rand() % 600; break;     // Từ trái
                case 1: t.x = 800; t.y = rand() % 600; break;   // Từ phải
                case 2: t.x = rand() % 800; t.y = 0; break;     // Từ trên
                case 3: t.x = rand() % 800; t.y = 600; break;   // Từ dưới
            }
            float distX = 400 - t.x, distY = 300 - t.y;
            float distance = sqrt(distX * distX + distY * distY);
            t.dx = distX / missileSpeed; // Dùng missileSpeed
            t.dy = distY / missileSpeed;
            t.active = true;
            targets.push_back(t);

            spawnedInWave++;           // Tăng số tên lửa đã sinh trong đợt
            lastMissileSpawnTime = currentTime; // Cập nhật thời gian sinh tên lửa cuối
        }
    }

    // Khi sinh đủ tên lửa trong đợt, chuyển sang đợt tiếp theo
    if (spawnedInWave >= missileCount && currentTime >= nextSpawnTime) {
        missileCount++;         // Tăng số lượng tên lửa cho đợt sau
        nextSpawnTime += 3000 + (rand() % 2001); // Ngẫu nhiên từ 3000 đến 5000 ms (3-5 giây)
        spawnedInWave = 0;      // Reset số tên lửa đã sinh cho đợt mới
    }

    // Cập nhật đạn địch
    for (auto& t : targets) {
        if (t.active) {
            t.x += t.dx * deltaTime * missileSpeed; // Dùng missileSpeed
            t.y += t.dy * deltaTime * missileSpeed;
            if (CheckCollisionWithArc(t)) t.active = false;
            else if (CheckCollisionWithChitbox(t)) {
                t.active = false;
                for (auto& life : lives) {
                    if (!life.isRed) { life.isRed = true; break; }
                }
                bool allRed = true;
                for (auto& life : lives) {
                    if (!life.isRed) { allRed = false; break; }
                }
                if (allRed) gameOver = true;
            }
        }
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ hitbox của nhân vật
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &chitbox);

    // Vẽ vòng tròn quỹ đạo và cung
    SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
    DrawCircle(renderer, trajectory);
    DrawArc(renderer, trajectory, arcStartAngle, 2 * PI / 3); // 120 độ

    // Vẽ đạn địch
    for (auto& t : targets) {
        if (t.active) {
            double angle = atan2(t.dy, t.dx) * 180.0 / PI;
            SDL_Rect missileRect = {(int)t.x - 5, (int)t.y - 5, 20, 40};
            SDL_Point center = {10, 20};
            SDL_RenderCopyEx(renderer, missileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE); // Sửa ¢er thành center
        }
    }

    // Vẽ mạng sống
    for (auto& life : lives) {
        Circle lifeCircle = {life.x, life.y, 10};
        SDL_SetRenderDrawColor(renderer, life.isRed ? 255 : 0, 0, life.isRed ? 0 : 255, 255);
        DrawCircle(renderer, lifeCircle);
    }

    // Vẽ hình ảnh "GAME OVER"
    if (gameOver && !gameOverTexture) {
        SDL_Surface* textSurface = IMG_Load("images/gameover.png");
        if (!textSurface) {
            std::cerr << "Không thể tải hình ảnh gameover.png: " << IMG_GetError() << std::endl;
            return;
        }
        gameOverTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
    }
    if (gameOver) {
        SDL_Rect textRect = {800 / 2 - 150, 600 / 2 - 150, 300, 300}; // Kích thước của hình ảnh
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &textRect);
    }

    SDL_RenderPresent(renderer);
}

void Game::reset() {
    // Khởi động lại các giá trị về trạng thái ban đầu
    gameOver = false;
    targets.clear();                // Xóa tất cả tên lửa
    for (auto& life : lives) {
        life.isRed = false;         // Reset mạng sống
    }
    missileCount = 1;               // Reset số lượng tên lửa mỗi đợt
    nextSpawnTime = 2000;           // Đặt lại mốc sinh đầu tiên sau 2 giây
    spawnedInWave = 0;              // Reset số tên lửa đã sinh trong đợt
    lastMissileSpawnTime = 0;       // Reset thời gian sinh tên lửa cuối
    startTime = SDL_GetTicks();     // Reset thời gian bắt đầu game
    arcStartAngle = -PI / 10.3;     // Reset góc cung
    if (gameOverTexture) {
        SDL_DestroyTexture(gameOverTexture); // Xóa texture Game Over cũ
        gameOverTexture = nullptr;
    }
}

bool Game::CheckCollisionWithArc(Target& t) {
    for (double angle = arcStartAngle; angle <= arcStartAngle + 2 * PI / 3; angle += 0.01) {
        int arcX = trajectory.x + trajectory.r * cos(angle);
        int arcY = trajectory.y + trajectory.r * sin(angle);
        int dist = (t.x - arcX) * (t.x - arcX) + (t.y - arcY) * (t.y - arcY);
        if (dist < 25) return true;
    }
    return false;
}

bool Game::CheckCollisionWithChitbox(Target& t) {
    return (t.x >= chitbox.x && t.x <= chitbox.x + chitbox.w &&
            t.y >= chitbox.y && t.y <= chitbox.y + chitbox.h);
}

void Game::DrawCircle(SDL_Renderer* renderer, Circle& c) {
    const int segments = 36;
    SDL_Point points[segments + 1];
    for (int i = 0; i <= segments; i++) {
        float rad = (2 * PI * i) / segments;
        points[i].x = c.x + c.r * cos(rad);
        points[i].y = c.y + c.r * sin(rad);
    }
    SDL_RenderDrawLines(renderer, points, segments + 1);
}

void Game::DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle) {
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    const int segments = 24;
    SDL_Point points[segments + 1];
    for (int i = 0; i <= segments; i++) {
        double angle = startAngle + (arcAngle * i / segments);
        points[i].x = c.x + c.r * cos(angle);
        points[i].y = c.y + c.r * sin(angle);
    }
    SDL_RenderDrawLines(renderer, points, segments + 1);
}
