#include "game.h"
#include "mainmenu.h" // Cần thiết để gọi saveHighscores, truy cập gameState
#include "config.h"   // Bao gồm config.h
#include <cmath>
#include <cstdlib>    // Cho rand() và RAND_MAX
#include <iostream>
#include <sstream>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <random> // Sử dụng bộ sinh số ngẫu nhiên tốt hơn

// --- Bộ sinh số ngẫu nhiên C++11 ---
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0); // Phân bố đều từ 0.0 đến 1.0
std::uniform_int_distribution<> dist_wave_increase(0, RANDOM_WAVES_UNTIL_INCREASE -1);
std::uniform_int_distribution<> dist_wave_delay(0, RANDOM_WAVE_DELAY -1);
std::uniform_int_distribution<> dist_side(0, 3); // 0: left, 1: right, 2: top, 3: bottom
std::uniform_int_distribution<> dist_y_spawn(0, SCREEN_HEIGHT - 1);
std::uniform_int_distribution<> dist_x_spawn(0, SCREEN_WIDTH - 1);


// Hàm khởi tạo Game
Game::Game(SDL_Renderer* r, Enemy* e, MainMenu* m)
    : renderer(r), enemy(e), menu(m),
      mspaceshipTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr),
      highscoreTexture(nullptr), pausedTexture(nullptr), backToMenuTexture(nullptr),
      restartTexture(nullptr), gameOverTextTexture(nullptr), volumeLabelTexture(nullptr),
      giveUpTexture(nullptr),
      gameOver(false), paused(false), showWarning(false),
      isFirstFastMissile(true), justStarted(false), // isFirstFastMissile có thể không cần nữa
      startTime(0), pauseStartTime(0), totalPausedTime(0), warningStartTime(0),
      score(0),
      warningX(0), warningY(0),
      volume(DEFAULT_VOLUME), sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)), isDraggingVolume(false),
      missileCount(INITIAL_MISSILE_COUNT), waveCount(0),
      nextSpawnTime(INITIAL_SPAWN_DELAY), lastMissileSpawnTime(0), spawnedMissilesInWave(0),
      arcStartAngle(INITIAL_SHIELD_START_ANGLE),
      // Khởi tạo Rects từ config.h
      chitbox(PLAYER_CHITBOX),
      pauseButton(PAUSE_BUTTON_RECT),
      backToMenuButton(BACK_TO_MENU_BUTTON_RECT_GAMEOVER),
      restartButton(RESTART_BUTTON_RECT),
      giveUpButton(GIVE_UP_BUTTON_RECT),
      volumeSlider(VOLUME_SLIDER_RECT), // Dùng rect cho màn hình pause
      volumeKnob(VOLUME_KNOB_RECT),     // Dùng rect cho màn hình pause
      trajectory{TRAJECTORY_CENTER.x, TRAJECTORY_CENTER.y, TRAJECTORY_RADIUS}
{
    // Tính số wave ngẫu nhiên để tăng độ khó
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);

    // Khởi tạo mạng sống
    lives.clear(); // Đảm bảo vector rỗng trước khi thêm
    for (int i = 0; i < PLAYER_LIVES; ++i) {
        Life life = {LIFE_ICON_START_X + i * LIFE_ICON_SPACING, LIFE_ICON_START_Y, false};
        lives.push_back(life);
    }

    // Đồng bộ âm lượng và độ nhạy ban đầu từ menu (nếu menu đã load settings)
    setVolume(menu->volume); // Sử dụng setter để cập nhật knob
    setSensitivity(menu->sensitivity);

    // Nạp texture tàu người chơi
    SDL_Surface* spaceshipSurface = IMG_Load(IMG_SPACESHIP.c_str());
    if (!spaceshipSurface) {
        std::cerr << "IMG_Load failed for " << IMG_SPACESHIP << ": " << IMG_GetError() << std::endl;
        exit(1); // Hoặc xử lý lỗi khác
    }
    mspaceshipTexture = SDL_CreateTextureFromSurface(renderer, spaceshipSurface);
    SDL_FreeSurface(spaceshipSurface);
    if (!mspaceshipTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_SPACESHIP << ": " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Nạp texture nút pause
    SDL_Surface* pauseButtonSurface = IMG_Load(IMG_PAUSE_BUTTON.c_str());
    if (!pauseButtonSurface) {
        std::cerr << "IMG_Load failed for " << IMG_PAUSE_BUTTON << ": " << IMG_GetError() << std::endl;
        exit(1);
    }
    pauseButtonTexture = SDL_CreateTextureFromSurface(renderer, pauseButtonSurface);
    SDL_FreeSurface(pauseButtonSurface);
    if (!pauseButtonTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for " << IMG_PAUSE_BUTTON << ": " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Khởi tạo các texture chữ (sẽ được tạo trong initTextures)
    initTextures(); // Tạo texture cho nút và text ban đầu
    updateScoreTexture(); // Cập nhật điểm ban đầu (0)
    updateHighscoreTexture(); // Cập nhật điểm cao ban đầu
    updatePausedTexture();
    updateGameOverTextTexture();
    updateVolumeLabelTexture();
}

// Hàm hủy Game
Game::~Game() {
    // Giải phóng textures đã tạo trong Game
    if (mspaceshipTexture) SDL_DestroyTexture(mspaceshipTexture);
    if (pauseButtonTexture) SDL_DestroyTexture(pauseButtonTexture);
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    if (pausedTexture) SDL_DestroyTexture(pausedTexture);
    if (backToMenuTexture) SDL_DestroyTexture(backToMenuTexture);
    if (restartTexture) SDL_DestroyTexture(restartTexture);
    if (gameOverTextTexture) SDL_DestroyTexture(gameOverTextTexture);
    if (volumeLabelTexture) SDL_DestroyTexture(volumeLabelTexture);
    if (giveUpTexture) SDL_DestroyTexture(giveUpTexture);
    // Lưu ý: enemy textures được quản lý trong lớp Enemy
    // Lưu ý: font được quản lý trong MainMenu hoặc main.cpp
}

// Khởi tạo / Tái tạo các texture chữ
void Game::initTextures() {
    TTF_Font* fontLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    TTF_Font* fontXLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_XLARGE);
    TTF_Font* fontNormal = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL); // Font cho volume label

    if (!fontLarge || !fontXLarge || !fontNormal) {
        std::cerr << "TTF_OpenFont failed in Game::initTextures: " << TTF_GetError() << std::endl;
        // Đóng font nào đã mở được
        if(fontLarge) TTF_CloseFont(fontLarge);
        if(fontXLarge) TTF_CloseFont(fontXLarge);
        if(fontNormal) TTF_CloseFont(fontNormal);
        return; // Không thể tạo texture nếu không có font
    }

    // Hàm trợ giúp tạo texture từ text
    auto createTexture = [&](const char* text, SDL_Texture*& texture, TTF_Font* font) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, TEXT_COLOR);
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
            return false;
        }
        if (texture) SDL_DestroyTexture(texture); // Hủy texture cũ nếu có
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    };

    // Tạo textures cho các nút và tiêu đề
    createTexture("Back to Menu", backToMenuTexture, fontLarge);
    createTexture("Restart", restartTexture, fontLarge);
    createTexture("Give Up", giveUpTexture, fontLarge);
    createTexture("Paused", pausedTexture, fontLarge); // Sử dụng font lớn cho "Paused"
    createTexture("Game over", gameOverTextTexture, fontXLarge);
    createTexture("Volume", volumeLabelTexture, fontNormal); // Font thường cho "Volume"

    // Đóng fonts
    TTF_CloseFont(fontLarge);
    TTF_CloseFont(fontXLarge);
    TTF_CloseFont(fontNormal);
}

