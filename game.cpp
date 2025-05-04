#include "game.h"
#include "mainmenu.h"
#include "config.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <random>
#include <memory> // Cho std::unique_ptr nếu muốn dùng

// --- C++11 Random Number Generation ---
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0); // Phân bố đều từ 0.0 đến 1.0
std::uniform_int_distribution<> dist_wave_increase(0, RANDOM_WAVES_UNTIL_INCREASE -1); // Ngẫu nhiên số wave tăng thêm
std::uniform_int_distribution<> dist_wave_delay(0, RANDOM_WAVE_DELAY -1); // Ngẫu nhiên độ trễ wave tăng thêm
std::uniform_int_distribution<> dist_side(0, 3); // Chọn cạnh màn hình (0:trái, 1:phải, 2:trên, 3:dưới)
std::uniform_int_distribution<> dist_y_spawn(0, SCREEN_HEIGHT - 1); // Vị trí Y ngẫu nhiên
std::uniform_int_distribution<> dist_x_spawn(0, SCREEN_WIDTH - 1); // Vị trí X ngẫu nhiên

// --- Hàm trợ giúp load texture (ĐỊNH NGHĨA Ở ĐÂY) ---
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    } else {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == nullptr) {
            std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        }
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}


// Constructor Game
// --- Cập nhật Constructor để nhận và lưu sfxHealCollect ---
Game::Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
           Mix_Chunk* sfxShieldHitIn, Mix_Chunk* sfxPlayerHitIn,
           Mix_Chunk* sfxGameOverIn, Mix_Chunk* sfxWarningIn,
           Mix_Chunk* sfxHealCollectIn, // Thêm tham số âm thanh heal
           Mix_Music* bgmGameIn,
           SDL_Texture* bgTexture)
    : renderer(r), enemy(e), menu(m),
      // Textures
      mspaceshipTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr),
      highscoreTexture(nullptr), pausedTexture(nullptr), backToMenuTexture(nullptr),
      restartTexture(nullptr), gameOverTextTexture(nullptr), volumeLabelTexture(nullptr),
      giveUpTexture(nullptr), backgroundTexture(bgTexture),
      // --- Textures mới ---
      allyShipTexture(nullptr), healItemTexture(nullptr),
      // Sounds
      sfxShieldHit(sfxShieldHitIn), sfxPlayerHit(sfxPlayerHitIn),
      sfxGameOver(sfxGameOverIn), sfxWarning(sfxWarningIn),
      // --- Âm thanh mới ---
      sfxHealCollect(sfxHealCollectIn), // Lưu con trỏ âm thanh heal
      bgmGame(bgmGameIn),
      // State
      gameOver(false), paused(false), showWarning(false),
      justStarted(false),
      // Timing
      startTime(0), pauseStartTime(0), totalPausedTime(0), warningStartTime(0),
      nextSpawnTime(INITIAL_SPAWN_DELAY), lastMissileSpawnTime(0),
      // --- Thời gian mới ---
      lastAllySpawnTime(0), // Khởi tạo thời gian spawn ally
      // Score, Wave
      score(0), missileCount(INITIAL_MISSILE_COUNT), waveCount(0),
      spawnedMissilesInWave(0),
      // Position, Angle
      warningX(0), warningY(0), arcStartAngle(INITIAL_SHIELD_START_ANGLE),
      // Settings
      volume(DEFAULT_VOLUME), sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)), isDraggingVolume(false),
      // UI
      chitbox(PLAYER_CHITBOX), pauseButton(PAUSE_BUTTON_RECT),
      backToMenuButton(BACK_TO_MENU_BUTTON_RECT_GAMEOVER), restartButton(RESTART_BUTTON_RECT),
      giveUpButton(GIVE_UP_BUTTON_RECT), volumeSlider(VOLUME_SLIDER_RECT),
      volumeKnob(VOLUME_KNOB_RECT),
      // Objects
      trajectory{TRAJECTORY_CENTER.x, TRAJECTORY_CENTER.y, TRAJECTORY_RADIUS}
      // Các vector allies, healItems sẽ được khởi tạo rỗng mặc định
{
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);

    lives.clear();
    for (int i = 0; i < PLAYER_LIVES; ++i) {
        Life life = {LIFE_ICON_START_X + i * LIFE_ICON_SPACING, LIFE_ICON_START_Y, false};
        lives.push_back(life);
    }

    // Áp dụng cài đặt ban đầu từ menu
    setVolume(menu->volume);
    setSensitivity(menu->sensitivity);

    // Nạp các texture ảnh cần thiết cho Game (sử dụng hàm loadTexture toàn cục)
    mspaceshipTexture = loadTexture(renderer, IMG_SPACESHIP);
    pauseButtonTexture = loadTexture(renderer, IMG_PAUSE_BUTTON);
    // --- NẠP TEXTURES MỚI ---
    allyShipTexture = loadTexture(renderer, IMG_ALLY_SHIP);
    healItemTexture = loadTexture(renderer, IMG_HEAL_ITEM);

    // Kiểm tra lỗi nạp texture cơ bản
    if (!mspaceshipTexture) { std::cerr << "Error loading spaceship texture!" << std::endl; /* Xử lý lỗi */ }
    if (!pauseButtonTexture) { std::cerr << "Error loading pause button texture!" << std::endl; /* Xử lý lỗi */ }
    // Kiểm tra lỗi nạp texture mới
    if (!allyShipTexture) { std::cerr << "Error loading ally ship texture!" << std::endl; /* Xử lý lỗi */ }
    if (!healItemTexture) { std::cerr << "Error loading heal item texture!" << std::endl; /* Xử lý lỗi */ }


    // Khởi tạo các texture chữ
    initTextures(); // Gọi hàm tạo texture chữ
    // Cập nhật các texture động ban đầu
    updateScoreTexture();
    updateHighscoreTexture();
    // Các texture chữ cố định như "Paused", "Volume" đã được tạo trong initTextures
}

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
    // --- GIẢI PHÓNG TEXTURES MỚI ---
    if (allyShipTexture) SDL_DestroyTexture(allyShipTexture);
    if (healItemTexture) SDL_DestroyTexture(healItemTexture);
    // backgroundTexture và âm thanh được giải phóng ở main.cpp
}

