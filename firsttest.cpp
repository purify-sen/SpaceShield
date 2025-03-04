#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>

#define PI M_PI //dinh nghia pi

struct Circle {
    int x, y;
    int r;
};

struct Line {
    int x1, y1;
    int x2, y2;
};

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

void DrawArc(SDL_Renderer* renderer, Circle &c, double startAngle, double arcAngle) { //ham ve vom
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    for (double angle = startAngle; angle <= startAngle + arcAngle; angle += 0.5) {
        double rad = angle * PI / 180.0;
        int x = c.x + c.r * cos(rad);
        int y = c.y + c.r * sin(rad);
        SDL_RenderDrawPoint(renderer, x, y);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Không thể khởi tạo. Lỗi: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Just a chill game",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          800, 600, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Không thể tạo cửa sổ. Lỗi: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Không thể tạo renderer! Lỗi: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Rect chitbox = { 390, 270, 20, 60 };
    Circle trajectory = { 400, 300, 50 };
    double arcStartAngle = -17.5;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_a) {
                    arcStartAngle -= 15; //nhan key a de xoay vom sang trai 15 do
                } else if (event.key.keysym.sym == SDLK_d) {
                    arcStartAngle += 15; //nhan key d de xoay vom sang phai 15 do
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &chitbox);
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        DrawCircle(renderer, trajectory);
        DrawArc(renderer, trajectory, arcStartAngle, 35);
        SDL_RenderPresent(renderer);
        SDL_Delay(6);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