// Cập nhật texture điểm số
void Game::updateScoreTexture() {
    std::stringstream ss;
    ss << "Score: " << score;
    std::string scoreStr = ss.str();

    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    if (!font) {
        std::cerr << "TTF_OpenFont failed for score: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), TEXT_COLOR);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for score: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);

    if (!scoreTexture) {
         std::cerr << "SDL_CreateTextureFromSurface failed for score: " << SDL_GetError() << std::endl;
    }
}

// Cập nhật texture điểm cao
void Game::updateHighscoreTexture() {
    std::stringstream ss;
    // Lấy điểm cao từ MainMenu (đã được load hoặc cập nhật)
    int highscore = menu->highscores.empty() ? 0 : menu->highscores[0];
    ss << "Highscore: " << highscore;
    std::string highscoreStr = ss.str();

    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
     if (!font) {
        std::cerr << "TTF_OpenFont failed for highscore: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, highscoreStr.c_str(), TEXT_COLOR);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for highscore: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);

     if (!highscoreTexture) {
         std::cerr << "SDL_CreateTextureFromSurface failed for highscore: " << SDL_GetError() << std::endl;
    }
}

// Cập nhật texture chữ "Paused" (có thể không cần nếu text không đổi)
void Game::updatePausedTexture() {
    // Text "Paused" không đổi, nên chỉ cần tạo 1 lần trong initTextures
    // Nếu muốn thay đổi text (ví dụ thêm điểm), thì mới cần hàm này
}

// Cập nhật texture chữ "Game Over" (có thể không cần nếu text không đổi)
void Game::updateGameOverTextTexture() {
    // Text "Game over" không đổi, nên chỉ cần tạo 1 lần trong initTextures
}

// Cập nhật texture chữ "Volume" (có thể không cần nếu text không đổi)
void Game::updateVolumeLabelTexture() {
    // Text "Volume" không đổi, nên chỉ cần tạo 1 lần trong initTextures
}

// Xử lý input trong game
void Game::handleInput(SDL_Event& event) { // Không cần MainMenu& menu nữa
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        // --- Xử lý click nút Pause ---
        if (SDL_PointInRect(&mousePoint, &pauseButton)) {
            if (!paused && !gameOver) {
                setGameStatePaused(); // Gọi hàm nội bộ để pause
                menu->gameState = MainMenu::PAUSED; // Cập nhật trạng thái cho MainMenu
            } else if (paused) {
                setGameStatePlaying(); // Gọi hàm nội bộ để resume
                menu->gameState = MainMenu::PLAYING; // Cập nhật trạng thái cho MainMenu
            }
        }

        // --- Xử lý khi đang Pause ---
        if (paused) {
            // Kéo volume knob
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                isDraggingVolume = true;
            }
            // Click nút Give Up
            if (SDL_PointInRect(&mousePoint, &giveUpButton)) {
                triggerGameOver(); // Gọi hàm nội bộ để kết thúc game
                menu->gameState = MainMenu::GAME_OVER; // Cập nhật trạng thái cho MainMenu
            }
        }

        // --- Xử lý khi Game Over ---
        if (gameOver) {
            // Click nút Back to Menu
            if (SDL_PointInRect(&mousePoint, &backToMenuButton)) {
                reset(); // Reset trạng thái game
                menu->gameState = MainMenu::MENU; // Quay về menu chính
            }
            // Click nút Restart
            if (SDL_PointInRect(&mousePoint, &restartButton)) {
                reset(); // Reset trạng thái game
                startGame(); // Bắt đầu game mới
                menu->gameState = MainMenu::PLAYING; // Đặt trạng thái đang chơi
            }
        }
    }

    // --- Xử lý thả chuột (khi kéo volume) ---
    if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingVolume) {
            isDraggingVolume = false;
            // Lưu cài đặt âm lượng vào file (thông qua MainMenu)
            menu->volume = volume; // Cập nhật volume trong menu
            menu->saveSettings();  // Lưu cài đặt
        }
    }

    // --- Xử lý kéo chuột (khi kéo volume) ---
    if (event.type == SDL_MOUSEMOTION && isDraggingVolume) {
        int mouseX, mouseY; // Lấy lại tọa độ chuột
        SDL_GetMouseState(&mouseX, &mouseY);

        // Tính toán vị trí mới cho knob dựa trên chuột X
        int newKnobX = mouseX - volumeKnob.w / 2;
        // Giới hạn vị trí knob trong thanh slider
        newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
        volumeKnob.x = newKnobX;

        // Tính toán giá trị volume mới (0-100)
        int newVolume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / (volumeSlider.w - volumeKnob.w)) * 100.0f));
        newVolume = std::max(0, std::min(newVolume, 100)); // Đảm bảo trong khoảng 0-100

        setVolume(newVolume); // Cập nhật âm lượng và UI
        // Không cần cập nhật menu->volume ở đây, chỉ cập nhật khi thả chuột
    }

    // --- Xử lý phím ESC ---
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (!paused && !gameOver) {
            setGameStatePaused();
            menu->gameState = MainMenu::PAUSED;
        } else if (paused) {
            setGameStatePlaying();
            menu->gameState = MainMenu::PLAYING;
        }
        // Nếu đang game over, ESC không làm gì cả
    }
}

