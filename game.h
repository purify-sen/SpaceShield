#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "target.h"
#include "life.h"

#define PI M_PI

struct Circle { int x, y, r; };

class Game {
public:
    SDL_Renderer* renderer;
    SDL_Texture* missileTexture;
    SDL_Texture* gameOverTexture; // Vẫn giữ để lưu texture của hình ảnh Game Over
    Circle trajectory = {400, 300, 50};        // Vòng tròn quỹ đạo (nhân vật)
    SDL_Rect chitbox = {390, 270, 20, 60};     // Hitbox của nhân vật
    std::vector<Target> targets;               // Danh sách đạn địch
    std::vector<Life> lives = {{700, 550, false}, {730, 550, false}, {760, 550, false}}; // Mạng sống
    double arcStartAngle = -PI / 10.3;         // Góc bắt đầu của cung (radian)
    bool gameOver = false;
    Uint32 startTime;                          // Thời gian bắt đầu game
    int missileCount = 1;                      // Số lượng tên lửa mỗi đợt
    Uint32 nextSpawnTime = 2000;               // Mốc thời gian sinh tên lửa đầu tiên (2 giây)
    Uint32 lastMissileSpawnTime = 0;           // Thời gian sinh tên lửa cuối cùng trong dãy
    int spawnedInWave = 0;                     // Số tên lửa đã sinh trong đợt hiện tại
    float missileSpeed = 150.0f;               // Tốc độ ban đầu của tên lửa

    Game(SDL_Renderer* r, SDL_Texture* mt);    // Xóa tham số TTF_Font* vì không cần nữa
    void update(float deltaTime);
    void render();
    void reset();

private:
    bool CheckCollisionWithArc(Target& t);     // Kiểm tra va chạm với cung
    bool CheckCollisionWithChitbox(Target& t); // Kiểm tra va chạm với hitbox
    void DrawCircle(SDL_Renderer* renderer, Circle& c); // Vẽ vòng tròn
    void DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle); // Vẽ cung
};

#endif
