#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include "game.h"

int main(int argc, char* argv[]) {
    // Khởi tạo SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Khởi tạo SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Khởi tạo SDL_image
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Tạo cửa sổ
    SDL_Window* window = SDL_CreateWindow("Space Shield", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Tạo renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Tải texture cho missile
    SDL_Surface* missileSurface = IMG_Load("images/missile.png");
    if (!missileSurface) {
        std::cerr << "IMG_Load failed for missile.png: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    SDL_Texture* missileTexture = SDL_CreateTextureFromSurface(renderer, missileSurface);
    SDL_FreeSurface(missileSurface);
    if (!missileTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Tạo đối tượng Game
    Game game(renderer, missileTexture);
    bool running = true;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            game.handleInput(event); // Xử lý input
        }

        float deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
        lastTime = SDL_GetTicks();

        game.update(deltaTime);
        game.render();

        SDL_Delay(16); // Giới hạn 60 FPS
    }

    // Giải phóng tài nguyên
    if (game.gameOverTexture) SDL_DestroyTexture(game.gameOverTexture);
    if (game.pauseTexture) SDL_DestroyTexture(game.pauseTexture);
    if (game.pauseButtonTexture) SDL_DestroyTexture(game.pauseButtonTexture); // Giải phóng pauseButtonTexture
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