// Cập nhật trạng thái game
void Game::update(float deltaTime) {
    // Không cập nhật nếu game over hoặc chưa bắt đầu
    if (gameOver || startTime == 0) return;

    // Nếu đang pause, chỉ cập nhật thời gian pause, không làm gì khác
    if (paused) {
        // Thời gian pause sẽ được cộng dồn khi resume trong handleInput hoặc setGameStatePlaying
        return;
    }

    // Tính thời gian hiện tại của game (đã trừ thời gian pause)
    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    // --- Cập nhật điều khiển khiên ---
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    // Tính hệ số độ nhạy: 0.75 khi sensitivity=0, 1.25 khi sensitivity=100
    float sensitivityFactor = MIN_SENSITIVITY_MULTIPLIER + (static_cast<float>(sensitivity) / 100.0f) * (MAX_SENSITIVITY_MULTIPLIER - MIN_SENSITIVITY_MULTIPLIER);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    // Giữ góc trong khoảng hợp lý (ví dụ: 0 đến 2*PI) nếu cần, nhưng atan2 xử lý được các góc
    // arcStartAngle = fmod(arcStartAngle, 2.0f * PI); // Có thể gây nhảy góc nếu âm

    // --- Spawn Kẻ địch ---

    // Spawn SpaceShark
    if (waveCount >= WAVE_START_SHARK && (waveCount - WAVE_START_SHARK) % WAVE_INTERVAL_SHARK == 0 && spaceSharks.empty()) {
        // Chỉ spawn nếu wave đủ lớn, đúng chu kỳ và chưa có con nào đang hoạt động
        SpaceShark ss;
        ss.radius = SHARK_INITIAL_RADIUS;
        ss.angle = static_cast<float>(dis(gen)) * 2.0f * PI; // Góc ngẫu nhiên
        ss.angularSpeed = SHARK_ANGULAR_SPEED;
        ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
        ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);
        ss.spawnTime = currentTime;
        ss.lastBulletTime = currentTime; // Bắn ngay khi có thể? Hoặc chờ 1 khoảng
        ss.active = true;
        spaceSharks.push_back(ss);
    }

    // Spawn Tên lửa nhanh (Fast Missile) - Quản lý cảnh báo
    if (waveCount >= WAVE_START_FAST_MISSILE && (waveCount - WAVE_START_FAST_MISSILE) % WAVE_INTERVAL_FAST_MISSILE == 0 && fastMissiles.empty() && !showWarning) {
         // Chỉ spawn nếu wave đủ lớn, đúng chu kỳ, chưa có tên lửa nhanh và chưa có cảnh báo
        showWarning = true;
        warningStartTime = currentTime;
        // Chọn vị trí cảnh báo ngẫu nhiên ở rìa màn hình
        int side = dist_side(gen);
        switch (side) {
            case 0: // Trái
                warningX = LIFE_ICON_RADIUS; // Cách rìa 1 chút
                warningY = dist_y_spawn(gen);
                break;
            case 1: // Phải
                warningX = SCREEN_WIDTH - LIFE_ICON_RADIUS;
                warningY = dist_y_spawn(gen);
                break;
            case 2: // Trên
                warningX = dist_x_spawn(gen);
                warningY = LIFE_ICON_RADIUS;
                break;
            case 3: // Dưới
                warningX = dist_x_spawn(gen);
                warningY = SCREEN_HEIGHT - LIFE_ICON_RADIUS;
                break;
        }
    }

    // Kích hoạt tên lửa nhanh sau khi cảnh báo
    if (showWarning && (currentTime - warningStartTime >= FAST_MISSILE_WARNING_DURATION)) {
        showWarning = false; // Tắt cảnh báo
        Target fm; // Tên lửa nhanh dùng cấu trúc Target
        fm.x = static_cast<float>(warningX);
        fm.y = static_cast<float>(warningY);
        // Tính vector hướng vào tâm
        float distX = static_cast<float>(TRAJECTORY_CENTER.x) - fm.x;
        float distY = static_cast<float>(TRAJECTORY_CENTER.y) - fm.y;
        float distance = sqrt(distX * distX + distY * distY);
        if (distance < 1e-6f) distance = 1.0f; // Tránh chia cho 0

        // Tính tốc độ tên lửa nhanh
        float baseSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
        float missileSpeed = baseSpeed * FAST_MISSILE_SPEED_MULTIPLIER;

        fm.dx = (distX / distance) * missileSpeed;
        fm.dy = (distY / distance) * missileSpeed;
        fm.active = true;
        fastMissiles.push_back(fm);
        // isFirstFastMissile = false; // Không cần cờ này nữa
    }

    // Spawn Tên lửa thường (Target) và chuyển Wave
    bool waveAdvanced = false;
    // Chỉ spawn khi game không vừa bắt đầu và đã đến thời gian spawn
    if (!justStarted && currentTime >= nextSpawnTime) {
        // Kiểm tra xem đã spawn đủ tên lửa cho wave này chưa
        if (spawnedMissilesInWave < missileCount) {
            // Kiểm tra khoảng thời gian tối thiểu giữa các tên lửa
            if (currentTime - lastMissileSpawnTime >= MISSILE_SPAWN_INTERVAL || spawnedMissilesInWave == 0) {
                Target t;
                // Chọn vị trí spawn ngẫu nhiên ở rìa
                int side = dist_side(gen);
                 switch (side) {
                    case 0: t.x = 0.0f; t.y = static_cast<float>(dist_y_spawn(gen)); break;
                    case 1: t.x = static_cast<float>(SCREEN_WIDTH); t.y = static_cast<float>(dist_y_spawn(gen)); break;
                    case 2: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = 0.0f; break;
                    case 3: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = static_cast<float>(SCREEN_HEIGHT); break;
                }
                // Tính vector hướng vào tâm
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - t.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - t.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;

                // Tính tốc độ tên lửa thường (có yếu tố ngẫu nhiên)
                float missileSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);

                t.dx = (distX / distance) * missileSpeed;
                t.dy = (distY / distance) * missileSpeed;
                t.active = true;
                targets.push_back(t);

                spawnedMissilesInWave++;
                lastMissileSpawnTime = currentTime;
            }
        }
        // Nếu đã spawn đủ tên lửa và chưa chuyển wave -> chuyển wave
        else if (!waveAdvanced) {
            waveCount++;
            // Kiểm tra xem có cần tăng số lượng tên lửa không
            if (waveCount % wavesUntilIncrease == 0) {
                missileCount++;
                if (missileCount > MAX_MISSILE_COUNT) missileCount = MAX_MISSILE_COUNT;
                // Tính lại số wave ngẫu nhiên cho lần tăng tiếp theo
                wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
            }
            // Đặt thời gian cho wave tiếp theo
            nextSpawnTime = currentTime + BASE_WAVE_DELAY + dist_wave_delay(gen);
            spawnedMissilesInWave = 0; // Reset bộ đếm cho wave mới
            waveAdvanced = true; // Đánh dấu đã chuyển wave trong frame này
        }
    }

    // Reset cờ justStarted sau lần update đầu tiên
    if (justStarted) {
        justStarted = false;
    }

    // --- Cập nhật vị trí và trạng thái Kẻ địch ---

    // Cập nhật SpaceShark
    for (auto& ss : spaceSharks) {
        if (ss.active) {
            // Cập nhật vị trí theo quỹ đạo xoắn ốc
            ss.angle += ss.angularSpeed * deltaTime;
            ss.radius += SHARK_SPIRAL_SPEED * deltaTime; // radius giảm dần
            if (ss.radius < SHARK_MIN_RADIUS) ss.radius = SHARK_MIN_RADIUS; // Giới hạn bán kính tối thiểu
            ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
            ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);

            // Bắn đạn cá mập
            if (currentTime - ss.lastBulletTime >= SHARK_BULLET_INTERVAL) {
                SharkBullet sb;
                sb.x = ss.x;
                sb.y = ss.y;
                // Hướng đạn vào tâm (tàu người chơi)
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - sb.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - sb.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                // Tốc độ đạn cá mập
                float bulletSpeed = DEFAULT_MISSILE_SPEED * SHARK_BULLET_SPEED_MULTIPLIER;
                sb.dx = (distX / distance) * bulletSpeed;
                sb.dy = (distY / distance) * bulletSpeed;
                sb.active = true;
                sharkBullets.push_back(sb);
                ss.lastBulletTime = currentTime; // Reset thời gian bắn
            }

            // Kiểm tra va chạm hoặc hết thời gian sống
            if (CheckCollisionWithChitbox(ss)) { // Va chạm tàu
                ss.active = false;
                HandleHit(); // Xử lý mất mạng
            } else if (CheckCollisionWithArc(ss)) { // Va chạm khiên
                ss.active = false;
                score += SCORE_PER_SHARK; // Cộng điểm
                updateScoreTexture();
            } else if (currentTime - ss.spawnTime >= SHARK_LIFETIME) { // Hết giờ
                 ss.active = false;
                 // Có thể cho điểm hoặc không khi hết giờ? Hiện tại là không.
            }
        }
    }

    // Cập nhật SharkBullet
    for (auto& sb : sharkBullets) {
        if (sb.active) {
            sb.x += sb.dx * deltaTime;
            sb.y += sb.dy * deltaTime;
            // Kiểm tra va chạm
            if (CheckCollisionWithChitbox(sb)) { // Va chạm tàu
                sb.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(sb)) { // Va chạm khiên
                sb.active = false;
                // Không cộng điểm khi đỡ đạn cá mập?
            }
            // Xóa đạn nếu ra khỏi màn hình (tùy chọn)
            else if (sb.x < -SHARK_BULLET_WIDTH || sb.x > SCREEN_WIDTH + SHARK_BULLET_WIDTH ||
                     sb.y < -SHARK_BULLET_HEIGHT || sb.y > SCREEN_HEIGHT + SHARK_BULLET_HEIGHT) {
                sb.active = false;
            }
        }
    }

    // Cập nhật Tên lửa thường (Target)
    for (auto& t : targets) {
        if (t.active) {
            t.x += t.dx * deltaTime;
            t.y += t.dy * deltaTime;
            // Kiểm tra va chạm
            if (CheckCollisionWithChitbox(t)) { // Va chạm tàu
                t.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(t)) { // Va chạm khiên
                t.active = false;
                score += SCORE_PER_MISSILE;
                updateScoreTexture();
            }
             // Xóa nếu ra khỏi màn hình (tùy chọn, thường thì không cần vì nó bay vào tâm)
        }
    }

    // Cập nhật Tên lửa nhanh (Fast Missile)
    for (auto& fm : fastMissiles) {
        if (fm.active) {
            fm.x += fm.dx * deltaTime;
            fm.y += fm.dy * deltaTime;
            // Kiểm tra va chạm
            if (CheckCollisionWithChitbox(fm)) { // Va chạm tàu
                fm.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(fm)) { // Va chạm khiên
                fm.active = false;
                score += SCORE_PER_FAST_MISSILE;
                updateScoreTexture();
            }
             // Xóa nếu ra khỏi màn hình (tùy chọn)
        }
    }

    // --- Xóa các đối tượng không hoạt động ---
    // Sử dụng erase-remove idiom
    targets.erase(
        std::remove_if(targets.begin(), targets.end(), [](const Target& t) { return !t.active; }),
        targets.end()
    );
    fastMissiles.erase(
        std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](const Target& fm) { return !fm.active; }),
        fastMissiles.end()
    );
    spaceSharks.erase(
        std::remove_if(spaceSharks.begin(), spaceSharks.end(), [](const SpaceShark& ss) { return !ss.active; }),
        spaceSharks.end()
    );
    sharkBullets.erase(
        std::remove_if(sharkBullets.begin(), sharkBullets.end(), [](const SharkBullet& sb) { return !sb.active; }),
        sharkBullets.end()
    );
}