// Khởi tạo các texture chữ cho màn hình Pause/Game Over
void Game::initTextures() {
    // Mở các cỡ font cần thiết
    TTF_Font* fontLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    TTF_Font* fontXLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_XLARGE);
    TTF_Font* fontNormal = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);

    // Kiểm tra lỗi mở font
    if (!fontLarge || !fontXLarge || !fontNormal) {
        std::cerr << "TTF_OpenFont failed in Game::initTextures: " << TTF_GetError() << std::endl;
        if(fontLarge) TTF_CloseFont(fontLarge);
        if(fontXLarge) TTF_CloseFont(fontXLarge);
        if(fontNormal) TTF_CloseFont(fontNormal);
        // Đặt các texture về null để tránh lỗi khi render
        pausedTexture = nullptr;
        backToMenuTexture = nullptr;
        restartTexture = nullptr;
        gameOverTextTexture = nullptr;
        volumeLabelTexture = nullptr;
        giveUpTexture = nullptr;
        scoreTexture = nullptr; // Cũng nên đặt về null nếu font lỗi
        highscoreTexture = nullptr;
        return;
    }
    std::cout << "Fonts opened successfully in Game::initTextures." << std::endl;


    // Hàm trợ giúp tạo texture (thêm kiểm tra lỗi)
    auto createTextureHelper = [&](const char* text, SDL_Texture*& texture, TTF_Font* fontToUse) {
        if (!fontToUse) {
             std::cerr << "Error: Attempting to create texture \"" << text << "\" with a null font." << std::endl;
             texture = nullptr; return false;
        }
        SDL_Surface* textSurface = TTF_RenderText_Solid(fontToUse, text, TEXT_COLOR);
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
            texture = nullptr; return false; // Đặt texture là null nếu lỗi
        }
        if (texture) SDL_DestroyTexture(texture); // Hủy texture cũ nếu có
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
        // std::cout << "Successfully created texture for: " << text << std::endl; // Bỏ comment để debug
        return true;
    };

    // Tạo textures và kiểm tra lỗi
    if (!createTextureHelper("Back to Menu", backToMenuTexture, fontLarge)) { std::cerr << "Error creating back to menu texture." << std::endl; }
    if (!createTextureHelper("Restart", restartTexture, fontLarge)) { std::cerr << "Error creating restart texture." << std::endl; }
    if (!createTextureHelper("Give Up", giveUpTexture, fontLarge)) { std::cerr << "Error creating give up texture." << std::endl; }
    if (!createTextureHelper("Paused", pausedTexture, fontLarge)) { std::cerr << "Error creating paused texture." << std::endl; }
    if (!createTextureHelper("Game over", gameOverTextTexture, fontXLarge)) { std::cerr << "Error creating game over texture." << std::endl; }
    if (!createTextureHelper("Volume", volumeLabelTexture, fontNormal)) { std::cerr << "Error creating volume label texture." << std::endl; }

    // Đóng fonts sau khi dùng xong
    TTF_CloseFont(fontLarge);
    TTF_CloseFont(fontXLarge);
    TTF_CloseFont(fontNormal);
}

// Cập nhật texture điểm số
void Game::updateScoreTexture() {
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    if (!font) { std::cerr << "Failed to open font for score: " << TTF_GetError() << std::endl; return; }
    std::stringstream ss;
    ss << "Score: " << score;
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, ss.str().c_str(), TEXT_COLOR);
    TTF_CloseFont(font); // Đóng font ngay
    if (!textSurface) { std::cerr << "Failed to render score text: " << TTF_GetError() << std::endl; return; }
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!scoreTexture) { std::cerr << "Failed to create score texture: " << SDL_GetError() << std::endl; }
}

// Cập nhật texture điểm cao
void Game::updateHighscoreTexture() {
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
     if (!font) { std::cerr << "Failed to open font for highscore: " << TTF_GetError() << std::endl; return; }
    std::stringstream ss;
    // Lấy điểm cao nhất từ MainMenu (đảm bảo menu đã load highscores)
    int highscore = (menu && !menu->highscores.empty()) ? menu->highscores[0] : 0;
    ss << "Highscore: " << highscore;
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, ss.str().c_str(), TEXT_COLOR);
    TTF_CloseFont(font); // Đóng font ngay
    if (!textSurface) { std::cerr << "Failed to render highscore text: " << TTF_GetError() << std::endl; return; }
    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
     if (!highscoreTexture) { std::cerr << "Failed to create highscore texture: " << SDL_GetError() << std::endl; }
}

// Các hàm update texture chữ cố định không cần thay đổi
void Game::updatePausedTexture() { /* Không cần cập nhật vì text cố định */ }
void Game::updateGameOverTextTexture() { /* Không cần cập nhật vì text cố định */ }
void Game::updateVolumeLabelTexture() { /* Không cần cập nhật vì text cố định */ }

// Xử lý input trong game
void Game::handleInput(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        // Click nút Pause (chỉ hoạt động khi không Game Over)
        if (!gameOver && SDL_PointInRect(&mousePoint, &pauseButton)) {
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            if (!paused) { setGameStatePaused(); menu->gameState = MainMenu::PAUSED; }
            else { setGameStatePlaying(); menu->gameState = MainMenu::PLAYING; }
            return; // Đã xử lý, không cần kiểm tra nút khác trong Pause/GameOver
        }

        // Khi đang Pause
        if (paused) {
            // Kiểm tra click vào núm volume (để bắt đầu kéo)
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                isDraggingVolume = true;
                // Không cần phát âm thanh click ở đây, vì chỉ là bắt đầu kéo
                return; // Đã xử lý
            }
            // Kiểm tra click vào nút Give Up
            if (SDL_PointInRect(&mousePoint, &giveUpButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                triggerGameOver(); // Kết thúc game
                menu->gameState = MainMenu::GAME_OVER; // Chuyển trạng thái menu
                return; // Đã xử lý
            }
        }

        // Khi Game Over
        if (gameOver) {
            // Kiểm tra click vào nút Back to Menu
            if (SDL_PointInRect(&mousePoint, &backToMenuButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset(); // Reset trạng thái game về ban đầu
                menu->gameState = MainMenu::MENU; // Chuyển về menu chính
                 Mix_HaltMusic(); // Dừng nhạc game
                 if (menu->bgmMenu) Mix_PlayMusic(menu->bgmMenu, -1); // Bật lại nhạc menu
                return; // Đã xử lý
            }
            // Kiểm tra click vào nút Restart
            if (SDL_PointInRect(&mousePoint, &restartButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset(); // Reset game
                startGame(); // Bắt đầu game mới
                menu->gameState = MainMenu::PLAYING; // Chuyển trạng thái menu sang chơi
                return; // Đã xử lý
            }
        }
    }
    // Thả chuột (kết thúc kéo volume)
    if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingVolume) {
            isDraggingVolume = false;
            // Lưu cài đặt volume mới vào menu và file
            if (menu) {
                menu->volume = volume;
                menu->saveSettings();
            }
        }
     }
    // Kéo chuột (thay đổi volume)
    if (event.type == SDL_MOUSEMOTION && isDraggingVolume) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        // Tính toán vị trí mới của núm dựa trên chuột X
        int newKnobX = mouseX - volumeKnob.w / 2;
        // Giới hạn vị trí núm trong phạm vi thanh trượt
        newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
        volumeKnob.x = newKnobX;
        // Tính toán giá trị volume mới (0-100)
        int newVolume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / (volumeSlider.w - volumeKnob.w)) * 100.0f));
        // Đảm bảo volume trong khoảng 0-100
        newVolume = std::max(0, std::min(newVolume, 100));
        setVolume(newVolume); // Áp dụng volume mới
     }
    // Phím ESC (Pause/Resume)
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (!gameOver) { // Chỉ hoạt động khi chưa game over
            if (!paused) {
                if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                setGameStatePaused();
                menu->gameState = MainMenu::PAUSED;
            } else {
                if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                setGameStatePlaying();
                menu->gameState = MainMenu::PLAYING;
            }
        }
     }
}

