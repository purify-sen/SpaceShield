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
std::uniform_real_distribution<> dis(0.0, 1.0);
std::uniform_int_distribution<> dist_wave_increase(0, RANDOM_WAVES_UNTIL_INCREASE -1);
std::uniform_int_distribution<> dist_wave_delay(0, RANDOM_WAVE_DELAY -1);
std::uniform_int_distribution<> dist_side(0, 3);
std::uniform_int_distribution<> dist_y_spawn(0, SCREEN_HEIGHT - 1);
std::uniform_int_distribution<> dist_x_spawn(0, SCREEN_WIDTH - 1);

// --- Hàm trợ giúp load texture (ĐỊNH NGHĨA Ở ĐÂY) ---
// Hàm này bây giờ là toàn cục, không thuộc lớp Game
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
Game::Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
           Mix_Chunk* sfxShieldHitIn, Mix_Chunk* sfxPlayerHitIn,
           Mix_Chunk* sfxGameOverIn, Mix_Chunk* sfxWarningIn, Mix_Music* bgmGameIn,
           SDL_Texture* bgTexture)
    : renderer(r), enemy(e), menu(m),
      // Textures
      mspaceshipTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr),
      highscoreTexture(nullptr), pausedTexture(nullptr), backToMenuTexture(nullptr),
      restartTexture(nullptr), gameOverTextTexture(nullptr), volumeLabelTexture(nullptr),
      giveUpTexture(nullptr), backgroundTexture(bgTexture),
      // Sounds
      sfxShieldHit(sfxShieldHitIn), sfxPlayerHit(sfxPlayerHitIn),
      sfxGameOver(sfxGameOverIn), sfxWarning(sfxWarningIn), bgmGame(bgmGameIn),
      // State
      gameOver(false), paused(false), showWarning(false),
      justStarted(false),
      // Timing
      startTime(0), pauseStartTime(0), totalPausedTime(0), warningStartTime(0),
      // Score, Wave
      score(0), missileCount(INITIAL_MISSILE_COUNT), waveCount(0),
      nextSpawnTime(INITIAL_SPAWN_DELAY), lastMissileSpawnTime(0), spawnedMissilesInWave(0),
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
    if (!mspaceshipTexture || !pauseButtonTexture) {
        std::cerr << "Error loading essential game textures!" << std::endl;
    }

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
        return;
    }
    std::cout << "Fonts opened successfully in Game::initTextures." << std::endl;


    // Hàm trợ giúp tạo texture (thêm kiểm tra lỗi)
    auto createTexture = [&](const char* text, SDL_Texture*& texture, TTF_Font* fontToUse) {
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
        std::cout << "Successfully created texture for: " << text << std::endl;
        return true;
    };

    // Tạo textures và kiểm tra lỗi
    if (!createTexture("Back to Menu", backToMenuTexture, fontLarge)) { std::cerr << "Error creating back to menu texture." << std::endl; }
    if (!createTexture("Restart", restartTexture, fontLarge)) { std::cerr << "Error creating restart texture." << std::endl; }
    if (!createTexture("Give Up", giveUpTexture, fontLarge)) { std::cerr << "Error creating give up texture." << std::endl; }
    if (!createTexture("Paused", pausedTexture, fontLarge)) { std::cerr << "Error creating paused texture." << std::endl; }
    if (!createTexture("Game over", gameOverTextTexture, fontXLarge)) { std::cerr << "Error creating game over texture." << std::endl; }
    if (!createTexture("Volume", volumeLabelTexture, fontNormal)) { std::cerr << "Error creating volume label texture." << std::endl; }

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
    int highscore = menu->highscores.empty() ? 0 : menu->highscores[0];
    ss << "Highscore: " << highscore;
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, ss.str().c_str(), TEXT_COLOR);
    TTF_CloseFont(font); // Đóng font ngay
    if (!textSurface) { std::cerr << "Failed to render highscore text: " << TTF_GetError() << std::endl; return; }
    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
     if (!highscoreTexture) { std::cerr << "Failed to create highscore texture: " << SDL_GetError() << std::endl; }
}