// Render game
void Game::render() {
    // Xóa màn hình với màu đen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // --- Render khi đang chơi ---
    if (!gameOver && !paused) {
        // Render tàu người chơi
        if (mspaceshipTexture) {
            SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox);
        }

        // Render quỹ đạo và khiên
        SDL_SetRenderDrawColor(renderer, TRAJECTORY_CIRCLE_COLOR.r, TRAJECTORY_CIRCLE_COLOR.g, TRAJECTORY_CIRCLE_COLOR.b, TRAJECTORY_CIRCLE_COLOR.a);
        DrawCircle(renderer, trajectory); // Vẽ vòng tròn quỹ đạo
        DrawArc(renderer, trajectory, arcStartAngle, SHIELD_ARC_ANGLE); // Vẽ khiên

        // Render mạng sống
        for (const auto& life : lives) {
            Circle lifeCircle = {life.x + LIFE_ICON_RADIUS, life.y + LIFE_ICON_RADIUS, LIFE_ICON_RADIUS}; // Tâm vòng tròn
            const SDL_Color& color = life.isRed ? LIFE_ICON_INACTIVE_COLOR : LIFE_ICON_ACTIVE_COLOR;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            // Có thể vẽ hình tròn đầy hoặc chỉ viền
            // DrawCircle(renderer, lifeCircle); // Chỉ viền
             // Vẽ hình tròn đầy:
            for (int w = 0; w < lifeCircle.r * 2; w++) {
                for (int h = 0; h < lifeCircle.r * 2; h++) {
                    int dx = lifeCircle.r - w; // horizontal offset
                    int dy = lifeCircle.r - h; // vertical offset
                    if ((dx*dx + dy*dy) <= (lifeCircle.r * lifeCircle.r)) {
                        SDL_RenderDrawPoint(renderer, lifeCircle.x + dx, lifeCircle.y + dy);
                    }
                }
            }
        }

        // Render kẻ địch (gọi hàm render của lớp Enemy)
        for (const auto& t : targets) {
            enemy->renderTarget(t);
        }
        for (const auto& fm : fastMissiles) {
            enemy->renderFastMissile(fm);
        }
        for (const auto& ss : spaceSharks) {
            enemy->renderSpaceShark(ss);
        }
        for (const auto& sb : sharkBullets) {
            enemy->renderSharkBullet(sb);
        }

        // Render cảnh báo nếu có
        if (showWarning) {
            enemy->renderWarning(static_cast<float>(warningX), static_cast<float>(warningY), warningStartTime, startTime, totalPausedTime);
        }

         // Render điểm số và điểm cao ở góc màn hình khi đang chơi
        if (scoreTexture) {
            int w, h;
            SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            // Vị trí góc trên bên phải chẳng hạn
            SDL_Rect scoreRect = {SCREEN_WIDTH - w - 10, 60, w, h};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
         if (highscoreTexture) {
            int w, h;
            SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
             // Dưới điểm số
            SDL_Rect highscoreRect = {SCREEN_WIDTH - w - 10, 60 + h + 5, w, h};
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }

    } // Kết thúc render khi đang chơi

    // --- Render nút Pause (luôn hiển thị trừ khi Game Over?) ---
    if (!gameOver && pauseButtonTexture) {
        SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton);
    }

    // --- Render màn hình Game Over ---
    if (gameOver) {
        // Lớp phủ mờ (tùy chọn)
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Render tiêu đề "Game Over"
        if (gameOverTextTexture) {
            int w, h;
            SDL_QueryTexture(gameOverTextTexture, NULL, NULL, &w, &h);
            SDL_Rect gameOverRect = { (SCREEN_WIDTH - w) / 2, GAMEOVER_TITLE_Y, w, h };
            SDL_RenderCopy(renderer, gameOverTextTexture, NULL, &gameOverRect);
        }

        // Render điểm số
        if (scoreTexture) {
            int w, h;
            SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = { (SCREEN_WIDTH - w) / 2, SCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }

        // Render điểm cao
        if (highscoreTexture) {
            int w, h;
            SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = { (SCREEN_WIDTH - w) / 2, HIGHSCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }

        // Render nút Restart
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &restartButton);
        if (restartTexture) {
            int w, h;
            SDL_QueryTexture(restartTexture, NULL, NULL, &w, &h);
            SDL_Rect restartTextRect = { restartButton.x + (restartButton.w - w) / 2, restartButton.y + (restartButton.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, restartTexture, NULL, &restartTextRect);
        }

        // Render nút Back to Menu
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &backToMenuButton);
        if (backToMenuTexture) {
            int w, h;
            SDL_QueryTexture(backToMenuTexture, NULL, NULL, &w, &h);
            SDL_Rect backToMenuTextRect = { backToMenuButton.x + (backToMenuButton.w - w) / 2, backToMenuButton.y + (backToMenuButton.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, backToMenuTexture, NULL, &backToMenuTextRect);
        }
    }
    // --- Render màn hình Pause ---
    else if (paused) {
        // Lớp phủ mờ
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Render tiêu đề "Paused"
        if (pausedTexture) {
            int w, h;
            SDL_QueryTexture(pausedTexture, NULL, NULL, &w, &h);
            SDL_Rect pausedRect = { (SCREEN_WIDTH - w) / 2, PAUSED_TITLE_Y, w, h };
            SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
        }

        // Render điểm số
        if (scoreTexture) {
            int w, h;
            SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = { (SCREEN_WIDTH - w) / 2, SCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }

        // Render điểm cao
        if (highscoreTexture) {
            int w, h;
            SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = { (SCREEN_WIDTH - w) / 2, HIGHSCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }

        // Render Volume Label
        if (volumeLabelTexture) {
            int w, h;
            SDL_QueryTexture(volumeLabelTexture, NULL, NULL, &w, &h);
            SDL_Rect volumeLabelRect = { (SCREEN_WIDTH - w) / 2, VOLUME_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, volumeLabelTexture, NULL, &volumeLabelRect);
        }

        // Render Volume Slider Background
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider);
        // Render Volume Knob
         const SDL_Color& knobColor = isDraggingVolume ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, knobColor.r, knobColor.g, knobColor.b, knobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob);

        // Render nút Give Up
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &giveUpButton);
        if (giveUpTexture) {
            int w, h;
            SDL_QueryTexture(giveUpTexture, NULL, NULL, &w, &h);
            SDL_Rect giveUpTextRect = { giveUpButton.x + (giveUpButton.w - w) / 2, giveUpButton.y + (giveUpButton.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, giveUpTexture, NULL, &giveUpTextRect);
        }
    }

    // Hiển thị tất cả những gì đã render lên màn hình
    SDL_RenderPresent(renderer);
}

// Reset trạng thái game về ban đầu
void Game::reset() {
    gameOver = false;
    paused = false;
    showWarning = false;
    isFirstFastMissile = true; // Reset cờ này nếu vẫn dùng
    warningStartTime = 0;
    warningX = 0;
    warningY = 0;
    targets.clear();
    fastMissiles.clear();
    spaceSharks.clear();
    sharkBullets.clear();

    // Reset mạng sống
    for (auto& life : lives) {
        life.isRed = false;
    }

    // Reset thông số wave và điểm
    missileCount = INITIAL_MISSILE_COUNT;
    waveCount = 0;
    // maxMissileSpeed = defaultMissileSpeed; // Không cần nữa
    score = 0;
    nextSpawnTime = INITIAL_SPAWN_DELAY; // Thời gian spawn ban đầu
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    arcStartAngle = INITIAL_SHIELD_START_ANGLE;
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen); // Random lại

    // Reset thời gian
    startTime = 0; // Sẽ được đặt lại khi gọi startGame()
    pauseStartTime = 0;
    totalPausedTime = 0;
    justStarted = false; // Sẽ được đặt thành true bởi startGame()

    // Không cần hủy và tạo lại texture nút ở đây vì chúng đã được tạo trong constructor/initTextures
    // Chỉ cần cập nhật texture điểm số và điểm cao
    updateScoreTexture();
    updateHighscoreTexture();
    // Đảm bảo volume và sensitivity được đặt lại từ MainMenu nếu cần
    setVolume(menu->volume);
    setSensitivity(menu->sensitivity);
}