// Cập nhật logic game
void Game::update(float deltaTime) {
    // Không cập nhật nếu game over, chưa bắt đầu, hoặc đang pause
    if (gameOver || startTime == 0 || paused) return;

    // Lấy thời gian hiện tại trong game (đã trừ thời gian pause)
    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    // --- Cập nhật điều khiển khiên ---
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    // Tính hệ số nhạy dựa trên cài đặt
    float sensitivityFactor = MIN_SENSITIVITY_MULTIPLIER + (static_cast<float>(sensitivity) / 100.0f) * (MAX_SENSITIVITY_MULTIPLIER - MIN_SENSITIVITY_MULTIPLIER);
    // Xoay khiên bằng phím A/D
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    // Giữ góc trong khoảng 0 - 2*PI để tránh tràn số (tùy chọn)
    arcStartAngle = fmod(arcStartAngle, 2.0f * PI);
    if (arcStartAngle < 0) arcStartAngle += 2.0f * PI;

    // --- Spawn Đối tượng ---
    // Spawn Đồng minh (Ally)
    if (currentTime - lastAllySpawnTime >= ALLY_SPAWN_INTERVAL) {
        SpawnAlly();
        lastAllySpawnTime = currentTime; // Cập nhật thời điểm spawn cuối
    }

    // Spawn SpaceShark (nếu đủ wave và chưa có con nào)
    if (waveCount >= WAVE_START_SHARK && (waveCount - WAVE_START_SHARK) % WAVE_INTERVAL_SHARK == 0 && spaceSharks.empty()) {
        SpaceShark ss;
        ss.radius = SHARK_INITIAL_RADIUS;
        ss.angle = static_cast<float>(dis(gen)) * 2.0f * PI; // Góc ngẫu nhiên
        ss.angularSpeed = (dis(gen) > 0.5 ? 1.0f : -1.0f) * SHARK_ANGULAR_SPEED; // Chiều quay ngẫu nhiên
        ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
        ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);
        ss.spawnTime = currentTime;
        ss.lastBulletTime = currentTime; // Bắn ngay khi xuất hiện? Có thể cộng thêm delay
        ss.active = true;
        spaceSharks.push_back(ss);
    }

    // Spawn Tên lửa nhanh (Fast Missile) - Quản lý cảnh báo
    // Chỉ bắt đầu cảnh báo nếu đủ wave, không có tên lửa nhanh nào đang hoạt động và chưa có cảnh báo
    if (waveCount >= WAVE_START_FAST_MISSILE && (waveCount - WAVE_START_FAST_MISSILE) % WAVE_INTERVAL_FAST_MISSILE == 0 && fastMissiles.empty() && !showWarning) {
        showWarning = true;
        warningStartTime = currentTime;
         if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1); // Phát cảnh báo lặp lại
        // Chọn vị trí cảnh báo ngẫu nhiên trên biên màn hình
        int side = dist_side(gen);
        switch (side) {
            case 0: warningX = WARNING_ICON_WIDTH / 2; warningY = dist_y_spawn(gen); break; // Trái
            case 1: warningX = SCREEN_WIDTH - WARNING_ICON_WIDTH / 2; warningY = dist_y_spawn(gen); break; // Phải
            case 2: warningX = dist_x_spawn(gen); warningY = WARNING_ICON_HEIGHT / 2; break; // Trên
            case 3: warningX = dist_x_spawn(gen); warningY = SCREEN_HEIGHT - WARNING_ICON_HEIGHT / 2; break; // Dưới
        }
    }

    // Kích hoạt tên lửa nhanh sau khi hết thời gian cảnh báo
    if (showWarning && (currentTime - warningStartTime >= FAST_MISSILE_WARNING_DURATION)) {
        showWarning = false; // Tắt cảnh báo
         Mix_HaltChannel(CHANNEL_WARNING); // Dừng âm thanh cảnh báo
        Target fm; // Tên lửa nhanh dùng chung cấu trúc Target
        fm.x = static_cast<float>(warningX); // Vị trí xuất hiện là vị trí cảnh báo
        fm.y = static_cast<float>(warningY);
        // Tính vector hướng về tâm
        float distX = static_cast<float>(TRAJECTORY_CENTER.x) - fm.x;
        float distY = static_cast<float>(TRAJECTORY_CENTER.y) - fm.y;
        float distance = sqrt(distX * distX + distY * distY);
        if (distance < 1e-6f) distance = 1.0f; // Tránh chia cho 0
        // Tính tốc độ (có yếu tố ngẫu nhiên nhỏ)
        float baseSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
        float missileSpeed = baseSpeed * FAST_MISSILE_SPEED_MULTIPLIER; // Nhân hệ số tốc độ nhanh
        // Đặt vector vận tốc
        fm.dx = (distX / distance) * missileSpeed;
        fm.dy = (distY / distance) * missileSpeed;
        fm.active = true;
        fastMissiles.push_back(fm); // Thêm vào danh sách
    }

    // Spawn Tên lửa thường (Target) và chuyển Wave
    bool waveAdvanced = false; // Cờ để đảm bảo chỉ chuyển wave 1 lần
    // Kiểm tra xem có đến lúc spawn wave tiếp theo chưa
    if (!justStarted && currentTime >= nextSpawnTime) {
        // Nếu số tên lửa đã spawn trong wave hiện tại < số lượng yêu cầu
        if (spawnedMissilesInWave < missileCount) {
            // Spawn tên lửa mới nếu đủ thời gian giãn cách hoặc là tên lửa đầu tiên của wave
            if (currentTime - lastMissileSpawnTime >= MISSILE_SPAWN_INTERVAL || spawnedMissilesInWave == 0) {
                Target t;
                // Chọn vị trí xuất hiện ngẫu nhiên trên biên
                int side = dist_side(gen);
                 switch (side) {
                    case 0: t.x = 0.0f - MISSILE_WIDTH; t.y = static_cast<float>(dist_y_spawn(gen)); break; // Xuất hiện hẳn bên ngoài trái
                    case 1: t.x = static_cast<float>(SCREEN_WIDTH); t.y = static_cast<float>(dist_y_spawn(gen)); break; // Ngoài phải
                    case 2: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = 0.0f - MISSILE_HEIGHT; break; // Ngoài trên
                    case 3: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = static_cast<float>(SCREEN_HEIGHT); break; // Ngoài dưới
                }
                // Tính vector hướng về tâm
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - t.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - t.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                // Tính tốc độ (có yếu tố ngẫu nhiên)
                float missileSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
                t.dx = (distX / distance) * missileSpeed;
                t.dy = (distY / distance) * missileSpeed;
                t.active = true;
                targets.push_back(t); // Thêm vào danh sách
                spawnedMissilesInWave++; // Tăng số lượng đã spawn
                lastMissileSpawnTime = currentTime; // Cập nhật thời điểm spawn cuối
            }
        }
        // Nếu đã spawn đủ tên lửa cho wave hiện tại và chưa chuyển wave
        else if (!waveAdvanced) {
            waveCount++; // Tăng số wave
            // Kiểm tra xem có tăng số lượng tên lửa cho wave sau không
            if (waveCount > 0 && waveCount % wavesUntilIncrease == 0) { // Bắt đầu kiểm tra từ wave 1
                missileCount++;
                if (missileCount > MAX_MISSILE_COUNT) missileCount = MAX_MISSILE_COUNT; // Giới hạn số lượng
                // Đặt lại số wave ngẫu nhiên cho lần tăng tiếp theo
                wavesUntilIncrease = waveCount + BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
            }
            // Đặt thời điểm spawn cho wave tiếp theo
            nextSpawnTime = currentTime + BASE_WAVE_DELAY + dist_wave_delay(gen);
            spawnedMissilesInWave = 0; // Reset bộ đếm spawn cho wave mới
            waveAdvanced = true; // Đánh dấu đã chuyển wave
        }
    }
    // Reset cờ justStarted sau lần update đầu tiên
    if (justStarted) justStarted = false;

    // --- Cập nhật vị trí và trạng thái Đối tượng ---
    // Cập nhật Đồng minh (Ally)
    for (auto& ally : allies) {
        if (ally.active) {
            // Di chuyển ngang từ trái sang phải
            ally.x += ally.speed * deltaTime;

            // Kiểm tra xem đã bay ra khỏi màn hình bên phải chưa
            if (ally.x > SCREEN_WIDTH) {
                ally.active = false; // Đánh dấu không hoạt động
                continue; // Bỏ qua phần còn lại cho ally này
            }

            // Kiểm tra thả heal:
            // 1. Chưa thả lần nào (droppingHeal == false)
            // 2. Vị trí X của ally nằm trong khoảng X của tàu người chơi
            if (!ally.droppingHeal && ally.x >= chitbox.x && ally.x <= chitbox.x + chitbox.w) {
                HealItem heal;
                heal.x = ally.x + ALLY_WIDTH / 2 - HEAL_ITEM_WIDTH / 2; // Thả từ giữa ally
                heal.y = ally.y + ALLY_HEIGHT; // Thả từ bên dưới ally
                heal.speed = HEAL_ITEM_DROP_SPEED;
                heal.active = true;
                healItems.push_back(heal); // Thêm heal item vào danh sách
                ally.droppingHeal = true; // Đánh dấu đã thả heal
            }
        }
    }

    // Cập nhật Vật phẩm Hồi máu (HealItem)
    for (auto& heal : healItems) {
        if (heal.active) {
            // Di chuyển xuống dưới
            heal.y += heal.speed * deltaTime;

            // Kiểm tra ra khỏi màn hình bên dưới
            if (heal.y > SCREEN_HEIGHT) {
                heal.active = false; // Đánh dấu không hoạt động
                continue;
            }

            // Kiểm tra va chạm với tàu người chơi
            if (CheckCollisionWithChitbox(heal)) {
                 HandleHealCollection(heal); // Gọi hàm xử lý nhặt heal
                 // Hàm HandleHealCollection sẽ đặt heal.active = false
            }
        }
    }


    // Cập nhật SpaceShark
    for (auto& ss : spaceSharks) {
        if (ss.active) {
            // Cập nhật vị trí theo quỹ đạo xoắn ốc
            ss.angle += ss.angularSpeed * deltaTime;
            ss.radius += SHARK_SPIRAL_SPEED * deltaTime;
            // Giữ bán kính không nhỏ hơn giá trị min
            if (ss.radius < SHARK_MIN_RADIUS) ss.radius = SHARK_MIN_RADIUS;
            ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
            ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);

            // Kiểm tra thời gian để bắn đạn
            if (currentTime - ss.lastBulletTime >= SHARK_BULLET_INTERVAL) {
                SharkBullet sb;
                sb.x = ss.x; sb.y = ss.y; // Đạn xuất phát từ vị trí cá mập
                // Tính vector hướng về tâm
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - sb.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - sb.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                // Tính tốc độ đạn
                float bulletSpeed = DEFAULT_MISSILE_SPEED * SHARK_BULLET_SPEED_MULTIPLIER;
                sb.dx = (distX / distance) * bulletSpeed;
                sb.dy = (distY / distance) * bulletSpeed;
                sb.active = true;
                sharkBullets.push_back(sb); // Thêm đạn vào danh sách
                ss.lastBulletTime = currentTime; // Cập nhật thời điểm bắn cuối
            }

            // Kiểm tra va chạm với tàu người chơi
            if (CheckCollisionWithChitbox(ss)) {
                ss.active = false; // Cá mập biến mất
                HandleHit(); // Người chơi mất máu
            }
            // Kiểm tra va chạm với khiên
            else if (CheckCollisionWithArc(ss)) {
                ss.active = false; // Cá mập biến mất
                score += SCORE_PER_SHARK; // Tăng điểm
                updateScoreTexture(); // Cập nhật hiển thị điểm
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); // Phát âm thanh đỡ
            }
            // Kiểm tra hết thời gian tồn tại
            else if (currentTime - ss.spawnTime >= SHARK_LIFETIME) {
                 ss.active = false; // Cá mập biến mất
            }
        }
    }

    // Cập nhật SharkBullet
    for (auto& sb : sharkBullets) {
        if (sb.active) {
            // Di chuyển đạn
            sb.x += sb.dx * deltaTime; sb.y += sb.dy * deltaTime;
            // Kiểm tra va chạm với tàu
            if (CheckCollisionWithChitbox(sb)) {
                sb.active = false; // Đạn biến mất
                HandleHit(); // Người chơi mất máu
            }
            // Kiểm tra va chạm với khiên
            else if (CheckCollisionWithArc(sb)) {
                sb.active = false; // Đạn biến mất
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); // Phát âm thanh đỡ
            }
            // Kiểm tra ra khỏi màn hình (thêm khoảng đệm)
            else if (sb.x < -SHARK_BULLET_WIDTH || sb.x > SCREEN_WIDTH + SHARK_BULLET_WIDTH ||
                       sb.y < -SHARK_BULLET_HEIGHT || sb.y > SCREEN_HEIGHT + SHARK_BULLET_HEIGHT) {
                sb.active = false; // Đạn biến mất
            }
        }
    }

    // Cập nhật Tên lửa thường (Target)
    for (auto& t : targets) {
        if (t.active) {
            // Di chuyển tên lửa
            t.x += t.dx * deltaTime; t.y += t.dy * deltaTime;
            // Kiểm tra va chạm với tàu
            if (CheckCollisionWithChitbox(t)) {
                t.active = false; // Tên lửa biến mất
                HandleHit(); // Người chơi mất máu
            }
            // Kiểm tra va chạm với khiên
            else if (CheckCollisionWithArc(t)) {
                t.active = false; // Tên lửa biến mất
                score += SCORE_PER_MISSILE; // Tăng điểm
                updateScoreTexture(); // Cập nhật hiển thị điểm
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); // Phát âm thanh đỡ
            }
             // Có thể thêm kiểm tra ra khỏi màn hình nếu cần
        }
    }

    // Cập nhật Tên lửa nhanh (Fast Missile)
    for (auto& fm : fastMissiles) {
        if (fm.active) {
            // Di chuyển tên lửa
            fm.x += fm.dx * deltaTime; fm.y += fm.dy * deltaTime;
            // Kiểm tra va chạm với tàu
            if (CheckCollisionWithChitbox(fm)) {
                fm.active = false; // Tên lửa biến mất
                HandleHit(); // Người chơi mất máu
            }
            // Kiểm tra va chạm với khiên
            else if (CheckCollisionWithArc(fm)) {
                fm.active = false; // Tên lửa biến mất
                score += SCORE_PER_FAST_MISSILE; // Tăng điểm
                updateScoreTexture(); // Cập nhật hiển thị điểm
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); // Phát âm thanh đỡ
            }
             // Có thể thêm kiểm tra ra khỏi màn hình nếu cần
        }
    }

    // --- Xóa các đối tượng không hoạt động ---
    // Sử dụng remove_if và erase để xóa hiệu quả
    targets.erase(std::remove_if(targets.begin(), targets.end(), [](const Target& t){ return !t.active; }), targets.end());
    fastMissiles.erase(std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](const Target& fm){ return !fm.active; }), fastMissiles.end());
    spaceSharks.erase(std::remove_if(spaceSharks.begin(), spaceSharks.end(), [](const SpaceShark& ss){ return !ss.active; }), spaceSharks.end());
    sharkBullets.erase(std::remove_if(sharkBullets.begin(), sharkBullets.end(), [](const SharkBullet& sb){ return !sb.active; }), sharkBullets.end());
    // --- XÓA ĐỐI TƯỢNG MỚI ---
    allies.erase(std::remove_if(allies.begin(), allies.end(), [](const AllyShip& a){ return !a.active; }), allies.end());
    healItems.erase(std::remove_if(healItems.begin(), healItems.end(), [](const HealItem& h){ return !h.active; }), healItems.end());
}

