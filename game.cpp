#include "game.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

Game::Game(SDL_Renderer* r, SDL_Texture* mt) 
    : renderer(r), missileTexture(mt), fastMissileTexture(nullptr), warningTexture(nullptr), 
      mspaceshipTexture(nullptr), gameOverTexture(nullptr), pauseTexture(nullptr), 
      pauseButtonTexture(nullptr), scoreTexture(nullptr), highscoreTexture(nullptr), 
      pausedTexture(nullptr), volumeTexture(nullptr), font(nullptr), gameOver(false), 
      paused(false), isDraggingKnob(false), showWarning(false), warningStartTime(0), 
      warningX(0), warningY(0), score(0), highscore(0), volume(64), defaultMissileSpeed(150.0f), 
      maxMissileSpeed(150.0f), pauseStartTime(0), totalPausedTime(0) {
    startTime = SDL_GetTicks();
    wavesUntilIncrease = 7 + (rand() % 6);

    font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 36);
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* spaceshipSurface = IMG_Load("images/mspaceship.png");
    if (!spaceshipSurface) {
        std::cerr << "IMG_Load failed for mspaceship.png: " << IMG_GetError() << std::endl;
    } else {
        mspaceshipTexture = SDL_CreateTextureFromSurface(renderer, spaceshipSurface);
        SDL_FreeSurface(spaceshipSurface);
        if (!mspaceshipTexture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for mspaceship.png: " << SDL_GetError() << std::endl;
        }
    }

    SDL_Surface* pauseButtonSurface = IMG_Load("images/pausebutton.png");
    if (!pauseButtonSurface) {
        std::cerr << "IMG_Load failed for pausebutton.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    pauseButtonTexture = SDL_CreateTextureFromSurface(renderer, pauseButtonSurface);
    SDL_FreeSurface(pauseButtonSurface);
    if (!pauseButtonTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for pausebutton.png: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* fastMissileSurface = IMG_Load("images/fmissile.png");
    if (!fastMissileSurface) {
        std::cerr << "IMG_Load failed for fmissile.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    fastMissileTexture = SDL_CreateTextureFromSurface(renderer, fastMissileSurface);
    SDL_FreeSurface(fastMissileSurface);
    if (!fastMissileTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for fmissile.png: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_Surface* warningSurface = IMG_Load("images/fwarning.png");
    if (!warningSurface) {
        std::cerr << "IMG_Load failed for fwarning.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    warningTexture = SDL_CreateTextureFromSurface(renderer, warningSurface);
    SDL_FreeSurface(warningSurface);
    if (!warningTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for fwarning.png: " << SDL_GetError() << std::endl;
        exit(1);
    }

    Mix_Volume(-1, volume);

    loadHighscore();
    updateScoreTexture();
    updateHighscoreTexture();
    updatePausedTexture();
    updateVolumeTexture();
}

Game::~Game() {
    saveHighscore();

    if (font) {
        TTF_CloseFont(font);
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
    }
    if (highscoreTexture) {
        SDL_DestroyTexture(highscoreTexture);
    }
    if (pausedTexture) {
        SDL_DestroyTexture(pausedTexture);
    }
    if (volumeTexture) {
        SDL_DestroyTexture(volumeTexture);
    }
    if (mspaceshipTexture) {
        SDL_DestroyTexture(mspaceshipTexture);
    }
    if (gameOverTexture) {
        SDL_DestroyTexture(gameOverTexture);
    }
    if (pauseTexture) {
        SDL_DestroyTexture(pauseTexture);
    }
    if (pauseButtonTexture) {
        SDL_DestroyTexture(pauseButtonTexture);
    }
    if (fastMissileTexture) {
        SDL_DestroyTexture(fastMissileTexture);
    }
    if (warningTexture) {
        SDL_DestroyTexture(warningTexture);
    }
}

void Game::loadHighscore() {
    std::ifstream file(playerDataFile);
    if (file.is_open()) {
        file >> highscore;
        file.close();
        std::cout << "Loaded highscore: " << highscore << std::endl;
    } else {
        std::cerr << "Could not open file " << playerDataFile << " for reading. Starting with highscore = 0." << std::endl;
        highscore = 0;
        saveHighscore();
    }
}

void Game::saveHighscore() {
    std::filesystem::create_directories("playerdata");

    std::ofstream file(playerDataFile);
    if (file.is_open()) {
        file << highscore;
        file.close();
        std::cout << "Saved highscore: " << highscore << std::endl;
    } else {
        std::cerr << "Could not open file " << playerDataFile << " for writing." << std::endl;
    }
}

void Game::updateScoreTexture() {
    std::stringstream ss;
    ss << "Score: " << score;
    std::string scoreStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
    }
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!scoreTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for score: " << SDL_GetError() << std::endl;
    }
}

void Game::updateHighscoreTexture() {
    std::stringstream ss;
    ss << "Highscore: " << highscore;
    std::string highscoreStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, highscoreStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (highscoreTexture) {
        SDL_DestroyTexture(highscoreTexture);
    }
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!highscoreTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for highscore: " << SDL_GetError() << std::endl;
    }
}

void Game::updatePausedTexture() {
    std::string pausedStr = "Paused";
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, pausedStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (pausedTexture) {
        SDL_DestroyTexture(pausedTexture);
    }
    pausedTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!pausedTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for paused: " << TTF_GetError() << std::endl;
    }
}

