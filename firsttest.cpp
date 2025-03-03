#include <SDL2/SDL.h>
#include <iostream>

struct Circle {
    int x, y;  // toa do tam
    int r;     // ban kinh
};

struct Line {
    int x1,x2;
    int y1,y2;
}

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

void rolling(SDL_Renderer* renderer, Circle &c, int r) {

}


int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Không thể khởi tạo. Lỗi: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Just a chill game",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          800, 600, SDL_WINDOW_SHOWN); //kich thuoc cua so

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

    int xa, ya, xb,yb;
    SDL_Rect chitbox = { 390, 270, 20, 60 }; 
    Circle trajectory = { 400, 300, 50};
    Line arrow = { xa, ya, xb, yb}

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;  // theo doi event, tat cua so khi an x
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); 
        SDL_RenderFillRect(renderer, &chitbox);
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255); 
        int rotatex,rotatey;
        DrawCircle(renderer, trajectory);
        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