// Render game
void Game::render() {
    // Vẽ nền game trước
    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL); // Vẽ toàn màn hình
    } else {
        // Vẽ nền đen nếu không có ảnh nền
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    // --- Render khi đang chơi ---
    if (!gameOver && !paused) {
        // Vẽ tàu người chơi
        if (mspaceshipTexture) { SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox); }
        // Vẽ quỹ đạo và khiên
        SDL_SetRenderDrawColor(renderer, TRAJECTORY_CIRCLE_COLOR.r, TRAJECTORY_CIRCLE_COLOR.g, TRAJECTORY_CIRCLE_COLOR.b, TRAJECTORY_CIRCLE_COLOR.a);
        DrawCircle(renderer, trajectory); // Vẽ vòng tròn quỹ đạo
        DrawArc(renderer, trajectory, arcStartAngle, SHIELD_ARC_ANGLE); // Vẽ khiên

        // Vẽ biểu tượng mạng sống
        for (const auto& life : lives) {
            // Tạo hình tròn nhỏ cho mỗi mạng
            Circle lifeCircle = {life.x + LIFE_ICON_RADIUS, life.y + LIFE_ICON_RADIUS, LIFE_ICON_RADIUS};
            // Chọn màu dựa trên trạng thái (đỏ=mất, xanh=còn)
            const SDL_Color& color = life.isRed ? LIFE_ICON_INACTIVE_COLOR : LIFE_ICON_ACTIVE_COLOR;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            // Vẽ hình tròn đầy (fill circle) - cách đơn giản
            for (int w = 0; w < lifeCircle.r * 2; w++) {
                for (int h = 0; h < lifeCircle.r * 2; h++) {
                    int dx = lifeCircle.r - w; int dy = lifeCircle.r - h;
                    // Kiểm tra điểm có nằm trong vòng tròn không
                    if ((dx*dx + dy*dy) <= (lifeCircle.r * lifeCircle.r)) {
                        SDL_RenderDrawPoint(renderer, lifeCircle.x + dx - lifeCircle.r, lifeCircle.y + dy - lifeCircle.r);
                    }
                }
            }
        }

        // Render kẻ địch (gọi hàm render của lớp Enemy)
        for (const auto& t : targets) { enemy->renderTarget(t); }
        for (const auto& fm : fastMissiles) { enemy->renderFastMissile(fm); }
        for (const auto& ss : spaceSharks) { enemy->renderSpaceShark(ss); }
        for (const auto& sb : sharkBullets) { enemy->renderSharkBullet(sb); }

        // Render cảnh báo nếu đang hiển thị
        if (showWarning) {
            enemy->renderWarning(static_cast<float>(warningX), static_cast<float>(warningY), warningStartTime, startTime, totalPausedTime);
        }

        // --- RENDER ĐỐI TƯỢNG MỚI ---
        // Render Đồng minh (Ally)
        for (const auto& ally : allies) {
            if (ally.active && allyShipTexture) {
                SDL_Rect allyRect = { (int)ally.x, (int)ally.y, ALLY_WIDTH, ALLY_HEIGHT };
                SDL_RenderCopy(renderer, allyShipTexture, NULL, &allyRect);
            }
        }
        // Render Vật phẩm Hồi máu (HealItem)
        for (const auto& heal : healItems) {
            if (heal.active && healItemTexture) {
                SDL_Rect healRect = { (int)heal.x, (int)heal.y, HEAL_ITEM_WIDTH, HEAL_ITEM_HEIGHT };
                SDL_RenderCopy(renderer, healItemTexture, NULL, &healRect);
            }
        }


        // Render điểm số và điểm cao
        if (scoreTexture) {
            int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = {SCREEN_WIDTH - w - 10, 10, w, h}; // Góc trên phải
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
         if (highscoreTexture) {
            int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = {SCREEN_WIDTH - w - 10, 10 + h + 5, w, h}; // Dưới điểm số
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }
    }

    // Render nút Pause (luôn hiển thị khi không game over)
    if (!gameOver && pauseButtonTexture) { SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton); }

    // --- Render màn hình Game Over hoặc Pause ---
    // Hàm trợ giúp render texture vào giữa Rect
    auto renderTextureCentered = [&](SDL_Texture* texture, const SDL_Rect& rect) {
        if (!texture) {
            std::cerr << "Warning: Trying to render a null texture centered." << std::endl;
            // Vẽ placeholder màu đỏ nếu texture null
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &rect);
            return;
        }
        int w, h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
    };
     // Hàm trợ giúp render texture tại vị trí X, Y (căn giữa theo X)
    auto renderTextureAt = [&](SDL_Texture* texture, int x, int y, bool centerAlign = true) {
        if (!texture) {
             std::cerr << "Warning: Trying to render a null texture at (" << x << ", " << y << ")." << std::endl;
             // Vẽ placeholder nếu texture null
             SDL_Rect placeholder = { x - 50, y - 15, 100, 30};
             SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &placeholder);
             return;
        }
        int w, h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect destRect = { x, y, w, h };
        if (centerAlign) { destRect.x = x - w / 2; } // Chỉ căn giữa theo X
        // Không căn giữa theo Y để dễ đặt vị trí Y cố định
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
    };


    if (gameOver) {
        // Lớp phủ mờ màu đen
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Bật chế độ blend alpha
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Tắt blend

        // Render các yếu tố Game Over
        renderTextureAt(gameOverTextTexture, SCREEN_WIDTH / 2, GAMEOVER_TITLE_Y);
        renderTextureAt(scoreTexture, SCREEN_WIDTH / 2, SCORE_LABEL_Y);
        renderTextureAt(highscoreTexture, SCREEN_WIDTH / 2, HIGHSCORE_LABEL_Y);

        // Vẽ nút Restart
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &restartButton);
        renderTextureCentered(restartTexture, restartButton);

        // Vẽ nút Back to Menu
        SDL_RenderFillRect(renderer, &backToMenuButton);
        renderTextureCentered(backToMenuTexture, backToMenuButton);
    }
    else if (paused) {
        // Lớp phủ mờ màu đen
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Render các yếu tố Pause
        renderTextureAt(pausedTexture, SCREEN_WIDTH / 2, PAUSED_TITLE_Y);
        renderTextureAt(scoreTexture, SCREEN_WIDTH / 2, SCORE_LABEL_Y);
        renderTextureAt(highscoreTexture, SCREEN_WIDTH / 2, HIGHSCORE_LABEL_Y);

        // Render Volume Slider
        renderTextureAt(volumeLabelTexture, volumeSlider.x, VOLUME_LABEL_Y, false); // Canh trái
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider); // Vẽ nền slider
        const SDL_Color& knobColor = isDraggingVolume ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR; // Chọn màu núm
        SDL_SetRenderDrawColor(renderer, knobColor.r, knobColor.g, knobColor.b, knobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob); // Vẽ núm slider

        // Vẽ nút Give Up
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &giveUpButton);
        renderTextureCentered(giveUpTexture, giveUpButton);
    }

    SDL_RenderPresent(renderer); // Hiển thị tất cả những gì đã vẽ lên màn hình
}

