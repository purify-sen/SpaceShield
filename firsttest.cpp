#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Thêm SDL_ttf để hiển thị chữ
#include <iostream>
#include <cmath>
#include <vector>

#define PI M_PI

struct Circle {
    int x, y;
    int r;
};

struct Target {
    float x, y;
    float dx, dy;
    bool active;
};

// Cấu trúc cho chấm tròn (lives)
struct Life {
    int x, y;
    bool isRed; // Trạng thái màu: false = xanh dương, true = đỏ
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

void DrawArc(SDL_Renderer* renderer, Circle &c, double startAngle, double arcAngle) {
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    for (double angle = startAngle; angle <= startAngle + arcAngle; angle += 0.5) {
        double rad = angle * PI / 180.0;
        int x = c.x + c.r * cos(rad);
        int y = c.y + c.r * sin(rad);
        SDL_RenderDrawPoint(renderer, x, y);
    }
}

bool CheckCollisionWithArc(Target &t, Circle &c, double startAngle, double arcAngle) {
    for (double angle = startAngle; angle <= startAngle + arcAngle; angle += 0.5) {
        double rad = angle * PI / 180.0;
        int arcX = c.x + c.r * cos(rad);
        int arcY = c.y + c.r * sin(rad);
        int dist = (t.x - arcX) * (t.x - arcX) + (t.y - arcY) * (t.y - arcY);
        if (dist < 25) {
            return true;
        }
    }
    return false;
}

// Kiểm tra va chạm với chitbox
bool CheckCollisionWithChitbox(Target &t, SDL_Rect &chitbox) {
    return (t.x >= chitbox.x && t.x <= chitbox.x + chitbox.w &&
            t.y >= chitbox.y && t.y <= chitbox.y + chitbox.h);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Không thể khởi tạo SDL. Lỗi: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() < 0) { // Khởi tạo SDL_ttf
        std::cerr << "Không thể khởi tạo TTF. Lỗi: " << TTF_GetError() << std::endl;
        SDL_Quit();
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

    // Tải font từ thư mục font/font.ttf
    TTF_Font* font = TTF_OpenFont("font/font.ttf", 48); // Kích thước chữ 48
    if (!font) {
        std::cerr << "Không thể tải font! Lỗi: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Rect chitbox = { 390, 270, 20, 60 };
    Circle trajectory = { 400, 300, 50 };
    double arcStartAngle = -17.5;
    std::vector<Target> targets;
    int spawnTimer = 0;

    // Khởi tạo 3 chấm tròn (lives) ở dưới bên phải
    std::vector<Life> lives = {
        { 700, 550, false }, // Chấm 1
        { 730, 550, false }, // Chấm 2
        { 760, 550, false }  // Chấm 3
    };
    bool gameOver = false; // Trạng thái trò chơi kết thúc

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && !gameOver) { // Chỉ xử lý phím khi chưa game over
                if (event.key.keysym.sym == SDLK_a) {
                    arcStartAngle -= 15;
                } else if (event.key.keysym.sym == SDLK_d) {
                    arcStartAngle += 15;
                }
            }
        }

        if (!gameOver) {
            // Tạo mục tiêu mới mỗi 2 giây
            spawnTimer++;
            if (spawnTimer >= 120) {
                Target newTarget;
                int side = rand() % 4;
                switch (side) {
                    case 0: newTarget.x = 0; newTarget.y = rand() % 600; break;
                    case 1: newTarget.x = 800; newTarget.y = rand() % 600; break;
                    case 2: newTarget.x = rand() % 800; newTarget.y = 0; break;
                    case 3: newTarget.x = rand() % 800; newTarget.y = 600; break;
                }
                float distX = 400 - newTarget.x;
                float distY = 300 - newTarget.y;
                float distance = sqrt(distX * distX + distY * distY);
                newTarget.dx = distX / 300.0f;
                newTarget.dy = distY / 300.0f;
                newTarget.active = true;
                targets.push_back(newTarget);
                spawnTimer = 0;
            }

            // Cập nhật vị trí mục tiêu và kiểm tra va chạm
            for (auto &t : targets) {
                if (t.active) {
                    t.x += t.dx;
                    t.y += t.dy;
                    if (CheckCollisionWithArc(t, trajectory, arcStartAngle, 120)) {
                        t.active = false; // Chạm cung thì biến mất
                    } else if (CheckCollisionWithChitbox(t, chitbox)) { // Chạm chitbox
                        t.active = false;
                        for (auto &life : lives) {
                            if (!life.isRed) { // Tìm chấm xanh đầu tiên và đổi thành đỏ
                                life.isRed = true;
                                break;
                            }
                        }
                        // Kiểm tra nếu cả 3 chấm đều đỏ
                        bool allRed = true;
                        for (auto &life : lives) {
                            if (!life.isRed) allRed = false;
                        }
                        if (allRed) gameOver = true; // Kết thúc trò chơi nếu cả 3 đỏ và bị chạm tiếp
                    }
                    float distToCenter = (t.x - 400) * (t.x - 400) + (t.y - 300) * (t.y - 300);
                    if (distToCenter < 25) {
                        t.active = false;
                    }
                }
            }
        }

        // Vẽ màn hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &chitbox);

        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        DrawCircle(renderer, trajectory);

        DrawArc(renderer, trajectory, arcStartAngle, 120);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (auto &t : targets) {
            if (t.active) {
                SDL_Rect targetRect = { (int)t.x - 5, (int)t.y - 5, 10, 10 };
                SDL_RenderFillRect(renderer, &targetRect);
            }
        }

        // Vẽ 3 chấm tròn (lives)
        for (auto &life : lives) {
            Circle lifeCircle = { life.x, life.y, 10 };
            SDL_SetRenderDrawColor(renderer, life.isRed ? 255 : 0, 0, life.isRed ? 0 : 255, 255);
            DrawCircle(renderer, lifeCircle);
        }

        // Hiển thị GAME OVER nếu trò chơi kết thúc
        if (gameOver) {
            SDL_Color orange = { 255, 165, 0, 255 }; // Màu cam
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, "GAME OVER", orange);
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = { 800 / 2 - textSurface->w / 2, 600 / 2 - textSurface->h / 2, textSurface->w, textSurface->h };
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(6);
    }

    TTF_CloseFont(font); // Giải phóng font
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}