void Game::updatePausedTexture() { /* Không cần cập nhật vì text cố định */ }
void Game::updateGameOverTextTexture() { /* Không cần cập nhật vì text cố định */ }
void Game::updateVolumeLabelTexture() { /* Không cần cập nhật vì text cố định */ }

// Xử lý input trong game
void Game::handleInput(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        // Click nút Pause
        if (SDL_PointInRect(&mousePoint, &pauseButton)) {
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            if (!paused && !gameOver) { setGameStatePaused(); menu->gameState = MainMenu::PAUSED; }
            else if (paused) { setGameStatePlaying(); menu->gameState = MainMenu::PLAYING; }
        }

        // Khi đang Pause
        if (paused) {
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                isDraggingVolume = true;
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            }
            if (SDL_PointInRect(&mousePoint, &giveUpButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                triggerGameOver();
                menu->gameState = MainMenu::GAME_OVER;
            }
        }

        // Khi Game Over
        if (gameOver) {
            if (SDL_PointInRect(&mousePoint, &backToMenuButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset();
                menu->gameState = MainMenu::MENU;
                 Mix_HaltMusic();
                 if (menu->bgmMenu) Mix_PlayMusic(menu->bgmMenu, -1);
            }
            if (SDL_PointInRect(&mousePoint, &restartButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset();
                startGame();
                menu->gameState = MainMenu::PLAYING;
            }
        }
    }
    // Thả chuột
    if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingVolume) {
            isDraggingVolume = false;
            menu->volume = volume;
            menu->saveSettings();
        }
     }
    // Kéo chuột
    if (event.type == SDL_MOUSEMOTION && isDraggingVolume) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        int newKnobX = mouseX - volumeKnob.w / 2;
        newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
        volumeKnob.x = newKnobX;
        int newVolume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / (volumeSlider.w - volumeKnob.w)) * 100.0f));
        newVolume = std::max(0, std::min(newVolume, 100));
        setVolume(newVolume);
     }
    // Phím ESC
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (!paused && !gameOver) {
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            setGameStatePaused();
            menu->gameState = MainMenu::PAUSED;
        } else if (paused) {
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            setGameStatePlaying();
            menu->gameState = MainMenu::PLAYING;
        }
     }
}