// Reset trạng thái game về ban đầu
void Game::reset() {
    gameOver = false;
    paused = false;
    showWarning = false;
    warningStartTime = 0;
    warningX = 0; warningY = 0;
    targets.clear(); fastMissiles.clear(); spaceSharks.clear(); sharkBullets.clear();
    // --- RESET ĐỐI TƯỢNG MỚI ---
    allies.clear();
    healItems.clear();
    // Reset mạng sống về trạng thái ban đầu (tất cả đều xanh)
    for (auto& life : lives) life.isRed = false;
    missileCount = INITIAL_MISSILE_COUNT;
    waveCount = 0;
    score = 0;
    nextSpawnTime = INITIAL_SPAWN_DELAY; // Reset thời điểm spawn wave đầu
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    // --- RESET THỜI GIAN MỚI ---
    lastAllySpawnTime = 0; // Reset thời điểm spawn ally
    arcStartAngle = INITIAL_SHIELD_START_ANGLE; // Reset góc khiên
    // Reset số wave ngẫu nhiên cho lần tăng số lượng tên lửa tiếp theo
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
    startTime = 0; // Reset thời điểm bắt đầu game
    pauseStartTime = 0;
    totalPausedTime = 0;
    justStarted = false; // Reset cờ bắt đầu
    isDraggingVolume = false; // Reset trạng thái kéo volume
    // Cập nhật lại hiển thị điểm và điểm cao
    updateScoreTexture();
    updateHighscoreTexture();
    // Áp dụng lại cài đặt từ menu (có thể đã thay đổi)
    if (menu) {
        setVolume(menu->volume);
        setSensitivity(menu->sensitivity);
    }

    // Dừng nhạc và các hiệu ứng âm thanh đang chạy
    Mix_HaltMusic();
    Mix_HaltChannel(CHANNEL_WARNING); // Dừng kênh cảnh báo
    // Có thể dừng các kênh SFX khác nếu cần: Mix_HaltChannel(-1);
}

