#include "game.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <SDL2/SDL_image.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>

Game::Game(SDL_Renderer* r, SDL_Texture* mt) 
    : renderer(r), missileTexture(mt), mspaceshipTexture(nullptr), gameOverTexture(nullptr), 
      pauseTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr), 
      highscoreTexture(nullptr), font(nullptr), gameOver(false), paused(false), score(0), highscore(0),
      defaultMissileSpeed(150.0f), maxMissileSpeed(150.0f), laserCount(0) {
    startTime = SDL_GetTicks();  // Lấy thời gian bắt đầu game

    // Tải font
    font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24);
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << std::endl;
        exit(1);
    }

    // Tải texture cho tàu vũ trụ
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

    // Tải texture cho pause
    SDL_Surface* pauseSurface = IMG_Load("images/pause.png");
    if (!pauseSurface) {
        std::cerr << "IMG_Load failed for pause.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    pauseTexture = SDL_CreateTextureFromSurface(renderer, pauseSurface);
    SDL_FreeSurface(pauseSurface);
    if (!pauseTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for pause.png: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Tải texture cho nút pause
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

    // Đọc highscore từ file
    loadHighscore();

    // Khởi tạo texture cho điểm số và điểm cao nhất
    updateScoreTexture();
    updateHighscoreTexture();
}

Game::~Game() {
    // Lưu highscore trước khi thoát game
    saveHighscore();

    // Giải phóng tài nguyên
    if (font) {
        TTF_CloseFont(font);
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
    }
    if (highscoreTexture) {
        SDL_DestroyTexture(highscoreTexture);
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

void Game::handleInput(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (mouseX >= pauseButton.x && mouseX <= pauseButton.x + pauseButton.w &&
            mouseY >= pauseButton.y && mouseY <= pauseButton.y + pauseButton.h) {
            paused = !paused;
        }

        if (gameOver) {
            saveHighscore();
            reset();
        }
    }
}

void Game::update(float deltaTime) {
    if (gameOver || paused) {
        return;
    }

    // Điều khiển cung
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= 2 * PI * deltaTime;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += 2 * PI * deltaTime;

    // Sinh tên lửa và laser
    Uint32 currentTime = SDL_GetTicks() - startTime;
    if (currentTime >= nextSpawnTime) {
        // Sinh tên lửa
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
                if (distance < 1e-6) distance = 1.0f; // Tránh chia cho 0
                float missileSpeed = defaultMissileSpeed + 
                    (maxMissileSpeed - defaultMissileSpeed) * (rand() / (float)RAND_MAX);
                t.dx = (distX / distance) * missileSpeed;
                t.dy = (distY / distance) * missileSpeed;
                t.active = true;
                targets.push_back(t);

                spawnedMissilesInWave++;
                lastMissileSpawnTime = currentTime;
            }
        }

        // Sinh laser (từ wave 8)
        if (waveCount >= 8 && spawnedLasersInWave < laserCount) {
            if (currentTime - lastLaserSpawnTime >= 300 || spawnedLasersInWave == 0) {
                Laser l;
                l.spawnTime = currentTime;
                l.active = false;
                int dir = rand() % 4;
                switch (dir) {
                    case 0: l.direction = LEFT; l.angle = PI; break;
                    case 1: l.direction = RIGHT; l.angle = 0; break;
                    case 2: l.direction = UP; l.angle = 3 * PI / 2; break;
                    case 3: l.direction = DOWN; l.angle = PI / 2; break;
                }
                lasers.push_back(l);

                spawnedLasersInWave++;
                lastLaserSpawnTime = currentTime;
            }
        }
    }

    // Chuyển sang wave mới
    if (spawnedMissilesInWave >= missileCount && 
        (waveCount < 8 || spawnedLasersInWave >= laserCount) && 
        currentTime >= nextSpawnTime) {
        waveCount++;

        // Tăng số lượng tên lửa sau mỗi 7-12 wave
        int wavesUntilIncrease = 7 + (rand() % 6); // Ngẫu nhiên từ 7 đến 12
        if (waveCount % wavesUntilIncrease == 0) {
            missileCount++;
        }

        // Tăng tốc độ tên lửa sau mỗi 4 wave
        if (waveCount > 0 && waveCount % 4 == 0) {
            maxMissileSpeed *= 1.15f;
            std::cout << "Max missile speed increased to: " << maxMissileSpeed << std::endl;
        }

        // Tăng số lượng laser sau mỗi 5 wave (từ wave 8)
        if (waveCount >= 8 && (waveCount - 8) % 5 == 0) {
            laserCount++;
            std::cout << "Laser count increased to: " << laserCount << std::endl;
        }

        nextSpawnTime += 3000 + (rand() % 2001);
        spawnedMissilesInWave = 0;
        spawnedLasersInWave = 0;
    }

    // Cập nhật tên lửa
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
                    if (!life.isRed) { life.isRed = true; break; }
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

    // Cập nhật laser
    for (auto& l : lasers) {
        if (!l.active && currentTime - l.spawnTime >= 2500) { // 2.5 giây cảnh báo
            // Kiểm tra xem cung có chặn laser hay không trước khi kích hoạt
            if (!CheckCollisionArcWithLaser(l)) {
                l.active = true; // Chỉ kích hoạt nếu cung không chặn
            } else {
                l.active = false; // Cung chặn laser, giữ trạng thái không active
            }
        }
    }

    // Kiểm tra va chạm với laser
    for (auto& l : lasers) {
        if (l.active) {
            if (CheckCollisionWithLaser(l)) {
                l.active = false; // Vô hiệu hóa ngay sau khi gây sát thương
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
            } else if (CheckCollisionArcWithLaser(l)) {
                l.active = false; // Vô hiệu hóa nếu cung chặn laser khi đã active
                score += 2;
                updateScoreTexture();
                if (score > highscore) {
                    highscore = score;
                    saveHighscore();
                    updateHighscoreTexture();
                }
            }
        }
    }

    // Xóa tên lửa không active
    targets.erase(
        std::remove_if(targets.begin(), targets.end(), [](Target& t) { return !t.active; }),
        targets.end()
    );

    // Xóa laser sau 3.5 giây hoặc khi bị chặn trước khi active
    lasers.erase(
        std::remove_if(lasers.begin(), lasers.end(), 
            [currentTime](Laser& l) { 
                return (!l.active && currentTime - l.spawnTime >= 2500) || 
                       (currentTime - l.spawnTime >= 3500); 
            }),
        lasers.end()
    );
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!gameOver && !paused) {
        // Vẽ tàu vũ trụ
        if (mspaceshipTexture) {
            SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox);
        }

        // Vẽ vòng tròn quỹ đạo và cung
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        DrawCircle(renderer, trajectory);
        DrawArc(renderer, trajectory, arcStartAngle, 2 * PI / 3);

        // Vẽ tên lửa
        for (auto& t : targets) {
            if (t.active && missileTexture) {
                double angle = atan2(t.dy, t.dx) * 180.0 / PI;
                SDL_Rect missileRect = {(int)t.x - 10, (int)t.y - 15, 20, 30};
                SDL_Point center = {10, 15};
                SDL_RenderCopyEx(renderer, missileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE);
            }
        }

        // Vẽ laser
        for (auto& l : lasers) {
            DrawLaser(renderer, l);
        }

        // Vẽ mạng sống
        for (auto& life : lives) {
            Circle lifeCircle = {life.x, life.y, 10};
            SDL_SetRenderDrawColor(renderer, life.isRed ? 255 : 0, 0, life.isRed ? 0 : 255, 255);
            DrawCircle(renderer, lifeCircle);
        }
    }

    // Vẽ nút pause
    if (pauseButtonTexture) {
        SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton);
    }

    // Vẽ điểm số và điểm cao nhất
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

    // Vẽ "GAME OVER"
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

    // Vẽ "PAUSE"
    if (paused) {
        SDL_Rect pauseRect = {800 / 2 - 150, 600 / 2 - 150, 300, 300};
        SDL_RenderCopy(renderer, pauseTexture, NULL, &pauseRect);
    }

    SDL_RenderPresent(renderer);
}

