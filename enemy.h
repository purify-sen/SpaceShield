#ifndef ENEMY_H
#define ENEMY_H

enum LaserDirection { LEFT, RIGHT, UP, DOWN };

struct Target {
    float x, y;      // Tọa độ
    float dx, dy;    // Vận tốc
    bool active;     // Trạng thái hoạt động
};

struct Laser {
    float angle;          // Góc của laser (radian)
    Uint32 spawnTime;     // Thời gian sinh ra (ms)
    bool active;          // Trạng thái kích hoạt
    LaserDirection direction; // Hướng của laser
};

#endif
