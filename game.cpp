#include "game.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <SDL2/SDL_image.h>
#include <sstream>
#include <fstream>
#include <filesystem> // Thêm để tạo thư mục nếu cần

Game::Game(SDL_Renderer* r, SDL_Texture* mt) 
    : renderer(r), missileTexture(mt), mspaceshipTexture(nullptr), gameOverTexture(nullptr), 
      pauseTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr), 
      highscoreTexture(nullptr), font(nullptr), gameOver(false), paused(false), score(0), highscore(0) {
    startTime = SDL_GetTicks();  // Lấy thời gian bắt đầu game

    // Tải font
    font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24); // Đường dẫn đến file font, kích thước 24
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << std::endl;
        exit(1); // Thoát nếu không tải được font
    }

    // Tải texture cho tàu vũ trụ
    SDL_Surface* spaceshipSurface = IMG_Load("images/mspaceship.png");
    if (!spaceshipSurface) {
        std::cerr << "IMG_Load failed for mspaceship.png: " << IMG_GetError() << std::endl;
        // Không thoát ngay, để game vẫn chạy được, nhưng không vẽ tàu
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
        file >> highscore; // Đọc highscore từ file
        file.close();
        std::cout << "Loaded highscore: " << highscore << std::endl;
    } else {
        std::cerr << "Could not open file " << playerDataFile << " for reading. Starting with highscore = 0." << std::endl;
        highscore = 0; // Nếu không mở được file, đặt highscore về 0
        // Tạo file mới với giá trị 0
        saveHighscore();
    }
}

void Game::saveHighscore() {
    // Tạo thư mục playerdata nếu chưa tồn tại
    std::filesystem::create_directories("playerdata");

    std::ofstream file(playerDataFile);
    if (file.is_open()) {
        file << highscore; // Ghi highscore vào file
        file.close();
        std::cout << "Saved highscore: " << highscore << std::endl;
    } else {
        std::cerr << "Could not open file " << playerDataFile << " for writing." << std::endl;
    }
}

void Game::updateScoreTexture() {
    // Tạo chuỗi văn bản "Score: <score>"
    std::stringstream ss;
    ss << "Score: " << score;
    std::string scoreStr = ss.str();

    // Tạo surface từ văn bản
    SDL_Color textColor = {255, 255, 255, 255}; // Màu trắng
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    // Tạo texture từ surface
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture); // Xóa texture cũ trước khi tạo mới
    }
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!scoreTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for score: " << SDL_GetError() << std::endl;
    }
}

void Game::updateHighscoreTexture() {
    // Tạo chuỗi văn bản "Highscore: <highscore>"
    std::stringstream ss;
    ss << "Highscore: " << highscore;
    std::string highscoreStr = ss.str();

    // Tạo surface từ văn bản
    SDL_Color textColor = {255, 255, 255, 255}; // Màu trắng
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, highscoreStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    // Tạo texture từ surface
    if (highscoreTexture) {
        SDL_DestroyTexture(highscoreTexture); // Xóa texture cũ trước khi tạo mới
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

        // Kiểm tra nhấp chuột vào nút pause
        if (mouseX >= pauseButton.x && mouseX <= pauseButton.x + pauseButton.w &&
            mouseY >= pauseButton.y && mouseY <= pauseButton.y + pauseButton.h) {
            paused = !paused; // Bật/tắt pause
        }

        // Khởi động lại game khi nhấp chuột trong trạng thái game over
        if (gameOver) {
            saveHighscore(); // Lưu highscore trước khi reset
            reset();
        }
    }
}

