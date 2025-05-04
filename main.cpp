#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <memory> // Cho smart pointers (nếu muốn dùng sau này)
#include <string> // Cho std::string
#include "config.h"
#include "game.h" // Đảm bảo include game.h để biết khai báo loadTexture
#include "mainmenu.h"
#include "enemy.h"

// Hàm trợ giúp để nạp hiệu ứng âm thanh (.wav)
Mix_Chunk* loadSoundEffect(const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound effect! SDL_mixer Error: " << path << " - " << Mix_GetError() << std::endl;
    }
    return chunk;
}

// Hàm trợ giúp để nạp nhạc nền (.ogg, .mp3, .mod, ...)
Mix_Music* loadMusic(const std::string& path) {
    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if (!music) {
        std::cerr << "Failed to load music! SDL_mixer Error: " << path << " - " << Mix_GetError() << std::endl;
    }
    return music;
}

// --- XÓA ĐỊNH NGHĨA loadTexture Ở ĐÂY ---
/*
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    // ... nội dung hàm đã bị xóa ...
}
*/


int main(int argc, char* argv[]) {
    // --- Khởi tạo SDL và các thư viện con ---
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    // Khởi tạo SDL_image cho PNG và JPG (nếu cần)
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Khởi tạo SDL_mixer
    if (Mix_OpenAudio(AUDIO_FREQUENCY, MIX_DEFAULT_FORMAT, AUDIO_CHANNELS, AUDIO_CHUNK_SIZE) < 0) {
        std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Cấp phát kênh âm thanh cho SFX
    Mix_AllocateChannels(8); // Ví dụ: 8 kênh đồng thời

    // --- Tạo cửa sổ và renderer ---
    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // Bật VSync
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    // --- Nạp tài nguyên chung ---
    // Font chính
    TTF_Font* mainFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    if (!mainFont) {
        std::cerr << "TTF_OpenFont failed for main font: " << FONT_PATH << " - " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }
    std::cout << "Successfully loaded main font: " << FONT_PATH << std::endl;

    // Texture tên lửa thường (truyền cho Enemy)
    // Bây giờ gọi hàm loadTexture đã được khai báo trong game.h (và định nghĩa trong game.cpp)
    SDL_Texture* missileTexture = loadTexture(renderer, IMG_MISSILE);
    if (!missileTexture) {
        TTF_CloseFont(mainFont); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); Mix_CloseAudio(); IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    // Texture nền
    SDL_Texture* mainMenuBgTexture = loadTexture(renderer, IMG_MAIN_MENU_BG);
    SDL_Texture* gameBgTexture = loadTexture(renderer, IMG_GAME_BG);
    // Kiểm tra lỗi nạp nền (tùy chọn, có thể tiếp tục nếu không có nền)
    if (!mainMenuBgTexture) { std::cerr << "Warning: Failed to load main menu background." << std::endl; }
    if (!gameBgTexture) { std::cerr << "Warning: Failed to load game background." << std::endl; }


    // Nạp âm thanh
    Mix_Chunk* sfxShieldHit = loadSoundEffect(SFX_SHIELD_HIT);
    Mix_Chunk* sfxPlayerHit = loadSoundEffect(SFX_PLAYER_HIT);
    Mix_Chunk* sfxButtonClick = loadSoundEffect(SFX_BUTTON_CLICK);
    Mix_Chunk* sfxGameOver = loadSoundEffect(SFX_GAME_OVER);
    Mix_Chunk* sfxWarning = loadSoundEffect(SFX_WARNING);
    Mix_Music* bgmMenu = loadMusic(BGM_MENU);
    Mix_Music* bgmGame = loadMusic(BGM_GAME);
    // Nên kiểm tra xem các con trỏ âm thanh có bị null không

    // --- Tạo các đối tượng chính của game ---
    MainMenu menu(renderer, mainFont, sfxButtonClick, bgmMenu, mainMenuBgTexture); // Truyền nền menu
    Enemy enemy(renderer, missileTexture); // Truyền texture tên lửa
    Game game(renderer, &enemy, &menu, sfxShieldHit, sfxPlayerHit, sfxGameOver, sfxWarning, bgmGame, gameBgTexture); // Truyền nền game

    // Áp dụng cài đặt đã nạp (hoặc mặc định) từ menu vào game
    menu.applySettingsToGame(game);

    // --- Vòng lặp chính của game ---
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks(); // Thời điểm của frame trước

    // Bắt đầu phát nhạc menu
    if (bgmMenu) {
        Mix_PlayMusic(bgmMenu, -1); // Phát lặp lại vô hạn (-1)
    }

    while (running) {
        // --- Xử lý sự kiện ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            // Chuyển sự kiện đến đối tượng phù hợp dựa trên trạng thái game
            switch (menu.gameState) {
                case MainMenu::MENU:
                case MainMenu::HIGHSCORE:
                case MainMenu::SETTINGS:
                    menu.handleInput(event, running, game);
                    break;
                case MainMenu::PLAYING:
                case MainMenu::PAUSED:
                case MainMenu::GAME_OVER:
                    game.handleInput(event); // Game tự xử lý input của nó
                    break;
            }
        }

        // --- Tính toán Delta Time ---
        Uint32 currentTime = SDL_GetTicks();
        // deltaTime tính bằng giây, tránh chia cho 0
        float deltaTime = (currentTime > lastTime) ? (currentTime - lastTime) / 1000.0f : 0.0f;
        lastTime = currentTime;

        // --- Cập nhật trạng thái game ---
        // Chỉ cập nhật Game nếu đang ở trạng thái PLAYING
        if (menu.gameState == MainMenu::PLAYING) {
            game.update(deltaTime);
            // Kiểm tra nếu game vừa kết thúc trong lần update này
            if (game.isGameOver()) {
                menu.gameState = MainMenu::GAME_OVER; // Chuyển trạng thái
            }
        }

        // --- Render ---
        // Render đối tượng phù hợp dựa trên trạng thái
         switch (menu.gameState) {
            case MainMenu::MENU:
            case MainMenu::HIGHSCORE:
            case MainMenu::SETTINGS:
                menu.render(); // Render menu hoặc các màn hình con của nó
                break;
            case MainMenu::PLAYING:
            case MainMenu::PAUSED:
            case MainMenu::GAME_OVER:
                game.render(); // Render màn hình game (playing, paused, game over)
                break;
        }
    } // Kết thúc vòng lặp chính

    // --- Dọn dẹp tài nguyên ---
    // Giải phóng âm thanh
    Mix_FreeChunk(sfxShieldHit);
    Mix_FreeChunk(sfxPlayerHit);
    Mix_FreeChunk(sfxButtonClick);
    Mix_FreeChunk(sfxGameOver);
    Mix_FreeChunk(sfxWarning);
    Mix_FreeMusic(bgmMenu);
    Mix_FreeMusic(bgmGame);

    // Giải phóng các tài nguyên khác
    TTF_CloseFont(mainFont);
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyTexture(mainMenuBgTexture);
    SDL_DestroyTexture(gameBgTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Đóng các thư viện con và SDL
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