// Bắt đầu game (sau khi reset hoặc lần đầu)
void Game::startGame() {
    startTime = SDL_GetTicks(); // Ghi lại thời điểm bắt đầu
    totalPausedTime = 0;      // Reset tổng thời gian pause
    pauseStartTime = 0;
    justStarted = true;       // Đánh dấu cho lần update đầu tiên
    gameOver = false;
    paused = false;
    // Đảm bảo các texture điểm được cập nhật
    updateScoreTexture();
    updateHighscoreTexture();
}


// --- Hàm kiểm tra va chạm ---
// (Sử dụng bán kính bình phương để tránh dùng sqrt)

// Va chạm giữa Target (tên lửa thường/nhanh) và Khiên (Arc)
bool Game::CheckCollisionWithArc(const Target& t) {
    // Tính tâm của tên lửa (gần đúng)
    float targetCenterX = t.x; // Giả sử t.x, t.y là tâm
    float targetCenterY = t.y;

    // Kiểm tra khoảng cách từ tâm tên lửa đến tâm quỹ đạo
    float dx = targetCenterX - trajectory.x;
    float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;

    // Kiểm tra sơ bộ: Nếu quá xa hoặc quá gần tâm quỹ đạo thì không thể va chạm
    float outerRadiusSq = (trajectory.r + 10.0f) * (trajectory.r + 10.0f); // Bán kính ngoài + lề
    float innerRadiusSq = (trajectory.r - 10.0f) * (trajectory.r - 10.0f); // Bán kính trong - lề
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) {
        return false;
    }

    // Tính góc của tên lửa so với tâm quỹ đạo
    float targetAngle = atan2(dy, dx);

    // Chuẩn hóa góc về khoảng [0, 2*PI) hoặc [-PI, PI) để so sánh dễ dàng
    // Chuẩn hóa góc khiên về cùng khoảng
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI);
    if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI);
     if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;

    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI);
    if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;


    // Kiểm tra xem góc của tên lửa có nằm trong khoảng góc của khiên không
    if (normalizedArcStart <= normalizedArcEnd) {
        // Trường hợp cung không vượt qua góc 0
        if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) {
            // Kiểm tra khoảng cách thực tế hơn nếu cần, hoặc trả về true ngay
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq; // Kiểm tra lại khoảng cách
        }
    } else {
        // Trường hợp cung vượt qua góc 0 (ví dụ: từ 5 PI/3 đến PI/3)
        if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq;
        }
    }

    return false; // Không va chạm
}


