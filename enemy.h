#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include "config.h" // Bao gồm config.h

// --- Định nghĩa cấu trúc cho các loại kẻ địch ---

// Tên lửa thường
struct Target {
    float x, y;     // Vị trí hiện tại
    float dx, dy;   // Vector vận tốc
    bool active;    // Trạng thái hoạt động
};

// Cá mập không gian
struct SpaceShark {
    float x, y;           // Vị trí hiện tại
    float radius;         // Bán kính hiện tại của quỹ đạo xoắn ốc
    float angle;          // Góc hiện tại trên quỹ đạo xoắn ốc
    float angularSpeed;   // Tốc độ góc (rad/giây)
    Uint32 spawnTime;     // Thời điểm xuất hiện (ms)
    Uint32 lastBulletTime;// Thời điểm bắn đạn cuối cùng (ms)
    bool active;          // Trạng thái hoạt động
};

// Đạn của cá mập
struct SharkBullet {
    float x, y;     // Vị trí hiện tại
    float dx, dy;   // Vector vận tốc
    bool active;    // Trạng thái hoạt động
};

// --- Lớp Enemy ---
// Quản lý textures và các hàm render cho kẻ địch
class Enemy {
public:
    // Con trỏ đến renderer chính
    SDL_Renderer* renderer;
    // Textures cho các loại kẻ địch và hiệu ứng
    SDL_Texture* missileTexture;     // Tên lửa thường
    SDL_Texture* fastMissileTexture; // Tên lửa nhanh
    SDL_Texture* warningTexture;     // Cảnh báo tên lửa nhanh
    SDL_Texture* spaceSharkTexture;  // Cá mập không gian
    SDL_Texture* sharkBulletTexture; // Đạn cá mập

    // Hàm khởi tạo: Nạp textures
    Enemy(SDL_Renderer* r, SDL_Texture* mt);
    // Hàm hủy: Giải phóng textures
    ~Enemy();

    // Hàm render cho từng loại đối tượng
    void renderTarget(const Target& t);
    void renderFastMissile(const Target& fm); // Tên lửa nhanh cũng dùng cấu trúc Target
    void renderWarning(float warningX, float warningY, Uint32 warningStartTime, Uint32 gameStartTime, Uint32 totalPausedTime);
    void renderSpaceShark(const SpaceShark& ss);
    void renderSharkBullet(const SharkBullet& sb);
};

#endif // ENEMY_H
