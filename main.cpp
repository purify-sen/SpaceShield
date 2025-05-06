#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <memory>
#include <string>
#include "config.h"
#include "game.h"
#include "mainmenu.h"
#include "enemy.h"

Mix_Chunk* loadSoundEffect(const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound effect! SDL_mixer Error: " << path << " - " << Mix_GetError() << std::endl;
    }
    return chunk;
}

Mix_Music* loadMusic(const std::string& path) {
    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if (!music) {
        std::cerr << "Failed to load music! SDL_mixer Error: " << path << " - " << Mix_GetError() << std::endl;
    }
    return music;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(AUDIO_FREQUENCY, MIX_DEFAULT_FORMAT, AUDIO_CHANNELS, AUDIO_CHUNK_SIZE) < 0) {
        std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_AllocateChannels(8);

    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    TTF_Font* mainFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    if (!mainFont) {
        std::cerr << "TTF_OpenFont failed for main font: " << FONT_PATH << " - " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }
    std::cout << "Successfully loaded main font: " << FONT_PATH << std::endl;

    SDL_Texture* missileTexture = loadTexture(renderer, IMG_MISSILE);
    if (!missileTexture) {
        std::cerr << "Error loading missile texture, exiting." << std::endl;
        TTF_CloseFont(mainFont); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Texture* mainMenuBgTexture = loadTexture(renderer, IMG_MAIN_MENU_BG);
    SDL_Texture* gameBgTexture = loadTexture(renderer, IMG_GAME_BG);
    if (!mainMenuBgTexture) { std::cerr << "Warning: Failed to load main menu background." << std::endl; }
    if (!gameBgTexture) { std::cerr << "Warning: Failed to load game background." << std::endl; }

    Mix_Chunk* sfxShieldHit = loadSoundEffect(SFX_SHIELD_HIT);
    Mix_Chunk* sfxPlayerHit = loadSoundEffect(SFX_PLAYER_HIT);
    Mix_Chunk* sfxButtonClick = loadSoundEffect(SFX_BUTTON_CLICK);
    Mix_Chunk* sfxGameOver = loadSoundEffect(SFX_GAME_OVER);
    Mix_Chunk* sfxWarning = loadSoundEffect(SFX_WARNING);
    Mix_Chunk* sfxHealCollect = loadSoundEffect(SFX_HEAL_COLLECT);
    Mix_Music* bgmMenu = loadMusic(BGM_MENU);
    Mix_Music* bgmGame = loadMusic(BGM_GAME);

    MainMenu menu(renderer, mainFont, sfxButtonClick, bgmMenu, mainMenuBgTexture);
    Enemy enemy(renderer, missileTexture);
    Game game(renderer, &enemy, &menu, sfxShieldHit, sfxPlayerHit, sfxGameOver, sfxWarning, sfxHealCollect, bgmGame, gameBgTexture);

    menu.applySettingsToGame(game);

    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    if (bgmMenu) {
        Mix_PlayMusic(bgmMenu, -1);
    } else {
        std::cerr << "Warning: Menu BGM not loaded, cannot play." << std::endl;
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            switch (menu.gameState) {
                case MainMenu::MENU:
                case MainMenu::HIGHSCORE:
                case MainMenu::SETTINGS:
                    menu.handleInput(event, running, game);
                    break;
                case MainMenu::PLAYING:
                    game.handleInput(event);
                    break;
                case MainMenu::PAUSED:
                    game.handleInput(event);
                    break;
                case MainMenu::GAME_OVER:
                    game.handleInput(event);
                    break;
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime > lastTime) ? (currentTime - lastTime) / 1000.0f : 0.0f;
        lastTime = currentTime;

        if (menu.gameState == MainMenu::PLAYING) {
            game.update(deltaTime);
            if (game.isGameOver()) {
                menu.gameState = MainMenu::GAME_OVER;
            }
        }

         switch (menu.gameState) {
            case MainMenu::MENU:
            case MainMenu::HIGHSCORE:
            case MainMenu::SETTINGS:
                menu.render();
                break;
            case MainMenu::PLAYING:
                game.render();
                break;
            case MainMenu::PAUSED:
                game.render();
                break;
            case MainMenu::GAME_OVER:
                game.render();
                break;
        }
    }

    Mix_FreeChunk(sfxShieldHit);
    Mix_FreeChunk(sfxPlayerHit);
    Mix_FreeChunk(sfxButtonClick);
    Mix_FreeChunk(sfxGameOver);
    Mix_FreeChunk(sfxWarning);
    Mix_FreeChunk(sfxHealCollect);
    Mix_FreeMusic(bgmMenu);
    Mix_FreeMusic(bgmGame);

    TTF_CloseFont(mainFont);
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyTexture(mainMenuBgTexture);
    SDL_DestroyTexture(gameBgTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}