// Va chạm giữa Target và Tàu người chơi (Chitbox)
bool Game::CheckCollisionWithChitbox(const Target& t) {
    // Va chạm AABB đơn giản
    // Giả sử t.x, t.y là góc trên bên trái của tên lửa (cần điều chỉnh nếu là tâm)
    // SDL_Rect targetRect = { (int)t.x - MISSILE_CENTER.x, (int)t.y - MISSILE_CENTER.y, MISSILE_WIDTH, MISSILE_HEIGHT }; // Nếu t là tâm
     SDL_Rect targetRect = { (int)t.x, (int)t.y, 5, 5 }; // Giả sử hitbox nhỏ quanh t.x, t.y
    return SDL_HasIntersection(&targetRect, &chitbox);
}

// Va chạm giữa SpaceShark và Khiên (Arc)
bool Game::CheckCollisionWithArc(const SpaceShark& ss) {
    // Tương tự CheckCollisionWithArc(Target& t), nhưng dùng bán kính va chạm lớn hơn
    float targetCenterX = ss.x;
    float targetCenterY = ss.y;
    float dx = targetCenterX - trajectory.x;
    float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;

    float collisionRadius = sqrt(SHARK_COLLISION_RADIUS_SQ); // Bán kính va chạm của cá mập
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius);
     if (innerRadiusSq < 0) innerRadiusSq = 0; // Tránh bán kính âm

    if (distSq > outerRadiusSq || distSq < innerRadiusSq) {
        return false;
    }

    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI);
    if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI);
     if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;

    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI);
    if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;

    if (normalizedArcStart <= normalizedArcEnd) {
        if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq;
        }
    } else {
        if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq;
        }
    }
    return false;
}