void Game::updateVolumeTexture() {
    std::stringstream ss;
    ss << "Volume: " << volume;
    std::string volumeStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, volumeStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (volumeTexture) {
        SDL_DestroyTexture(volumeTexture);
    }
    volumeTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!volumeTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for volume: " << SDL_GetError() << std::endl;
    }
}

void Game::handleInput(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (mouseX >= pauseButton.x && mouseX <= pauseButton.x + pauseButton.w &&
            mouseY >= pauseButton.y && mouseY <= pauseButton.y + pauseButton.h) {
            if (!paused) {
                pauseStartTime = SDL_GetTicks();
                paused = true;
            } else {
                totalPausedTime += SDL_GetTicks() - pauseStartTime;
                paused = false;
                pauseStartTime = 0;
            }
            isDraggingKnob = false;
        }

        if (paused) {
            if (mouseX >= volumeKnob.x && mouseX <= volumeKnob.x + volumeKnob.w &&
                mouseY >= volumeKnob.y && mouseY <= volumeKnob.y + volumeKnob.h) {
                isDraggingKnob = true;
            }
        }

        if (gameOver) {
            saveHighscore();
            reset();
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        isDraggingKnob = false;
    }
    else if (event.type == SDL_MOUSEMOTION && isDraggingKnob) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        int newX = mouseX - volumeKnob.w / 2;
        if (newX < volumeSlider.x) newX = volumeSlider.x;
        if (newX > volumeSlider.x + volumeSlider.w - volumeKnob.w) {
            newX = volumeSlider.x + volumeSlider.w - volumeKnob.w;
        }

        volumeKnob.x = newX;

        float position = (float)(volumeKnob.x - volumeSlider.x) / (volumeSlider.w - volumeKnob.w);
        volume = (int)(position * 128);
        Mix_Volume(-1, volume);
        updateVolumeTexture();
    }
}