// Cập nhật logic game
void Game::update(float deltaTime) {
    if (gameOver || startTime == 0 || paused) return;

    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    // Cập nhật điều khiển khiên
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    float sensitivityFactor = MIN_SENSITIVITY_MULTIPLIER + (static_cast<float>(sensitivity) / 100.0f) * (MAX_SENSITIVITY_MULTIPLIER - MIN_SENSITIVITY_MULTIPLIER);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;

    // --- Spawn Kẻ địch ---
    // Spawn SpaceShark
    if (waveCount >= WAVE_START_SHARK && (waveCount - WAVE_START_SHARK) % WAVE_INTERVAL_SHARK == 0 && spaceSharks.empty()) {
        SpaceShark ss;
        ss.radius = SHARK_INITIAL_RADIUS;
        ss.angle = static_cast<float>(dis(gen)) * 2.0f * PI;
        ss.angularSpeed = SHARK_ANGULAR_SPEED;
        ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
        ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);
        ss.spawnTime = currentTime;
        ss.lastBulletTime = currentTime;
        ss.active = true;
        spaceSharks.push_back(ss);
    }

    // Spawn Tên lửa nhanh (Fast Missile) - Quản lý cảnh báo
    if (waveCount >= WAVE_START_FAST_MISSILE && (waveCount - WAVE_START_FAST_MISSILE) % WAVE_INTERVAL_FAST_MISSILE == 0 && fastMissiles.empty() && !showWarning) {
        showWarning = true;
        warningStartTime = currentTime;
         if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1);
        int side = dist_side(gen);
        switch (side) {
            case 0: warningX = LIFE_ICON_RADIUS; warningY = dist_y_spawn(gen); break;
            case 1: warningX = SCREEN_WIDTH - LIFE_ICON_RADIUS; warningY = dist_y_spawn(gen); break;
            case 2: warningX = dist_x_spawn(gen); warningY = LIFE_ICON_RADIUS; break;
            case 3: warningX = dist_x_spawn(gen); warningY = SCREEN_HEIGHT - LIFE_ICON_RADIUS; break;
        }
    }

    // Kích hoạt tên lửa nhanh sau khi cảnh báo
    if (showWarning && (currentTime - warningStartTime >= FAST_MISSILE_WARNING_DURATION)) {
        showWarning = false;
         Mix_HaltChannel(CHANNEL_WARNING);
        Target fm;
        fm.x = static_cast<float>(warningX);
        fm.y = static_cast<float>(warningY);
        float distX = static_cast<float>(TRAJECTORY_CENTER.x) - fm.x;
        float distY = static_cast<float>(TRAJECTORY_CENTER.y) - fm.y;
        float distance = sqrt(distX * distX + distY * distY);
        if (distance < 1e-6f) distance = 1.0f;
        float baseSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
        float missileSpeed = baseSpeed * FAST_MISSILE_SPEED_MULTIPLIER;
        fm.dx = (distX / distance) * missileSpeed;
        fm.dy = (distY / distance) * missileSpeed;
        fm.active = true;
        fastMissiles.push_back(fm);
    }

    // Spawn Tên lửa thường (Target) và chuyển Wave
    bool waveAdvanced = false;
    if (!justStarted && currentTime >= nextSpawnTime) {
        if (spawnedMissilesInWave < missileCount) {
            if (currentTime - lastMissileSpawnTime >= MISSILE_SPAWN_INTERVAL || spawnedMissilesInWave == 0) {
                Target t;
                int side = dist_side(gen);
                 switch (side) {
                    case 0: t.x = 0.0f; t.y = static_cast<float>(dist_y_spawn(gen)); break;
                    case 1: t.x = static_cast<float>(SCREEN_WIDTH); t.y = static_cast<float>(dist_y_spawn(gen)); break;
                    case 2: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = 0.0f; break;
                    case 3: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = static_cast<float>(SCREEN_HEIGHT); break;
                }
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - t.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - t.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                float missileSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
                t.dx = (distX / distance) * missileSpeed;
                t.dy = (distY / distance) * missileSpeed;
                t.active = true;
                targets.push_back(t);
                spawnedMissilesInWave++;
                lastMissileSpawnTime = currentTime;
            }
        }
        else if (!waveAdvanced) {
            waveCount++;
            if (waveCount % wavesUntilIncrease == 0) {
                missileCount++;
                if (missileCount > MAX_MISSILE_COUNT) missileCount = MAX_MISSILE_COUNT;
                wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
            }
            nextSpawnTime = currentTime + BASE_WAVE_DELAY + dist_wave_delay(gen);
            spawnedMissilesInWave = 0;
            waveAdvanced = true;
        }
    }
    if (justStarted) justStarted = false;

    // --- Cập nhật vị trí và trạng thái Kẻ địch ---
    // Cập nhật SpaceShark
    for (auto& ss : spaceSharks) {
        if (ss.active) {
            ss.angle += ss.angularSpeed * deltaTime;
            ss.radius += SHARK_SPIRAL_SPEED * deltaTime;
            if (ss.radius < SHARK_MIN_RADIUS) ss.radius = SHARK_MIN_RADIUS;
            ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
            ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);

            if (currentTime - ss.lastBulletTime >= SHARK_BULLET_INTERVAL) {
                SharkBullet sb;
                sb.x = ss.x; sb.y = ss.y;
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - sb.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - sb.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                float bulletSpeed = DEFAULT_MISSILE_SPEED * SHARK_BULLET_SPEED_MULTIPLIER;
                sb.dx = (distX / distance) * bulletSpeed;
                sb.dy = (distY / distance) * bulletSpeed;
                sb.active = true;
                sharkBullets.push_back(sb);
                ss.lastBulletTime = currentTime;
            }

            if (CheckCollisionWithChitbox(ss)) {
                ss.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(ss)) {
                ss.active = false;
                score += SCORE_PER_SHARK;
                updateScoreTexture();
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            } else if (currentTime - ss.spawnTime >= SHARK_LIFETIME) {
                 ss.active = false;
            }
        }
    }

    // Cập nhật SharkBullet
    for (auto& sb : sharkBullets) {
        if (sb.active) {
            sb.x += sb.dx * deltaTime; sb.y += sb.dy * deltaTime;
            if (CheckCollisionWithChitbox(sb)) {
                sb.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(sb)) {
                sb.active = false;
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            } else if (sb.x < -SHARK_BULLET_WIDTH || sb.x > SCREEN_WIDTH + SHARK_BULLET_WIDTH ||
                       sb.y < -SHARK_BULLET_HEIGHT || sb.y > SCREEN_HEIGHT + SHARK_BULLET_HEIGHT) {
                sb.active = false;
            }
        }
    }

    // Cập nhật Tên lửa thường (Target)
    for (auto& t : targets) {
        if (t.active) {
            t.x += t.dx * deltaTime; t.y += t.dy * deltaTime;
            if (CheckCollisionWithChitbox(t)) {
                t.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(t)) {
                t.active = false;
                score += SCORE_PER_MISSILE;
                updateScoreTexture();
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            }
        }
    }

    // Cập nhật Tên lửa nhanh (Fast Missile)
    for (auto& fm : fastMissiles) {
        if (fm.active) {
            fm.x += fm.dx * deltaTime; fm.y += fm.dy * deltaTime;
            if (CheckCollisionWithChitbox(fm)) {
                fm.active = false;
                HandleHit();
            } else if (CheckCollisionWithArc(fm)) {
                fm.active = false;
                score += SCORE_PER_FAST_MISSILE;
                updateScoreTexture();
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            }
        }
    }

    // --- Xóa các đối tượng không hoạt động ---
    targets.erase(std::remove_if(targets.begin(), targets.end(), [](const Target& t){ return !t.active; }), targets.end());
    fastMissiles.erase(std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](const Target& fm){ return !fm.active; }), fastMissiles.end());
    spaceSharks.erase(std::remove_if(spaceSharks.begin(), spaceSharks.end(), [](const SpaceShark& ss){ return !ss.active; }), spaceSharks.end());
    sharkBullets.erase(std::remove_if(sharkBullets.begin(), sharkBullets.end(), [](const SharkBullet& sb){ return !sb.active; }), sharkBullets.end());
}

