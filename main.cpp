#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <algorithm>
#include <ctime>    // Cho time() để khởi tạo seed
#include <memory>   // Cho smart pointers (nếu dùng)
#include "config.h" // Bao gồm config.h
#include "game.h"
#include "mainmenu.h"
#include "enemy.h"

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
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
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

    // Khởi tạo seed cho hàm rand() cũ (nếu vẫn dùng) hoặc bộ sinh C++11 (đã làm trong game.cpp)
    // srand(static_cast<unsigned int>(time(NULL))); // Không cần nếu dùng <random>

    // --- Tạo cửa sổ và renderer ---
    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // Bật VSync
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // --- Nạp tài nguyên chung ---
    // Nạp font chính (quản lý ở đây, truyền vào MainMenu)
    TTF_Font* mainFont = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE); // Font chính cho menu
    if (!mainFont) {
        std::cerr << "TTF_OpenFont failed for main font: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Nạp texture tên lửa thường (quản lý ở đây, truyền vào Enemy)
    SDL_Surface* missileSurface = IMG_Load(IMG_MISSILE.c_str());
    if (!missileSurface) {
        std::cerr << "IMG_Load failed for " << IMG_MISSILE << ": " << IMG_GetError() << std::endl;
        // Cleanup...
        TTF_CloseFont(mainFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* missileTexture = SDL_CreateTextureFromSurface(renderer, missileSurface);
    SDL_FreeSurface(missileSurface); // Giải phóng surface ngay sau khi tạo texture
    if (!missileTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_MISSILE << ": " << SDL_GetError() << std::endl;
         // Cleanup...
        TTF_CloseFont(mainFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // --- Tạo các đối tượng chính của game ---
    // Sử dụng cấp phát động thông thường hoặc smart pointers
    MainMenu menu(renderer, mainFont); // Truyền font đã nạp
    Enemy enemy(renderer, missileTexture); // Truyền texture tên lửa
    Game game(renderer, &enemy, &menu); // Truyền con trỏ enemy và menu

    // Áp dụng cài đặt ban đầu từ menu (đã load) vào game
    menu.applySettingsToGame(game);

    // --- Vòng lặp chính của game ---
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks(); // Thời điểm của frame trước

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
                // Cập nhật texture điểm cao trong menu (nếu cần hiển thị ngay)
                // menu.updateHighscoreListTexture(); // Game đã tự cập nhật highscore texture của nó
            }
        }
        // Không cần update gì cho Menu hoặc Enemy ở đây (trừ khi có animation)

        // --- Render ---
        // Render đối tượng phù hợp dựa trên trạng thái
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

        // Không cần SDL_Delay nếu đã bật VSync
    }

    // --- Dọn dẹp tài nguyên ---
    TTF_CloseFont(mainFont); // Giải phóng font chính
    SDL_DestroyTexture(missileTexture); // Giải phóng texture tên lửa thường
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
