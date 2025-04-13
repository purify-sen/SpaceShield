#ifndef ENEMY_H
#define ENEMY_H

struct Target {
    float x, y;
    float dx, dy;
    bool active;
    bool isFast = false; // Biến xác định Fast Missile
    bool warningActive = false; // Trạng thái cảnh báo
    Uint32 warningStartTime = 0; // Thời điểm bắt đầu cảnh báo
};

#endif