// Render game
void Game::render() {
    // Vẽ nền game trước
    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    // --- Render khi đang chơi ---
    if (!gameOver && !paused) {
        if (mspaceshipTexture) { SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox); }
        SDL_SetRenderDrawColor(renderer, TRAJECTORY_CIRCLE_COLOR.r, TRAJECTORY_CIRCLE_COLOR.g, TRAJECTORY_CIRCLE_COLOR.b, TRAJECTORY_CIRCLE_COLOR.a);
        DrawCircle(renderer, trajectory);
        DrawArc(renderer, trajectory, arcStartAngle, SHIELD_ARC_ANGLE);

        for (const auto& life : lives) {
            Circle lifeCircle = {life.x + LIFE_ICON_RADIUS, life.y + LIFE_ICON_RADIUS, LIFE_ICON_RADIUS};
            const SDL_Color& color = life.isRed ? LIFE_ICON_INACTIVE_COLOR : LIFE_ICON_ACTIVE_COLOR;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            for (int w = 0; w < lifeCircle.r * 2; w++) {
                for (int h = 0; h < lifeCircle.r * 2; h++) {
                    int dx = lifeCircle.r - w; int dy = lifeCircle.r - h;
                    if ((dx*dx + dy*dy) <= (lifeCircle.r * lifeCircle.r)) {
                        SDL_RenderDrawPoint(renderer, lifeCircle.x + dx, lifeCircle.y + dy);
                    }
                }
            }
        }

        for (const auto& t : targets) { enemy->renderTarget(t); }
        for (const auto& fm : fastMissiles) { enemy->renderFastMissile(fm); }
        for (const auto& ss : spaceSharks) { enemy->renderSpaceShark(ss); }
        for (const auto& sb : sharkBullets) { enemy->renderSharkBullet(sb); }

        if (showWarning) { enemy->renderWarning(static_cast<float>(warningX), static_cast<float>(warningY), warningStartTime, startTime, totalPausedTime); }

        if (scoreTexture) {
            int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = {SCREEN_WIDTH - w - 10, 60, w, h};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
         if (highscoreTexture) {
            int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = {SCREEN_WIDTH - w - 10, 60 + h + 5, w, h};
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }
    }

    // Render nút Pause
    if (!gameOver && pauseButtonTexture) { SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton); }

    // --- Render màn hình Game Over hoặc Pause ---
    if (gameOver) {
        // Lớp phủ mờ
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Render các yếu tố Game Over
        if (gameOverTextTexture) {
            int w, h; SDL_QueryTexture(gameOverTextTexture, NULL, NULL, &w, &h);
            SDL_Rect gameOverRect = { (SCREEN_WIDTH - w) / 2, GAMEOVER_TITLE_Y, w, h };
            SDL_RenderCopy(renderer, gameOverTextTexture, NULL, &gameOverRect);
        } else { std::cerr << "Warning: gameOverTextTexture is null in render()" << std::endl; }

        if (scoreTexture) {
             int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
             SDL_Rect scoreRect = { (SCREEN_WIDTH - w) / 2, SCORE_LABEL_Y, w, h };
             SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        } else { std::cerr << "Warning: scoreTexture is null in render()" << std::endl; }

        if (highscoreTexture) {
             int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
             SDL_Rect highscoreRect = { (SCREEN_WIDTH - w) / 2, HIGHSCORE_LABEL_Y, w, h };
             SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        } else { std::cerr << "Warning: highscoreTexture is null in render()" << std::endl; }

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &restartButton);
        if (restartTexture) {
             int w, h; SDL_QueryTexture(restartTexture, NULL, NULL, &w, &h);
             SDL_Rect textRect = { restartButton.x + (restartButton.w - w) / 2, restartButton.y + (restartButton.h - h) / 2, w, h };
             SDL_RenderCopy(renderer, restartTexture, NULL, &textRect);
        } else { std::cerr << "Warning: restartTexture is null in render()" << std::endl; }

        SDL_RenderFillRect(renderer, &backToMenuButton);
        if (backToMenuTexture) {
             int w, h; SDL_QueryTexture(backToMenuTexture, NULL, NULL, &w, &h);
             SDL_Rect textRect = { backToMenuButton.x + (backToMenuButton.w - w) / 2, backToMenuButton.y + (backToMenuButton.h - h) / 2, w, h };
             SDL_RenderCopy(renderer, backToMenuTexture, NULL, &textRect);
        } else { std::cerr << "Warning: backToMenuTexture is null in render()" << std::endl; }
    }
    else if (paused) {
        // Lớp phủ mờ
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Render các yếu tố Pause
        // *** Thêm kiểm tra texture và vẽ placeholder nếu cần ***
        if (pausedTexture) {
            int w, h; SDL_QueryTexture(pausedTexture, NULL, NULL, &w, &h);
            SDL_Rect pausedRect = { (SCREEN_WIDTH - w) / 2, PAUSED_TITLE_Y, w, h };
            SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
        } else {
            std::cerr << "Warning: pausedTexture is null in render()" << std::endl;
            // Vẽ placeholder
            SDL_Rect placeholder = {(SCREEN_WIDTH - 100) / 2, PAUSED_TITLE_Y, 100, 40};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &placeholder);
        }

        if (scoreTexture) {
             int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
             SDL_Rect scoreRect = { (SCREEN_WIDTH - w) / 2, SCORE_LABEL_Y, w, h };
             SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        } else { std::cerr << "Warning: scoreTexture is null in render()" << std::endl; }

        if (highscoreTexture) {
            int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = { (SCREEN_WIDTH - w) / 2, HIGHSCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        } else { std::cerr << "Warning: highscoreTexture is null in render()" << std::endl; }

        if (volumeLabelTexture) {
             int w, h; SDL_QueryTexture(volumeLabelTexture, NULL, NULL, &w, &h);
             SDL_Rect labelRect = { volumeSlider.x, VOLUME_LABEL_Y, w, h };
             SDL_RenderCopy(renderer, volumeLabelTexture, NULL, &labelRect);
        } else {
            std::cerr << "Warning: volumeLabelTexture is null in render()" << std::endl;
            SDL_Rect placeholder = {volumeSlider.x, VOLUME_LABEL_Y, 80, 20};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &placeholder);
        }

        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider);
        const SDL_Color& knobColor = isDraggingVolume ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, knobColor.r, knobColor.g, knobColor.b, knobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob);

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &giveUpButton);
        if (giveUpTexture) {
             int w, h; SDL_QueryTexture(giveUpTexture, NULL, NULL, &w, &h);
             SDL_Rect textRect = { giveUpButton.x + (giveUpButton.w - w) / 2, giveUpButton.y + (giveUpButton.h - h) / 2, w, h };
             SDL_RenderCopy(renderer, giveUpTexture, NULL, &textRect);
        } else {
            std::cerr << "Warning: giveUpTexture is null in render()" << std::endl;
             SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &giveUpButton);
        }
    }

    SDL_RenderPresent(renderer);
}