void Game::update(float deltaTime) {
    if (gameOver) {
        return;
    }

    // Tính thời gian hiệu quả, trừ đi thời gian đã pause
    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    // Logic chỉ chạy khi không pause
    if (!paused) {
        // Input điều khiển vòng cung
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_A]) arcStartAngle -= 2 * PI * deltaTime;
        if (keys[SDL_SCANCODE_D]) arcStartAngle += 2 * PI * deltaTime;

        // Kiểm tra và hiển thị cảnh báo cho Fast Missile
        if (waveCount >= 9 && (waveCount - 9) % 3 == 0 && fastMissiles.empty() && !showWarning) {
            // Kích hoạt cảnh báo nếu chưa có Fast Missile và chưa hiển thị cảnh báo
            showWarning = true;
            warningStartTime = currentTime;
            // Chọn vị trí cho Fast Missile và lưu vị trí cảnh báo, dịch vào trong 20 pixel
            int side = rand() % 4;
            switch (side) {
                case 0: warningX = 20; warningY = rand() % 560 + 20; break; // Trái (vào 20 pixel, y từ 20 đến 580)
                case 1: warningX = 780; warningY = rand() % 560 + 20; break; // Phải (vào 20 pixel, y từ 20 đến 580)
                case 2: warningX = rand() % 760 + 20; warningY = 20; break; // Trên (vào 20 pixel, x từ 20 đến 780)
                case 3: warningX = rand() % 760 + 20; warningY = 580; break; // Dưới (vào 20 pixel, x từ 20 đến 780)
            }
        }

        // Sau 2 giây hiển thị cảnh báo, sinh Fast Missile tại vị trí đã lưu
        if (showWarning && (currentTime - warningStartTime >= 2000)) {
            showWarning = false; // Tắt cảnh báo
            Target fm;
            fm.x = warningX;
            fm.y = warningY;
            float distX = 400 - fm.x, distY = 300 - fm.y;
            float distance = sqrt(distX * distX + distY * distY);
            if (distance < 1e-6) distance = 1.0f;
            float missileSpeed = defaultMissileSpeed * 2.5f; // Tốc độ Fast Missile gấp 2.5 lần
            fm.dx = (distX / distance) * missileSpeed;
            fm.dy = (distY / distance) * missileSpeed;
            fm.active = true;
            fastMissiles.push_back(fm);
        }

        // Sinh tên lửa thường
        if (currentTime >= nextSpawnTime) {
            if (spawnedMissilesInWave < missileCount) {
                if (currentTime - lastMissileSpawnTime >= 300 || spawnedMissilesInWave == 0) {
                    Target t;
                    int side = rand() % 4;
                    switch (side) {
                        case 0: t.x = 0; t.y = rand() % 600; break;
                        case 1: t.x = 800; t.y = rand() % 600; break;
                        case 2: t.x = rand() % 800; t.y = 0; break;
                        case 3: t.x = rand() % 800; t.y = 600; break;
                    }
                    float distX = 400 - t.x, distY = 300 - t.y;
                    float distance = sqrt(distX * distX + distY * distY);
                    if (distance < 1e-6) distance = 1.0f;
                    float missileSpeed = defaultMissileSpeed;
                    t.dx = (distX / distance) * missileSpeed;
                    t.dy = (distY / distance) * missileSpeed;
                    t.active = true;
                    targets.push_back(t);

                    spawnedMissilesInWave++;
                    lastMissileSpawnTime = currentTime;
                }
            }
        }

        // Cập nhật wave
        if (spawnedMissilesInWave >= missileCount && currentTime >= nextSpawnTime) {
            waveCount++;
            if (waveCount % wavesUntilIncrease == 0) {
                missileCount++;
                if (missileCount > 5) missileCount = 5;
                std::cout << "Missile count increased to: " << missileCount << std::endl;
            }
            nextSpawnTime += 3000 + (rand() % 2001);
            spawnedMissilesInWave = 0;
        }

        // Di chuyển và kiểm tra va chạm cho tên lửa thường
        for (auto& t : targets) {
            if (t.active) {
                t.x += t.dx * deltaTime;
                t.y += t.dy * deltaTime;
                if (CheckCollisionWithArc(t)) {
                    t.active = false;
                    score++;
                    updateScoreTexture();
                    if (score > highscore) {
                        highscore = score;
                        saveHighscore();
                        updateHighscoreTexture();
                    }
                }
                else if (CheckCollisionWithChitbox(t)) {
                    t.active = false;
                    for (auto& life : lives) {
                        if (!life.isRed) {
                            life.isRed = true;
                            break;
                        }
                    }
                    bool allRed = true;
                    for (auto& life : lives) {
                        if (!life.isRed) { allRed = false; break; }
                    }
                    if (allRed) {
                        gameOver = true;
                        saveHighscore();
                    }
                }
            }
        }

        // Di chuyển và kiểm tra va chạm cho Fast Missile
        for (auto& fm : fastMissiles) {
            if (fm.active) {
                fm.x += fm.dx * deltaTime;
                fm.y += fm.dy * deltaTime;
                if (CheckCollisionWithArc(fm)) {
                    fm.active = false;
                    score += 2;
                    updateScoreTexture();
                    if (score > highscore) {
                        highscore = score;
                        saveHighscore();
                        updateHighscoreTexture();
                    }
                }
                else if (CheckCollisionWithChitbox(fm)) {
                    fm.active = false;
                    for (auto& life : lives) {
                        if (!life.isRed) {
                            life.isRed = true;
                            break;
                        }
                    }
                    bool allRed = true;
                    for (auto& life : lives) {
                        if (!life.isRed) { allRed = false; break; }
                    }
                    if (allRed) {
                        gameOver = true;
                        saveHighscore();
                    }
                }
            }
        }

        // Xóa tên lửa không hoạt động
        targets.erase(
            std::remove_if(targets.begin(), targets.end(), [](Target& t) { return !t.active; }),
            targets.end()
        );
        fastMissiles.erase(
            std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](Target& fm) { return !fm.active; }),
            fastMissiles.end()
        );
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!gameOver && !paused) {
        if (mspaceshipTexture) {
            SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        DrawCircle(renderer, trajectory);
        DrawArc(renderer, trajectory, arcStartAngle, 2 * PI / 3);

        // Vẽ tên lửa thường
        for (auto& t : targets) {
            if (t.active && missileTexture) {
                double angle = atan2(t.dy, t.dx) * 180.0 / PI;
                SDL_Rect missileRect = {(int)t.x - 10, (int)t.y - 15, 30, 30};
                SDL_Point center = {10, 15};
                SDL_RenderCopyEx(renderer, missileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE);
            }
        }

        // Vẽ Fast Missile
        for (auto& fm : fastMissiles) {
            if (fm.active && fastMissileTexture) {
                double angle = atan2(fm.dy, fm.dx) * 180.0 / PI;
                SDL_Rect missileRect = {(int)fm.x - 10, (int)fm.y - 15, 20, 30};
                SDL_Point center = {10, 15};
                SDL_RenderCopyEx(renderer, fastMissileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE);
            }
        }

        for (auto& life : lives) {
            Circle lifeCircle = {life.x, life.y, 10};
            SDL_SetRenderDrawColor(renderer, life.isRed ? 255 : 0, 0, life.isRed ? 0 : 255, 255);
            DrawCircle(renderer, lifeCircle);
        }

        // Hiển thị cảnh báo Fast Missile với hiệu ứng nhấp nháy
        if (showWarning && warningTexture) {
            // Tính alpha cho hiệu ứng nhấp nháy (dao động từ 50 đến 255)
            Uint32 elapsedTime = SDL_GetTicks() - startTime - totalPausedTime - warningStartTime;
            float alpha = 152.5f + 102.5f * sin(2 * PI * 2 * elapsedTime / 1000.0f); // 2 lần nhấp nháy mỗi giây
            SDL_SetTextureAlphaMod(warningTexture, static_cast<Uint8>(alpha));

            SDL_Rect warningRect = {(int)warningX - 15, (int)warningY - 22, 30, 45}; // Kích thước 30x45
            SDL_RenderCopy(renderer, warningTexture, NULL, &warningRect);

            // Đặt lại alpha về 255 để không ảnh hưởng đến các texture khác
            SDL_SetTextureAlphaMod(warningTexture, 255);
        }
    }

    if (pauseButtonTexture) {
        SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton);
    }

    if (scoreTexture) {
        int w, h;
        SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
        SDL_Rect scoreRect = {800 - w - 10, 10, w, h};
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
    }
    if (highscoreTexture) {
        int w, h;
        SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
        SDL_Rect highscoreRect = {800 - w - 10, 40, w, h};
        SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
    }

    if (gameOver && !gameOverTexture) {
        SDL_Surface* textSurface = IMG_Load("images/gameover.png");
        if (!textSurface) {
            std::cerr << "Không thể tải hình ảnh gameover.png: " << IMG_GetError() << std::endl;
            return;
        }
        gameOverTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
    }
    if (gameOver) {
        SDL_Rect textRect = {800 / 2 - 150, 600 / 2 - 150, 300, 300};
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &textRect);
    }

    if (paused) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_Rect overlay = {0, 0, 800, 600};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (pausedTexture) {
            int w, h;
            SDL_QueryTexture(pausedTexture, NULL, NULL, &w, &h);
            SDL_Rect pausedRect = {800 / 2 - w / 2, 300 - h / 2, w, h};
            SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
        }

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &volumeSlider);

        SDL_SetRenderDrawColor(renderer, isDraggingKnob ? 255 : 200, isDraggingKnob ? 255 : 200, isDraggingKnob ? 0 : 255, 255);
        SDL_RenderFillRect(renderer, &volumeKnob);

        if (volumeTexture) {
            int w, h;
            SDL_QueryTexture(volumeTexture, NULL, NULL, &w, &h);
            SDL_Rect volumeRect = {800 / 2 - w / 2, volumeSlider.y - h - 20, w, h};
            SDL_RenderCopy(renderer, volumeTexture, NULL, &volumeRect);
        }
    }

    SDL_RenderPresent(renderer);
}