// Bắt đầu game mới
void Game::startGame() {
    startTime = SDL_GetTicks(); // Ghi lại thời điểm bắt đầu
    totalPausedTime = 0; // Reset tổng thời gian pause
    pauseStartTime = 0;
    justStarted = true; // Đánh dấu là vừa bắt đầu (cho logic spawn wave đầu)
    gameOver = false;
    paused = false;
    // Cập nhật hiển thị điểm và điểm cao khi bắt đầu
    updateScoreTexture();
    updateHighscoreTexture();

    // Dừng nhạc hiện tại (nếu có) và bắt đầu nhạc game
    Mix_HaltMusic();
    if (bgmGame) {
        Mix_PlayMusic(bgmGame, -1); // Phát nhạc game lặp lại
    } else {
        std::cerr << "Warning: Game BGM not loaded, cannot play." << std::endl;
    }
}

// --- Hàm kiểm tra va chạm ---
// Va chạm giữa tên lửa thường/nhanh và khiên
bool Game::CheckCollisionWithArc(const Target& t) {
    if (!t.active) return false; // Bỏ qua nếu không hoạt động
    float targetCenterX = t.x; float targetCenterY = t.y; // Giả sử tâm va chạm là vị trí t.x, t.y
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy; // Bình phương khoảng cách từ tâm đến tên lửa

    // Bán kính va chạm của tên lửa (lấy từ config, hoặc đặt giá trị nhỏ)
    float collisionRadius = sqrt(MISSILE_COLLISION_RADIUS_SQ); // Hoặc FAST_MISSILE_COLLISION_RADIUS_SQ

    // Tính bình phương bán kính ngoài và trong của vùng va chạm khiên
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius);
    if (innerRadiusSq < 0) innerRadiusSq = 0; // Đảm bảo không âm

    // Kiểm tra nhanh: Nếu khoảng cách quá xa hoặc quá gần tâm, không thể va chạm khiên
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;

    // Tính góc của tên lửa so với tâm
    float targetAngle = atan2(dy, dx);
    // Chuẩn hóa góc về 0 -> 2*PI
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;

    // Kiểm tra xem góc của tên lửa có nằm trong góc của khiên không
    // Trường hợp 1: Khiên không vắt qua điểm 0 radian
    if (normalizedArcStart <= normalizedArcEnd) {
        return (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd);
    }
    // Trường hợp 2: Khiên vắt qua điểm 0 radian
    else {
        return (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd);
    }
    // Nếu cả khoảng cách và góc đều khớp -> va chạm
    return false; // Mặc định không va chạm
}
// Va chạm giữa tên lửa thường/nhanh và tàu người chơi (dùng hitbox chữ nhật)
bool Game::CheckCollisionWithChitbox(const Target& t) {
     if (!t.active) return false;
     // Tạo một Rect nhỏ đại diện cho tên lửa (có thể điều chỉnh kích thước)
     SDL_Rect targetRect = { (int)(t.x - 2), (int)(t.y - 2), 5, 5 };
    // Sử dụng hàm kiểm tra giao nhau của SDL
    return SDL_HasIntersection(&targetRect, &chitbox);
}
// Va chạm giữa Space Shark và khiên
bool Game::CheckCollisionWithArc(const SpaceShark& ss) {
    if (!ss.active) return false;
    float targetCenterX = ss.x; float targetCenterY = ss.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float collisionRadius = sqrt(SHARK_COLLISION_RADIUS_SQ); // Bán kính va chạm của cá mập
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius); if (innerRadiusSq < 0) innerRadiusSq = 0;
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;
    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;
    if (normalizedArcStart <= normalizedArcEnd) { return (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd); }
    else { return (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd); }
    return false;
}
// Va chạm giữa Space Shark và tàu người chơi
bool Game::CheckCollisionWithChitbox(const SpaceShark& ss) {
    if (!ss.active) return false;
    // Tạo Rect dựa trên vị trí và kích thước của cá mập (đã trừ tâm xoay)
    SDL_Rect sharkRect = { (int)(ss.x - SHARK_CENTER.x), (int)(ss.y - SHARK_CENTER.y), SHARK_WIDTH, SHARK_HEIGHT };
    return SDL_HasIntersection(&sharkRect, &chitbox);
}
// Va chạm giữa Shark Bullet và khiên
bool Game::CheckCollisionWithArc(const SharkBullet& sb) {
    if (!sb.active) return false;
    float targetCenterX = sb.x; float targetCenterY = sb.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float collisionRadius = sqrt(SHARK_BULLET_COLLISION_RADIUS_SQ); // Bán kính va chạm đạn cá mập
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius); if (innerRadiusSq < 0) innerRadiusSq = 0;
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;
    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;
    if (normalizedArcStart <= normalizedArcEnd) { return (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd); }
    else { return (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd); }
    return false;
}
// Va chạm giữa Shark Bullet và tàu người chơi
bool Game::CheckCollisionWithChitbox(const SharkBullet& sb) {
    if (!sb.active) return false;
    // Tạo Rect dựa trên vị trí và kích thước đạn (đã trừ tâm xoay)
    SDL_Rect bulletRect = { (int)(sb.x - SHARK_BULLET_CENTER.x), (int)(sb.y - SHARK_BULLET_CENTER.y), SHARK_BULLET_WIDTH, SHARK_BULLET_HEIGHT };
    return SDL_HasIntersection(&bulletRect, &chitbox);
}