// Reset trạng thái game
void Game::reset() {
    gameOver = false;
    paused = false;
    showWarning = false;
    warningStartTime = 0;
    warningX = 0; warningY = 0;
    targets.clear(); fastMissiles.clear(); spaceSharks.clear(); sharkBullets.clear();
    for (auto& life : lives) life.isRed = false;
    missileCount = INITIAL_MISSILE_COUNT;
    waveCount = 0;
    score = 0;
    nextSpawnTime = INITIAL_SPAWN_DELAY;
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    arcStartAngle = INITIAL_SHIELD_START_ANGLE;
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
    startTime = 0;
    pauseStartTime = 0;
    totalPausedTime = 0;
    justStarted = false;
    updateScoreTexture();
    updateHighscoreTexture();
    setVolume(menu->volume);
    setSensitivity(menu->sensitivity);

    Mix_HaltMusic();
    Mix_HaltChannel(CHANNEL_WARNING);
}

// Bắt đầu game mới
void Game::startGame() {
    startTime = SDL_GetTicks();
    totalPausedTime = 0;
    pauseStartTime = 0;
    justStarted = true;
    gameOver = false;
    paused = false;
    updateScoreTexture();
    updateHighscoreTexture();

    Mix_HaltMusic();
    if (bgmGame) {
        Mix_PlayMusic(bgmGame, -1);
    }
}