// Va chạm giữa SpaceShark và Tàu người chơi (Chitbox)
bool Game::CheckCollisionWithChitbox(const SpaceShark& ss) {
    // Va chạm AABB giữa hitbox cá mập và hitbox tàu
    SDL_Rect sharkRect = { (int)ss.x - SHARK_CENTER.x, (int)ss.y - SHARK_CENTER.y, SHARK_WIDTH, SHARK_HEIGHT };
    return SDL_HasIntersection(&sharkRect, &chitbox);
}

// Va chạm giữa SharkBullet và Khiên (Arc)
bool Game::CheckCollisionWithArc(const SharkBullet& sb) {
    // Tương tự CheckCollisionWithArc(Target& t), dùng bán kính va chạm của đạn cá mập
    float targetCenterX = sb.x;
    float targetCenterY = sb.y;
    float dx = targetCenterX - trajectory.x;
    float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;

    float collisionRadius = sqrt(SHARK_BULLET_COLLISION_RADIUS_SQ);
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius);
     if (innerRadiusSq < 0) innerRadiusSq = 0;

    if (distSq > outerRadiusSq || distSq < innerRadiusSq) {
        return false;
    }

    float targetAngle = atan2(dy, dx);
     float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI);
    if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI);
     if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;

    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI);
    if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;


    if (normalizedArcStart <= normalizedArcEnd) {
        if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq;
        }
    } else {
        if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq;
        }
    }
    return false;
}

