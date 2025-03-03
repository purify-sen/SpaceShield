#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

#define PI M_PI

struct Circle {
    int x, y;  // Tọa độ tâm
    int r;     // Bán kính
};

struct Arrow {
    int x, y;      // Tọa độ mũi tên
    double angle;  // Góc bắn
    double speed;  // Tốc độ di chuyển
    int size;      // Kích thước mũi tên (hình vuông)
};

// Vẽ đường tròn bằng thuật toán Midpoint Circle Algorithm
void DrawCircle(SDL_Renderer* renderer, Circle &c) {
    int dx = c.r - 1;
    int dy = 0;
    int err = dx - (c.r << 1);

    while (dx >= dy) {
        SDL_RenderDrawPoint(renderer, c.x + dx, c.y + dy);
        SDL_RenderDrawPoint(renderer, c.x - dx, c.y + dy);
        SDL_RenderDrawPoint(renderer, c.x + dx, c.y - dy);
        SDL_RenderDrawPoint(renderer, c.x - dx, c.y - dy);
        SDL_RenderDrawPoint(renderer, c.x + dy, c.y + dx);
        SDL_RenderDrawPoint(renderer, c.x - dy, c.y + dx);
        SDL_RenderDrawPoint(renderer, c.x + dy, c.y - dx);
        SDL_RenderDrawPoint(renderer, c.x - dy, c.y - dx);
        
        dy++;
        if (err <= 0) {
            err += 2 * dy + 1;
        }
        if (err > 0) {
            dx--;
            err -= 2 * dx + 1;
        }
    }
}

// Vẽ mũi tên hình vuông
void DrawArrow(SDL_Renderer* renderer, Arrow &arrow) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_Rect arrowRect = { arrow.x - arrow.size / 2, arrow.y - arrow.size / 2, arrow.size, arrow.size };
    SDL_RenderFillRect(renderer, &arrowRect);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Just a chill game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Circle trajectory = { 400, 300, 50 };  // Quỹ đạo hình tròn
    std::vector<Arrow> arrows;  // Danh sách mũi tên
    double arrowSpeed = 2.0;  // Tốc độ ban đầu của mũi tên
    Uint32 lastSpawnTime = SDL_GetTicks();  // Thời gian cuối cùng sinh ra mũi tên
    Uint32 lastSpeedIncreaseTime = SDL_GetTicks();  // Thời gian cuối cùng tăng tốc độ
    std::srand(std::time(nullptr));
    
    bool running = true;
    SDL_Event event;
    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Sinh ra 3 mũi tên mỗi 2 giây
        if (currentTime - lastSpawnTime >= 2000) {
            for (int i = 0; i < 3; ++i) {
                double angle = std::rand() % 360;  // Góc ngẫu nhiên
                double rad = angle * PI / 180.0;
                arrows.push_back({static_cast<int>(400 + 200 * cos(rad)), static_cast<int>(300 + 200 * sin(rad)), angle, arrowSpeed, 20});
            }
            lastSpawnTime = currentTime;
        }

        // Tăng tốc độ mũi tên 15% mỗi 2 giây
        if (currentTime - lastSpeedIncreaseTime >= 2000) {
            arrowSpeed *= 1.15;
            lastSpeedIncreaseTime = currentTime;
        }

        // Cập nhật vị trí mũi tên
        for (auto &arrow : arrows) {
            double rad = arrow.angle * PI / 180.0;
            arrow.x -= static_cast<int>(arrow.speed * cos(rad));
            arrow.y -= static_cast<int>(arrow.speed * sin(rad));
            
            // Nếu mũi tên chạm vào trung tâm, dừng trò chơi
            if (abs(arrow.x - 400) < 10 && abs(arrow.y - 300) < 10) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        DrawCircle(renderer, trajectory);

        // Vẽ các mũi tên
        for (auto &arrow : arrows) {
            DrawArrow(renderer, arrow);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // Giữ tốc độ 60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
