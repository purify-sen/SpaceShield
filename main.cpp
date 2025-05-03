#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <memory>
#include "config.h"
#include "game.h"
#include "mainmenu.h"
#include "enemy.h"

// Hàm trợ giúp để nạp âm thanh
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
    // Khởi tạo SDL và các thư viện con
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) { std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl; return 1; }
    if (TTF_Init() == -1) { std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl; SDL_Quit(); return 1; }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl; TTF_Quit(); SDL_Quit(); return 1; }
    if (Mix_OpenAudio(AUDIO_FREQUENCY, MIX_DEFAULT_FORMAT, AUDIO_CHANNELS, AUDIO_CHUNK_SIZE) < 0) { std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl; IMG_Quit(); TTF_Quit(); SDL_Quit(); return 1; }
    Mix_AllocateChannels(8);

    // Tạo cửa sổ và renderer
    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) { std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl; Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl; SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit(); return 1; }

    // --- Nạp tài nguyên chung ---
    // Font
    // *** FIX: Nạp font chính ở đây và kiểm tra lỗi kỹ ***
    TTF_Font* mainFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE); // Nạp font với cỡ chữ lớn mặc định cho menu
    if (!mainFont) {
        std::cerr << "TTF_OpenFont failed for main font: " << FONT_PATH << " - " << TTF_GetError() << std::endl;
        // Cleanup...
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1; // Thoát nếu không nạp được font
    }
    std::cout << "Successfully loaded main font: " << FONT_PATH << std::endl;


    // Texture tên lửa thường
    SDL_Surface* missileSurface = IMG_Load(IMG_MISSILE.c_str());
    if (!missileSurface) { std::cerr << "IMG_Load failed for " << IMG_MISSILE << ": " << IMG_GetError() << std::endl; TTF_CloseFont(mainFont); /*... cleanup ...*/ return 1; }
    SDL_Texture* missileTexture = SDL_CreateTextureFromSurface(renderer, missileSurface);
    SDL_FreeSurface(missileSurface);
    if (!missileTexture) { std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_MISSILE << ": " << SDL_GetError() << std::endl; TTF_CloseFont(mainFont); /*... cleanup ...*/ return 1; }

    // Nạp âm thanh yêu cầu
    Mix_Chunk* sfxShieldHit = loadSoundEffect(SFX_SHIELD_HIT);
    Mix_Chunk* sfxPlayerHit = loadSoundEffect(SFX_PLAYER_HIT);
    Mix_Chunk* sfxButtonClick = loadSoundEffect(SFX_BUTTON_CLICK);
    Mix_Chunk* sfxGameOver = loadSoundEffect(SFX_GAME_OVER);
    Mix_Chunk* sfxWarning = loadSoundEffect(SFX_WARNING);
    Mix_Music* bgmMenu = loadMusic(BGM_MENU);
    Mix_Music* bgmGame = loadMusic(BGM_GAME);

    // Tạo các đối tượng chính của game
    // *** Truyền mainFont đã được kiểm tra vào MainMenu ***
    MainMenu menu(renderer, mainFont, sfxButtonClick, bgmMenu);
    Enemy enemy(renderer, missileTexture);
    Game game(renderer, &enemy, &menu, sfxShieldHit, sfxPlayerHit, sfxGameOver, sfxWarning, bgmGame);

    menu.applySettingsToGame(game);

    // Vòng lặp chính của game
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    if (bgmMenu) {
        Mix_PlayMusic(bgmMenu, -1);
    }

    while (running) {
        // Xử lý sự kiện
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            // Chuyển sự kiện đến đối tượng phù hợp
            switch (menu.gameState) {
                case MainMenu::MENU:
                case MainMenu::HIGHSCORE:
                case MainMenu::SETTINGS:
                    menu.handleInput(event, running, game);
                    break;
                case MainMenu::PLAYING:
                case MainMenu::PAUSED:
                case MainMenu::GAME_OVER:
                    game.handleInput(event);
                    break;
            }
        }

        // Tính toán Delta Time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime > lastTime) ? (currentTime - lastTime) / 1000.0f : 0.0f;
        lastTime = currentTime;

        // Cập nhật trạng thái game
        if (menu.gameState == MainMenu::PLAYING) {
            game.update(deltaTime);
            if (game.isGameOver()) {
                menu.gameState = MainMenu::GAME_OVER;
            }
        }

        // Render
         switch (menu.gameState) {
            case MainMenu::MENU:
            case MainMenu::HIGHSCORE:
            case MainMenu::SETTINGS:
                menu.render();
                break;
            case MainMenu::PLAYING:
            case MainMenu::PAUSED:
            case MainMenu::GAME_OVER:
                game.render();
                break;
        }
    }

    // Dọn dẹp tài nguyên
    // Giải phóng âm thanh đã nạp
    Mix_FreeChunk(sfxShieldHit);
    Mix_FreeChunk(sfxPlayerHit);
    Mix_FreeChunk(sfxButtonClick);
    Mix_FreeChunk(sfxGameOver);
    Mix_FreeChunk(sfxWarning);
    Mix_FreeMusic(bgmMenu);
    Mix_FreeMusic(bgmGame);

    // Giải phóng các tài nguyên khác
    TTF_CloseFont(mainFont); // *** Giải phóng font chính ở đây ***
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