void Game::update(float deltaTime) {
    if (gameOver || paused) {
        return; // Không cập nhật logic game khi game over hoặc pause
    }

    // Điều khiển cung (nhân vật)
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= 2 * PI * deltaTime;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += 2 * PI * deltaTime;

    // Sinh đạn địch theo thời gian
    Uint32 currentTime = SDL_GetTicks() - startTime; // Thời gian đã trôi qua
    if (currentTime >= nextSpawnTime && spawnedInWave < missileCount) {
        // Sinh từng tên lửa cách nhau 0.3 giây
        if (currentTime - lastMissileSpawnTime >= 300 || spawnedInWave == 0) { // 300 ms = 0.3 giây
            Target t;
            int side = rand() % 4;
            switch (side) {
                case 0: t.x = 0; t.y = rand() % 600; break;     // Từ trái
                case 1: t.x = 800; t.y = rand() % 600; break;   // Từ phải
                case 2: t.x = rand() % 800; t.y = 0; break;     // Từ trên
                case 3: t.x = rand() % 800; t.y = 600; break;   // Từ dưới
            }
            float distX = 400 - t.x, distY = 300 - t.y;
            float distance = sqrt(distX * distX + distY * distY);
            t.dx = distX / missileSpeed; // Dùng missileSpeed
            t.dy = distY / missileSpeed;
            t.active = true;
            targets.push_back(t);

            spawnedInWave++;           // Tăng số tên lửa đã sinh trong đợt
            lastMissileSpawnTime = currentTime; // Cập nhật thời gian sinh tên lửa cuối
        }
    }

    // Khi sinh đủ tên lửa trong đợt, chuyển sang đợt tiếp theo
    if (spawnedInWave >= missileCount && currentTime >= nextSpawnTime) {
        waveCount++; // Tăng số wave đã hoàn thành

        // Tăng số lượng tên lửa sau mỗi 2-3 wave
        int wavesUntilIncrease = 2 + (rand() % 2); // Ngẫu nhiên 2 hoặc 3
        if (waveCount % wavesUntilIncrease == 0) {
            missileCount++; // Tăng số lượng tên lửa
        }

        // Tăng tốc độ tên lửa 15% sau mỗi 4 wave
        if (waveCount > 0 && waveCount % 4 == 0) {
            missileSpeed *= 1.15f; // Tăng 15%
            std::cout << "Missile speed increased to: " << missileSpeed << std::endl; // In ra để kiểm tra
        }

        nextSpawnTime += 3000 + (rand() % 2001); // Ngẫu nhiên từ 3000 đến 5000 ms (3-5 giây)
        spawnedInWave = 0;      // Reset số tên lửa đã sinh cho đợt mới
    }

    // Cập nhật đạn địch
    for (auto& t : targets) {
        if (t.active) {
            t.x += t.dx * deltaTime * missileSpeed; // Dùng missileSpeed
            t.y += t.dy * deltaTime * missileSpeed;
            if (CheckCollisionWithArc(t)) {
                t.active = false;
                score++; // Tăng điểm số khi tên lửa va chạm với cung (khien)
                updateScoreTexture(); // Cập nhật texture điểm số
                if (score > highscore) {
                    highscore = score;
                    saveHighscore(); // Lưu highscore mới vào file
                    updateHighscoreTexture(); // Cập nhật texture điểm cao nhất
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
                    saveHighscore(); // Lưu highscore khi game over
                }
            }
        }
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!gameOver && !paused) {
        // Vẽ tàu vũ trụ thay vì hitbox màu đỏ
        if (mspaceshipTexture) {
            SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox);
        }

        // Vẽ vòng tròn quỹ đạo và cung
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        DrawCircle(renderer, trajectory);
        DrawArc(renderer, trajectory, arcStartAngle, 2 * PI / 3); // 120 độ

        // Vẽ đạn địch
        for (auto& t : targets) {
            if (t.active && missileTexture) {
                double angle = atan2(t.dy, t.dx) * 180.0 / PI;
                SDL_Rect missileRect = {(int)t.x - 10, (int)t.y - 15, 20, 30}; // Kích thước tên lửa: 20x30
                SDL_Point center = {10, 15}; // Tâm xoay: nửa chiều rộng, nửa chiều cao
                SDL_RenderCopyEx(renderer, missileTexture, NULL, &missileRect, angle, &center, SDL_FLIP_NONE); // Sửa lỗi cú pháp
            }
        }

        // Vẽ mạng sống
        for (auto& life : lives) {
            Circle lifeCircle = {life.x, life.y, 10};
            SDL_SetRenderDrawColor(renderer, life.isRed ? 255 : 0, 0, life.isRed ? 0 : 255, 255);
            DrawCircle(renderer, lifeCircle);
        }
    }

    // Vẽ nút pause (hình ảnh pausebutton.png)
    if (pauseButtonTexture) {
        SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton);
    }

    // Vẽ điểm số và điểm cao nhất bên phải màn hình
    if (scoreTexture) {
        int w, h;
        SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
        SDL_Rect scoreRect = {800 - w - 10, 10, w, h}; // Bên phải, cách lề 10px, phía trên
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
    }
    if (highscoreTexture) {
        int w, h;
        SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
        SDL_Rect highscoreRect = {800 - w - 10, 40, w, h}; // Bên phải, dưới score 30px
        SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
    }

    // Vẽ hình ảnh "GAME OVER"
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
        SDL_Rect textRect = {800 / 2 - 150, 600 / 2 - 150, 300, 300}; // Kích thước của hình ảnh
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &textRect);
    }

    // Vẽ hình ảnh "PAUSE"
    if (paused) {
        SDL_Rect pauseRect = {800 / 2 - 150, 600 / 2 - 150, 300, 300}; // Cùng vị trí với game over
        SDL_RenderCopy(renderer, pauseTexture, NULL, &pauseRect);
    }

    SDL_RenderPresent(renderer);
}

void Game::reset() {
    // Khởi động lại các giá trị về trạng thái ban đầu, nhưng không reset highscore
    gameOver = false;
    paused = false;                    // Đặt lại trạng thái pause
    targets.clear();                   // Xóa tất cả tên lửa
    for (auto& life : lives) {
        life.isRed = false;            // Reset mạng sống
    }
    missileCount = 1;                  // Reset số lượng tên lửa mỗi đợt
    waveCount = 0;                     // Reset số wave
    missileSpeed = 150.0f;             // Reset tốc độ tên lửa
    score = 0;                         // Reset điểm số, nhưng không reset highscore
    updateScoreTexture();              // Cập nhật texture điểm số
    nextSpawnTime = 2000;              // Đặt lại mốc sinh đầu tiên sau 2 giây
    spawnedInWave = 0;                 // Reset số tên lửa đã sinh trong đợt
    lastMissileSpawnTime = 0;          // Reset thời gian sinh tên lửa cuối
    startTime = SDL_GetTicks();        // Reset thời gian bắt đầu game
    arcStartAngle = -PI / 10.3;        // Reset góc cung
    if (gameOverTexture) {
        SDL_DestroyTexture(gameOverTexture); // Xóa texture Game Over cũ
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
