#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include "game.h"

int main(int argc, char* argv[]) {
    // Khởi tạo SDL, SDL_ttf, SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "Khởi tạo thất bại!" << std::endl;
        return -1;
    }

    // Tạo cửa sổ và renderer
    SDL_Window* window = SDL_CreateWindow("Just a chill game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // Tải texture cho missile
    SDL_Surface* missileSurface = IMG_Load("images/missile.png");
    SDL_Texture* missileTexture = SDL_CreateTextureFromSurface(renderer, missileSurface);
    SDL_FreeSurface(missileSurface);

    // Kiểm tra lỗi
    if (!window || !renderer || !missileTexture) {
        std::cerr << "Tạo tài nguyên thất bại!" << std::endl;
        return -1;
    }

    // Tạo đối tượng Game và chạy game
    Game game(renderer, missileTexture); // Xóa tham số font
    bool running = true;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        float deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
        lastTime = SDL_GetTicks();

        game.update(deltaTime);
        game.render();
    }

    // Giải phóng tài nguyên
    if (game.gameOverTexture) SDL_DestroyTexture(game.gameOverTexture);
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