void Game::reset() {
    gameOver = false;
    paused = false;
    isDraggingKnob = false;
    showWarning = false;
    warningStartTime = 0;
    warningX = 0;
    warningY = 0;
    targets.clear();
    fastMissiles.clear();
    for (auto& life : lives) {
        life.isRed = false;
    }
    missileCount = 1;
    waveCount = 0;
    maxMissileSpeed = defaultMissileSpeed;
    score = 0;
    updateScoreTexture();
    nextSpawnTime = 2000;
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    startTime = SDL_GetTicks();
    arcStartAngle = -PI / 10.3;
    wavesUntilIncrease = 7 + (rand() % 6);
    volume = 64;
    volumeKnob.x = 364;
    updateVolumeTexture();
    pauseStartTime = 0;
    totalPausedTime = 0;
    if (gameOverTexture) {
        SDL_DestroyTexture(gameOverTexture);
        gameOverTexture = nullptr;
    }
}

bool Game::CheckCollisionWithArc(Target& t) {
    for (double angle = arcStartAngle; angle <= arcStartAngle + 2 * PI / 3; angle += 0.01) {
        int arcX = trajectory.x + trajectory.r * cos(angle);
        int arcY = trajectory.y + trajectory.r * sin(angle);
        int dist = (t.x - arcX) * (t.x - arcX) + (t.y - arcY) * (t.y - arcY);
        if (dist < 25) return true;
    }
    return false;
}

bool Game::CheckCollisionWithChitbox(Target& t) {
    return (t.x >= chitbox.x && t.x <= chitbox.x + chitbox.w &&
            t.y >= chitbox.y && t.y <= chitbox.y + chitbox.h);
}

void Game::DrawCircle(SDL_Renderer* renderer, Circle& c) {
    const int segments = 36;
    SDL_Point points[segments + 1];
    for (int i = 0; i <= segments; i++) {
        float rad = (2 * PI * i) / segments;
        points[i].x = c.x + c.r * cos(rad);
        points[i].y = c.y + c.r * sin(rad);
    }
    SDL_RenderDrawLines(renderer, points, segments + 1);
}

void Game::DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle) {
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    const int segments = 24;
    SDL_Point points[segments + 1];
    for (int i = 0; i <= segments; i++) {
        double angle = startAngle + (arcAngle * i / segments);
        points[i].x = c.x + c.r * cos(angle);
        points[i].y = c.y + c.r * sin(angle);
    }
    SDL_RenderDrawLines(renderer, points, segments + 1);
}