void Game::reset() {
    gameOver = false;
    paused = false;
    targets.clear();
    lasers.clear();
    for (auto& life : lives) {
        life.isRed = false;
    }
    missileCount = 1;
    laserCount = 0;
    waveCount = 0;
    maxMissileSpeed = defaultMissileSpeed;
    score = 0;
    updateScoreTexture();
    nextSpawnTime = 2000;
    spawnedMissilesInWave = 0;
    spawnedLasersInWave = 0;
    lastMissileSpawnTime = 0;
    lastLaserSpawnTime = 0;
    startTime = SDL_GetTicks();
    arcStartAngle = -PI / 10.3;
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

bool Game::CheckCollisionWithLaser(Laser& l) {
    float laserX1 = 400 + 1000 * cos(l.angle);
    float laserY1 = 300 + 1000 * sin(l.angle);
    float laserX2 = 400 - 1000 * cos(l.angle);
    float laserY2 = 300 - 1000 * sin(l.angle);

    for (int x = chitbox.x; x <= chitbox.x + chitbox.w; x++) {
        for (int y = chitbox.y; y <= chitbox.y + chitbox.h; y++) {
            float num = abs((laserY2 - laserY1) * x - (laserX2 - laserX1) * y + laserX2 * laserY1 - laserY2 * laserX1);
            float den = sqrt((laserY2 - laserY1) * (laserY2 - laserY1) + (laserX2 - laserX1) * (laserX2 - laserX1));
            float distance = num / den;
            if (distance < 3.0f) { // Tăng ngưỡng để dễ phát hiện va chạm
                return true;
            }
        }
    }
    return false;
}

bool Game::CheckCollisionArcWithLaser(Laser& l) {
    float laserX1 = 400 + 1000 * cos(l.angle);
    float laserY1 = 300 + 1000 * sin(l.angle);
    float laserX2 = 400 - 1000 * cos(l.angle);
    float laserY2 = 300 - 1000 * sin(l.angle);

    for (double angle = arcStartAngle; angle <= arcStartAngle + 2 * PI / 3; angle += 0.05) {
        int arcX = trajectory.x + trajectory.r * cos(angle);
        int arcY = trajectory.y + trajectory.r * sin(angle);
        float num = abs((laserY2 - laserY1) * arcX - (laserX2 - laserX1) * arcY + laserX2 * laserY1 - laserY2 * laserX1);
        float den = sqrt((laserY2 - laserY1) * (laserY2 - laserY1) + (laserX2 - laserX1) * (laserX2 - laserX1));
        float distance = num / den;
        if (distance < 3.0f) {
            return true;
        }
    }
    return false;
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

void Game::DrawLaser(SDL_Renderer* renderer, Laser& l) {
    float x1 = 400 + 1000 * cos(l.angle);
    float y1 = 300 + 1000 * sin(l.angle);
    float x2 = 400 - 1000 * cos(l.angle);
    float y2 = 300 - 1000 * sin(l.angle);

    Uint32 currentTime = SDL_GetTicks() - startTime;
    if (!l.active && currentTime - l.spawnTime < 2500) {
        // Vẽ nét đứt màu vàng trong 2.5 giây cảnh báo
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        float length = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        float dx = (x2 - x1) / length * 10;
        float dy = (y2 - y1) / length * 10;
        for (float t = 0; t < length; t += 20) {
            float startX = x1 + (t / length) * (x2 - x1);
            float startY = y1 + (t / length) * (y2 - y1);
            float endX = startX + dx;
            float endY = startY + dy;
            if (endX > x1 && endX > x2 || endX < x1 && endX < x2) break;
            SDL_RenderDrawLine(renderer, (int)startX, (int)startY, (int)endX, (int)endY);
        }

        // Vẽ mũi tên
        float arrowBaseX, arrowBaseY;
        switch (l.direction) {
            case LEFT: 
                arrowBaseX = 400 + 50; // Bên phải tâm
                arrowBaseY = 300;
                break;
            case RIGHT: 
                arrowBaseX = 400 - 50; // Bên trái tâm
                arrowBaseY = 300;
                break;
            case UP: 
                arrowBaseX = 400;
                arrowBaseY = 300 + 50; // Phía trên tâm
                break;
            case DOWN: 
                arrowBaseX = 400;
                arrowBaseY = 300 - 50; // Phía dưới tâm
                break;
        }
        float arrowSize = 10;
        float arrowAngle1 = l.angle + PI / 6;
        float arrowAngle2 = l.angle - PI / 6;
        float arrowTipX1 = arrowBaseX + arrowSize * cos(arrowAngle1);
        float arrowTipY1 = arrowBaseY + arrowSize * sin(arrowAngle1);
        float arrowTipX2 = arrowBaseX + arrowSize * cos(arrowAngle2);
        float arrowTipY2 = arrowBaseY + arrowSize * sin(arrowAngle2);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderDrawLine(renderer, (int)arrowBaseX, (int)arrowBaseY, (int)arrowTipX1, (int)arrowTipY1);
        SDL_RenderDrawLine(renderer, (int)arrowBaseX, (int)arrowBaseY, (int)arrowTipX2, (int)arrowTipY2);
    } else if (l.active && currentTime - l.spawnTime < 3000) { // Hoạt hình tia đỏ trong 0.5s
        // Vẽ tia đỏ trong 500ms sau khi kích hoạt (2500ms đến 3000ms)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        
        // Kiểm tra xem cung có chặn laser hay không
        bool arcBlocksLaser = false;
        float arcHitX = x2, arcHitY = y2; // Điểm cuối mặc định của tia
        for (double angle = arcStartAngle; angle <= arcStartAngle + 2 * PI / 3; angle += 0.05) {
            int arcX = trajectory.x + trajectory.r * cos(angle);
            int arcY = trajectory.y + trajectory.r * sin(angle);
            float num = abs((y2 - y1) * arcX - (x2 - x1) * arcY + x2 * y1 - y2 * x1);
            float den = sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
            float distance = num / den;
            if (distance < 3.0f) {
                arcBlocksLaser = true;
                // Tính giao điểm gần nhất giữa laser và cung
                float t = ((arcX - x1) * (x2 - x1) + (arcY - y1) * (y2 - y1)) / 
                          ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
                if (t >= 0 && t <= 1) {
                    arcHitX = x1 + t * (x2 - x1);
                    arcHitY = y1 + t * (y2 - y1);
                }
                break;
            }
        }

        // Vẽ tia đỏ
        if (arcBlocksLaser) {
            // Tia dừng tại cung
            SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)arcHitX, (int)arcHitY);
        } else {
            // Tia xuyên qua màn hình
            SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
        }
    } else if (currentTime - l.spawnTime < 3500) {
        // Vẽ màu xám nếu laser bị vô hiệu hóa sau 0.5s
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
    }
}