// Va chạm giữa SharkBullet và Tàu người chơi (Chitbox)
bool Game::CheckCollisionWithChitbox(const SharkBullet& sb) {
    // Va chạm AABB
    SDL_Rect bulletRect = { (int)sb.x - SHARK_BULLET_CENTER.x, (int)sb.y - SHARK_BULLET_CENTER.y, SHARK_BULLET_WIDTH, SHARK_BULLET_HEIGHT };
    return SDL_HasIntersection(&bulletRect, &chitbox);
}


// --- Hàm tiện ích ---

// Vẽ hình tròn (viền)
void Game::DrawCircle(SDL_Renderer* renderer, const Circle& c) {
    SDL_Point points[CIRCLE_SEGMENTS + 1];
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float rad = (2.0f * PI * i) / CIRCLE_SEGMENTS;
        points[i].x = c.x + static_cast<int>(c.r * cos(rad));
        points[i].y = c.y + static_cast<int>(c.r * sin(rad));
    }
    SDL_RenderDrawLines(renderer, points, CIRCLE_SEGMENTS + 1);
}

// Vẽ cung tròn (viền)
void Game::DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle) {
    SDL_SetRenderDrawColor(renderer, SHIELD_ARC_COLOR.r, SHIELD_ARC_COLOR.g, SHIELD_ARC_COLOR.b, SHIELD_ARC_COLOR.a); // Đặt màu cho khiên
    SDL_Point points[ARC_SEGMENTS + 1];
    for (int i = 0; i <= ARC_SEGMENTS; ++i) {
        double angle = startAngle + (arcAngle * i / ARC_SEGMENTS);
        points[i].x = c.x + static_cast<int>(c.r * cos(angle));
        points[i].y = c.y + static_cast<int>(c.r * sin(angle));
    }
    SDL_RenderDrawLines(renderer, points, ARC_SEGMENTS + 1);
}

// Xử lý khi tàu bị bắn trúng
void Game::HandleHit() {
    if (gameOver) return; // Không xử lý nếu đã game over

    // Tìm mạng sống chưa mất (chưa đỏ) đầu tiên và đánh dấu là đã mất
    for (auto& life : lives) {
        if (!life.isRed) {
            life.isRed = true;
            // Có thể thêm hiệu ứng âm thanh, hình ảnh ở đây
            goto checkGameOver; // Thoát vòng lặp sau khi tìm thấy
        }
    }

checkGameOver:
    // Kiểm tra xem tất cả mạng sống đã mất chưa
    bool allRed = true;
    for (const auto& life : lives) {
        if (!life.isRed) {
            allRed = false;
            break;
        }
    }

    // Nếu tất cả đã mất, kết thúc game
    if (allRed) {
        triggerGameOver(); // Gọi hàm kết thúc game
        menu->gameState = MainMenu::GAME_OVER; // Cập nhật trạng thái cho MainMenu
    }
}

// Setter cho volume (cập nhật cả knob và âm lượng mixer)
void Game::setVolume(int vol) {
    if (vol >= 0 && vol <= 100) {
        volume = vol;
        // Cập nhật vị trí knob (đảm bảo không vượt ra ngoài slider)
        int knobRange = volumeSlider.w - volumeKnob.w;
        volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRange));
        // Cập nhật âm lượng thực tế
        Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100); // MIX_MAX_VOLUME (128)
        // Có thể cập nhật cả âm lượng hiệu ứng nếu cần: Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100);
    }
}
