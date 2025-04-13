#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include "game.h"
#include "mainmenu.h"
#include "enemy.h"

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    srand(static_cast<unsigned int>(time(NULL)));

    SDL_Window* window = SDL_CreateWindow("Space Shield", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface* missileSurface = IMG_Load("images/missile.png");
    if (!missileSurface) {
        std::cerr << "IMG_Load failed for missile.png: " << IMG_GetError() << std::endl;
        return 1;
    }
    SDL_Texture* missileTexture = SDL_CreateTextureFromSurface(renderer, missileSurface);
    SDL_FreeSurface(missileSurface);
    if (!missileTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for missile.png: " << SDL_GetError() << std::endl;
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 36);
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << std::endl;
        return 1;
    }

    MainMenu menu(renderer, font);
    Enemy enemy(renderer, missileTexture);
    Game game(renderer, &enemy, &menu);

    // Đồng bộ âm lượng ban đầu
    game.setVolume(menu.volume);
    Mix_VolumeMusic(game.getVolume() * 128 / 100);

    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (menu.gameState == MainMenu::MENU || menu.gameState == MainMenu::HIGHSCORE || menu.gameState == MainMenu::SETTINGS) {
                menu.handleInput(event, running, game);
            }
            else if (menu.gameState == MainMenu::PLAYING || menu.gameState == MainMenu::PAUSED || menu.gameState == MainMenu::GAME_OVER) {
                game.handleInput(event, menu);
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (menu.gameState == MainMenu::PLAYING) {
            game.update(deltaTime);
            if (game.isGameOver()) {
                menu.gameState = MainMenu::GAME_OVER;
                menu.updateHighscoreListTexture();
            }
        }

        if (menu.gameState == MainMenu::MENU || menu.gameState == MainMenu::HIGHSCORE || menu.gameState == MainMenu::SETTINGS) {
            menu.render();
        }
        else if (menu.gameState == MainMenu::PLAYING || menu.gameState == MainMenu::PAUSED || menu.gameState == MainMenu::GAME_OVER) {
            game.render();
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