// --- Hàm kiểm tra va chạm ---
bool Game::CheckCollisionWithArc(const Target& t) {
    float targetCenterX = t.x; float targetCenterY = t.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float outerRadiusSq = (trajectory.r + 10.0f) * (trajectory.r + 10.0f);
    float innerRadiusSq = (trajectory.r - 10.0f) * (trajectory.r - 10.0f);
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;
    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;
    if (normalizedArcStart <= normalizedArcEnd) { if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) return distSq <= outerRadiusSq && distSq >= innerRadiusSq; }
    else { if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) return distSq <= outerRadiusSq && distSq >= innerRadiusSq; }
    return false;
}
bool Game::CheckCollisionWithChitbox(const Target& t) {
     SDL_Rect targetRect = { (int)t.x - 2, (int)t.y - 2, 5, 5 };
    return SDL_HasIntersection(&targetRect, &chitbox);
}
bool Game::CheckCollisionWithArc(const SpaceShark& ss) {
    float targetCenterX = ss.x; float targetCenterY = ss.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float collisionRadius = sqrt(SHARK_COLLISION_RADIUS_SQ);
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius); if (innerRadiusSq < 0) innerRadiusSq = 0;
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;
    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;
    if (normalizedArcStart <= normalizedArcEnd) { if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) return distSq <= outerRadiusSq && distSq >= innerRadiusSq; }
    else { if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) return distSq <= outerRadiusSq && distSq >= innerRadiusSq; }
    return false;
}
bool Game::CheckCollisionWithChitbox(const SpaceShark& ss) {
    SDL_Rect sharkRect = { (int)ss.x - SHARK_CENTER.x, (int)ss.y - SHARK_CENTER.y, SHARK_WIDTH, SHARK_HEIGHT };
    return SDL_HasIntersection(&sharkRect, &chitbox);
}
bool Game::CheckCollisionWithArc(const SharkBullet& sb) {
    float targetCenterX = sb.x; float targetCenterY = sb.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float collisionRadius = sqrt(SHARK_BULLET_COLLISION_RADIUS_SQ);
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius); if (innerRadiusSq < 0) innerRadiusSq = 0;
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;
    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;
    if (normalizedArcStart <= normalizedArcEnd) { if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) return distSq <= outerRadiusSq && distSq >= innerRadiusSq; }
    else { if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) return distSq <= outerRadiusSq && distSq >= innerRadiusSq; }
    return false;
}
bool Game::CheckCollisionWithChitbox(const SharkBullet& sb) {
    SDL_Rect bulletRect = { (int)sb.x - SHARK_BULLET_CENTER.x, (int)sb.y - SHARK_BULLET_CENTER.y, SHARK_BULLET_WIDTH, SHARK_BULLET_HEIGHT };
    return SDL_HasIntersection(&bulletRect, &chitbox);
}