// --- VA CHẠM MỚI ---
// Va chạm giữa Heal Item và tàu người chơi
bool Game::CheckCollisionWithChitbox(const HealItem& hi) {
    if (!hi.active) return false;
    // Tạo Rect cho Heal Item (không cần trừ tâm xoay nếu ảnh căn từ góc trái trên)
    SDL_Rect healRect = { (int)hi.x, (int)hi.y, HEAL_ITEM_WIDTH, HEAL_ITEM_HEIGHT };
    return SDL_HasIntersection(&healRect, &chitbox);
}


// --- Hàm tiện ích vẽ hình ---
// Vẽ hình tròn (chỉ viền)
void Game::DrawCircle(SDL_Renderer* renderer, const Circle& c) {
    SDL_Point points[CIRCLE_SEGMENTS + 1];
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float rad = (2.0f * PI * i) / CIRCLE_SEGMENTS;
        points[i].x = c.x + static_cast<int>(c.r * cos(rad));
        points[i].y = c.y + static_cast<int>(c.r * sin(rad));
    }
    SDL_RenderDrawLines(renderer, points, CIRCLE_SEGMENTS + 1); // Vẽ các đường nối các điểm
}
// Vẽ cung tròn (khiên)
void Game::DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle) {
    // Đặt màu vẽ cho khiên
    SDL_SetRenderDrawColor(renderer, SHIELD_ARC_COLOR.r, SHIELD_ARC_COLOR.g, SHIELD_ARC_COLOR.b, SHIELD_ARC_COLOR.a);
    SDL_Point points[ARC_SEGMENTS + 1];
    for (int i = 0; i <= ARC_SEGMENTS; ++i) {
        double angle = startAngle + (arcAngle * i / ARC_SEGMENTS); // Tính góc cho từng điểm
        points[i].x = c.x + static_cast<int>(c.r * cos(angle));
        points[i].y = c.y + static_cast<int>(c.r * sin(angle));
    }
    SDL_RenderDrawLines(renderer, points, ARC_SEGMENTS + 1); // Vẽ các đường nối các điểm của cung
}