// --- Hàm tiện ích ---
void Game::DrawCircle(SDL_Renderer* renderer, const Circle& c) {
    SDL_Point points[CIRCLE_SEGMENTS + 1];
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float rad = (2.0f * PI * i) / CIRCLE_SEGMENTS;
        points[i].x = c.x + static_cast<int>(c.r * cos(rad));
        points[i].y = c.y + static_cast<int>(c.r * sin(rad));
    }
    SDL_RenderDrawLines(renderer, points, CIRCLE_SEGMENTS + 1);
}
void Game::DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle) {
    SDL_SetRenderDrawColor(renderer, SHIELD_ARC_COLOR.r, SHIELD_ARC_COLOR.g, SHIELD_ARC_COLOR.b, SHIELD_ARC_COLOR.a);
    SDL_Point points[ARC_SEGMENTS + 1];
    for (int i = 0; i <= ARC_SEGMENTS; ++i) {
        double angle = startAngle + (arcAngle * i / ARC_SEGMENTS);
        points[i].x = c.x + static_cast<int>(c.r * cos(angle));
        points[i].y = c.y + static_cast<int>(c.r * sin(angle));
    }
    SDL_RenderDrawLines(renderer, points, ARC_SEGMENTS + 1);
}

// Xử lý khi người chơi bị bắn trúng
void Game::HandleHit() {
    if (gameOver) return;
    if (sfxPlayerHit) Mix_PlayChannel(CHANNEL_SFX, sfxPlayerHit, 0);
    for (auto& life : lives) {
        if (!life.isRed) {
            life.isRed = true;
            goto checkGameOver;
        }
    }
checkGameOver:
    bool allRed = true;
    for (const auto& life : lives) {
        if (!life.isRed) {
            allRed = false;
            break;
        }
    }
    if (allRed) {
        triggerGameOver();
        menu->gameState = MainMenu::GAME_OVER;
    }
}

// Setter cho volume
void Game::setVolume(int vol) {
    if (vol >= 0 && vol <= 100) {
        volume = vol;
        int knobRange = volumeSlider.w - volumeKnob.w;
        volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRange));
        Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100);
        Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100);
    }
 }

// Hàm cho MainMenu quản lý trạng thái
void Game::setGameStatePlaying() {
     if (paused) {
        if (pauseStartTime != 0) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
        }
        paused = false;
        pauseStartTime = 0;
        Mix_ResumeMusic();
        if (showWarning) {
             if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1);
        }
     }
}
void Game::setGameStatePaused() {
    if (!paused && !gameOver) {
        pauseStartTime = SDL_GetTicks();
        paused = true;
         Mix_PauseMusic();
         Mix_HaltChannel(CHANNEL_WARNING);
    }
}
void Game::triggerGameOver() {
    if (!gameOver) {
         gameOver = true;
         Mix_HaltMusic();
         Mix_HaltChannel(CHANNEL_WARNING);
         if (sfxGameOver) Mix_PlayChannel(CHANNEL_SFX, sfxGameOver, 0);
         menu->saveHighscores(score);
         updateScoreTexture();
         updateHighscoreTexture();
         if(paused) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            pauseStartTime = 0;
            paused = false;
         }
    }
}

// Setter cho sensitivity
void Game::setSensitivity(int sens) {
    if (sens >= 0 && sens <= 100) {
        sensitivity = sens;
    }
}

// --- XÓA ĐỊNH NGHĨA loadTexture Ở ĐÂY nếu bạn đã copy nó vào đây ---
/*
SDL_Texture* Game::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    // ... nội dung hàm ...
}
*/