// Xử lý khi người chơi bị bắn trúng
void Game::HandleHit() {
    if (gameOver) return; // Không xử lý nếu đã game over
    // Phát âm thanh bị bắn
    if (sfxPlayerHit) Mix_PlayChannel(CHANNEL_SFX, sfxPlayerHit, 0);
    // Tìm mạng sống đầu tiên còn xanh và biến nó thành đỏ
    for (auto& life : lives) {
        if (!life.isRed) {
            life.isRed = true;
            goto checkGameOver; // Nhảy đến phần kiểm tra game over
        }
    }
// Nhãn để nhảy đến sau khi giảm mạng
checkGameOver:
    // Kiểm tra xem tất cả mạng sống đã đỏ chưa
    bool allRed = true;
    for (const auto& life : lives) {
        if (!life.isRed) {
            allRed = false; // Nếu còn mạng xanh thì chưa hết máu
            break;
        }
    }
    // Nếu tất cả đã đỏ -> Game Over
    if (allRed) {
        triggerGameOver(); // Gọi hàm xử lý game over
        if (menu) menu->gameState = MainMenu::GAME_OVER; // Cập nhật trạng thái menu
    }
}

// --- XỬ LÝ MỚI ---
// Xử lý khi nhặt được Heal Item
void Game::HandleHealCollection(HealItem& heal) {
    if (!heal.active) return; // Bỏ qua nếu heal không hoạt động

    heal.active = false; // Đánh dấu heal không hoạt động (biến mất)

    // Phát âm thanh nhặt heal
    if (sfxHealCollect) {
        Mix_PlayChannel(CHANNEL_SFX, sfxHealCollect, 0);
    }

    // Tìm mạng sống đầu tiên đang đỏ và biến nó thành xanh (hồi máu)
    for (auto& life : lives) {
        if (life.isRed) {
            life.isRed = false; // Hồi lại mạng
            break; // Chỉ hồi 1 mạng mỗi lần nhặt
        }
    }
    // Lưu ý: Logic này chỉ hồi mạng đã mất, không tăng số mạng tối đa.
}

// Hàm tạo đồng minh mới
void Game::SpawnAlly() {
    AllyShip ally;
    ally.x = 0.0f - ALLY_WIDTH; // Xuất hiện từ bên trái, ngoài màn hình
    ally.y = 10.0f;           // Xuất hiện ở gần đỉnh màn hình (có thể random Y)
    ally.speed = ALLY_SPEED;
    ally.active = true;
    ally.droppingHeal = false; // Ban đầu chưa thả heal
    allies.push_back(ally);    // Thêm vào danh sách đồng minh
}


// Setter cho volume (đồng thời cập nhật vị trí núm và âm lượng Mixer)
void Game::setVolume(int vol) {
    if (vol >= 0 && vol <= 100) {
        volume = vol;
        // Cập nhật vị trí núm volume trên thanh trượt
        int knobRange = volumeSlider.w - volumeKnob.w;
        volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRange));
        // Cập nhật âm lượng thực tế của nhạc và SFX
        Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100); // Âm lượng nhạc nền
        Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100); // Âm lượng tất cả SFX (-1)
    }
 }

// Hàm cho MainMenu quản lý trạng thái: Chuyển sang Playing
void Game::setGameStatePlaying() {
     if (paused) { // Chỉ thực hiện nếu đang pause
        // Tính thời gian đã pause và cộng vào tổng thời gian pause
        if (pauseStartTime != 0) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
        }
        paused = false; // Tắt cờ pause
        pauseStartTime = 0; // Reset thời điểm bắt đầu pause
        Mix_ResumeMusic(); // Tiếp tục nhạc nền
        // Nếu có cảnh báo đang hiển thị khi pause, phát lại âm thanh cảnh báo
        if (showWarning) {
             if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1);
        }
     }
}
// Hàm cho MainMenu quản lý trạng thái: Chuyển sang Paused
void Game::setGameStatePaused() {
    if (!paused && !gameOver) { // Chỉ thực hiện nếu đang chơi và chưa game over
        pauseStartTime = SDL_GetTicks(); // Ghi lại thời điểm bắt đầu pause
        paused = true; // Bật cờ pause
         Mix_PauseMusic(); // Tạm dừng nhạc nền
         Mix_HaltChannel(CHANNEL_WARNING); // Dừng âm thanh cảnh báo (nếu đang phát)
    }
}
// Hàm kích hoạt trạng thái Game Over
void Game::triggerGameOver() {
    if (!gameOver) { // Chỉ thực hiện một lần
         gameOver = true; // Bật cờ game over
         Mix_HaltMusic(); // Dừng nhạc nền
         Mix_HaltChannel(CHANNEL_WARNING); // Dừng âm thanh cảnh báo
         // Phát âm thanh game over
         if (sfxGameOver) Mix_PlayChannel(CHANNEL_SFX, sfxGameOver, 0);
         // Lưu điểm cao mới vào MainMenu và file
         if (menu) menu->saveHighscores(score);
         // Cập nhật lại hiển thị điểm và điểm cao (có thể điểm cao đã thay đổi)
         updateScoreTexture();
         updateHighscoreTexture();
         // Nếu đang pause khi game over, cần cập nhật lại totalPausedTime
         if(paused) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            pauseStartTime = 0;
            paused = false; // Không còn ở trạng thái pause nữa
         }
    }
}

// Setter cho sensitivity
void Game::setSensitivity(int sens) {
    if (sens >= 0 && sens <= 100) {
        sensitivity = sens;
        // Không cần cập nhật gì thêm ở đây vì sensitivity được dùng trực tiếp trong update()
    }
}

// --- THÊM ĐỊNH NGHĨA GETTER MỚI ---
// Trả về giá trị của biến private isDraggingVolume
bool Game::isDraggingVolumeSlider() const {
    return isDraggingVolume;
}
