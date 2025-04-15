#include "game.h"
#include "mainmenu.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

Game::Game(SDL_Renderer* r, Enemy* e, MainMenu* m) 
    : renderer(r), enemy(e), menu(m), mspaceshipTexture(nullptr), 
      pauseButtonTexture(nullptr), scoreTexture(nullptr), 
      highscoreTexture(nullptr), pausedTexture(nullptr), 
      backToMenuTexture(nullptr), restartTexture(nullptr), 
      gameOverTextTexture(nullptr), volumeLabelTexture(nullptr),
      giveUpTexture(nullptr),
      gameOver(false), paused(false), showWarning(false), 
      isFirstFastMissile(true), justStarted(false),
      warningStartTime(0), warningX(0), warningY(0), 
      score(0), pauseStartTime(0), totalPausedTime(0), 
      volume(100), sensitivity(50), isDraggingVolume(false),
      missileCount(1), waveCount(0),
      nextSpawnTime(2000), lastMissileSpawnTime(0),
      arcStartAngle(-PI / 10.3),
      defaultMissileSpeed(100.0f), maxMissileSpeed(100.0f),
      chitbox{375, 250, 50, 100},
      pauseButton{750, 10, 40, 40},
      backToMenuButton{300, 400, 200, 50},
      restartButton{300, 340, 200, 50},
      giveUpButton{300, 460, 200, 50},
      trajectory{400, 300, 60} {
    wavesUntilIncrease = 7 + (rand() % 6);

    for (int i = 0; i < 3; i++) {
        Life life = {20 + i * 30, 20, false};
        lives.push_back(life);
    }

    volumeSlider = {300, 420, 200, 10};
    volumeKnob = {300 + (volume * 200 / 100) - 5, 415, 10, 20};
    Mix_VolumeMusic(volume * 128 / 100);
    sensitivity = menu->sensitivity;

    SDL_Surface* spaceshipSurface = IMG_Load("images/mspaceship.png");
    if (!spaceshipSurface) {
        std::cerr << "IMG_Load failed for mspaceship.png: " << IMG_GetError() << std::endl;
        exit(1);
    }
    mspaceshipTexture = SDL_CreateTextureFromSurface(renderer, spaceshipSurface);
    SDL_FreeSurface(spaceshipSurface);
    if (!mspaceshipTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for mspaceship.png: " << SDL_GetError() << std::endl;
        exit(1);
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

    updatePausedTexture();
    updateGameOverTextTexture();
    updateVolumeLabelTexture();
    updateScoreTexture();
    updateHighscoreTexture();
    initTextures();
}

Game::~Game() {
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
}

void Game::initTextures() {
    SDL_Color textColor = {255, 255, 255, 255};
    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 36);
    if (!font) {
        std::cerr << "TTF_OpenFont failed for Back to Menu: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Back to Menu", textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for Back to Menu: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }
    backToMenuTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!backToMenuTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for Back to Menu: " << SDL_GetError() << std::endl;
    }
    SDL_FreeSurface(textSurface);

    textSurface = TTF_RenderText_Solid(font, "Restart", textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for Restart: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }
    restartTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!restartTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for Restart: " << SDL_GetError() << std::endl;
    }
    SDL_FreeSurface(textSurface);

    textSurface = TTF_RenderText_Solid(font, "Give Up", textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for Give Up: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }
    giveUpTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!giveUpTexture) {
        std::cerr << "SDL_CreateTextureFromSurface failed for Give Up: " << SDL_GetError() << std::endl;
    }
    SDL_FreeSurface(textSurface);

    TTF_CloseFont(font);
}

void Game::updateScoreTexture() {
    std::stringstream ss;
    ss << "Score: " << score;
    std::string scoreStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 36);
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
}

void Game::updateHighscoreTexture() {
    std::stringstream ss;
    int highscore = menu->highscores.empty() ? 0 : menu->highscores[0];
    ss << "Highscore: " << highscore;
    std::string highscoreStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 36);
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, highscoreStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
}

void Game::updatePausedTexture() {
    std::string pausedStr = "Paused";
    SDL_Color textColor = {255, 255, 255, 255};
    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 36);
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, pausedStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (pausedTexture) SDL_DestroyTexture(pausedTexture);
    pausedTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
}

void Game::updateGameOverTextTexture() {
    std::string gameOverStr = "Game over";
    SDL_Color textColor = {255, 255, 255, 255};
    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 48);
    if (!font) {
        std::cerr << "TTF_OpenFont failed for Game over: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, gameOverStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for Game over: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (gameOverTextTexture) SDL_DestroyTexture(gameOverTextTexture);
    gameOverTextTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
}

void Game::updateVolumeLabelTexture() {
    std::string volumeLabelStr = "Volume";
    SDL_Color textColor = {255, 255, 255, 255};
    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24);
    if (!font) {
        std::cerr << "TTF_OpenFont failed for Volume: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, volumeLabelStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed for Volume: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    if (volumeLabelTexture) SDL_DestroyTexture(volumeLabelTexture);
    volumeLabelTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
}

void Game::handleInput(SDL_Event& event, MainMenu& menu) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (mouseX >= pauseButton.x && mouseX <= pauseButton.x + pauseButton.w &&
            mouseY >= pauseButton.y && mouseY <= pauseButton.y + pauseButton.h) {
            if (!paused) {
                pauseStartTime = SDL_GetTicks();
                paused = true;
                menu.gameState = MainMenu::PAUSED;
            } else {
                totalPausedTime += SDL_GetTicks() - pauseStartTime;
                paused = false;
                pauseStartTime = 0;
                menu.gameState = MainMenu::PLAYING;
            }
        }

        if (paused && mouseX >= volumeKnob.x && mouseX <= volumeKnob.x + volumeKnob.w &&
            mouseY >= volumeKnob.y && mouseY <= volumeKnob.y + volumeKnob.h) {
            isDraggingVolume = true;
        }

        if (paused && mouseX >= giveUpButton.x && mouseX <= giveUpButton.x + giveUpButton.w &&
            mouseY >= giveUpButton.y && mouseY <= giveUpButton.y + giveUpButton.h) {
            paused = false;
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            pauseStartTime = 0;
            gameOver = true;
            menu.gameState = MainMenu::GAME_OVER;
            menu.saveHighscores(score);
            updateScoreTexture();
            updateHighscoreTexture();
        }

        if (gameOver) {
            if (mouseX >= backToMenuButton.x && mouseX <= backToMenuButton.x + backToMenuButton.w &&
                mouseY >= backToMenuButton.y && mouseY <= backToMenuButton.y + backToMenuButton.h) {
                reset();
                menu.gameState = MainMenu::MENU;
            }
            if (mouseX >= restartButton.x && mouseX <= restartButton.x + restartButton.w &&
                mouseY >= restartButton.y && mouseY <= restartButton.y + restartButton.h) {
                reset();
                menu.gameState = MainMenu::PLAYING;
                startGame();
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP) {
        isDraggingVolume = false;
    }

    if (event.type == SDL_MOUSEMOTION && isDraggingVolume) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        int newX = mouseX - volumeKnob.w / 2;
        if (newX < volumeSlider.x) newX = volumeSlider.x;
        if (newX > volumeSlider.x + volumeSlider.w - volumeKnob.w) newX = volumeSlider.x + volumeSlider.w - volumeKnob.w;
        volumeKnob.x = newX;

        int newVolume = ((newX - volumeSlider.x) * 100) / volumeSlider.w;
        setVolume(newVolume);
        menu.volume = volume;
    }

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (!paused && !gameOver) {
            pauseStartTime = SDL_GetTicks();
            paused = true;
            menu.gameState = MainMenu::PAUSED;
        } else if (paused) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            paused = false;
            pauseStartTime = 0;
            menu.gameState = MainMenu::PLAYING;
        }
    }
}

void Game::update(float deltaTime) {
    if (gameOver) return;

    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    if (!paused) {
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        float sensitivityFactor = 0.75f + (sensitivity / 100.0f) * 0.5f;
        if (keys[SDL_SCANCODE_A]) arcStartAngle -= 2 * PI * deltaTime * sensitivityFactor;
        if (keys[SDL_SCANCODE_D]) arcStartAngle += 2 * PI * deltaTime * sensitivityFactor;

        // Tạo SpaceShark ở wave 15, 30, 45, ...
        if (waveCount >= 15 && waveCount % 15 == 0 && spaceSharks.empty()) {
            SpaceShark ss;
            ss.radius = 300.0f;
            ss.angle = static_cast<float>(rand()) / RAND_MAX * 2 * PI;
            ss.angularSpeed = 2.0f;
            ss.x = 400 + ss.radius * cos(ss.angle);
            ss.y = 300 + ss.radius * sin(ss.angle);
            ss.spawnTime = currentTime;
            ss.lastBulletTime = currentTime;
            ss.active = true;
            spaceSharks.push_back(ss);
        }

        // Tạo tên lửa nhanh
        if (waveCount >= 9 && (waveCount - 9) % 3 == 0 && fastMissiles.empty() && !showWarning) {
            showWarning = true;
            warningStartTime = currentTime;
            int side = rand() % 4;
            switch (side) {
                case 0: warningX = 20; warningY = rand() % 560 + 20; break;
                case 1: warningX = 780; warningY = rand() % 560 + 20; break;
                case 2: warningX = rand() % 760 + 20; warningY = 20; break;
                case 3: warningX = rand() % 760 + 20; warningY = 580; break;
            }
        }

        if (showWarning && (currentTime - warningStartTime >= 2000)) {
            showWarning = false;
            Target fm;
            fm.x = warningX;
            fm.y = warningY;
            float distX = 400 - fm.x, distY = 300 - fm.y;
            float distance = sqrt(distX * distX + distY * distY);
            if (distance < 1e-6) distance = 1.0f;
            float baseSpeed = defaultMissileSpeed + static_cast<float>(rand()) / RAND_MAX * (maxMissileSpeed - defaultMissileSpeed);
            float missileSpeed = baseSpeed * 4.5f;
            fm.dx = (distX / distance) * missileSpeed;
            fm.dy = (distY / distance) * missileSpeed;
            fm.active = true;
            fastMissiles.push_back(fm);
            isFirstFastMissile = false;
        }

        // Tạo tên lửa thường và chuyển wave
        bool waveAdvanced = false;
        if (!justStarted && currentTime >= nextSpawnTime) {
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
                    float missileSpeed = defaultMissileSpeed + static_cast<float>(rand()) / RAND_MAX * (maxMissileSpeed - defaultMissileSpeed);
                    t.dx = (distX / distance) * missileSpeed;
                    t.dy = (distY / distance) * missileSpeed;
                    t.active = true;
                    targets.push_back(t);

                    spawnedMissilesInWave++;
                    lastMissileSpawnTime = currentTime;
                }
            }
        }

        if (!justStarted && spawnedMissilesInWave >= missileCount && currentTime >= nextSpawnTime && !waveAdvanced) {
            waveCount++;
            if (waveCount % wavesUntilIncrease == 0) {
                missileCount++;
                if (missileCount > 5) missileCount = 5;
            }
            nextSpawnTime = currentTime + 3000 + (rand() % 2001);
            spawnedMissilesInWave = 0;
            waveAdvanced = true;
        }

        if (justStarted) {
            justStarted = false;
        }

        // Cập nhật SpaceShark
        for (auto& ss : spaceSharks) {
            if (ss.active) {
                ss.angle += ss.angularSpeed * deltaTime;
                ss.radius -= 20.0f * deltaTime;
                if (ss.radius < 60.0f) ss.radius = 60.0f;
                ss.x = 400 + ss.radius * cos(ss.angle);
                ss.y = 300 + ss.radius * sin(ss.angle);

                if (currentTime - ss.lastBulletTime >= 5000) {
                    SharkBullet sb;
                    sb.x = ss.x;
                    sb.y = ss.y;
                    float distX = 400 - sb.x, distY = 300 - sb.y;
                    float distance = sqrt(distX * distX + distY * distY);
                    if (distance < 1e-6) distance = 1.0f;
                    float bulletSpeed = defaultMissileSpeed * 0.5f;
                    sb.dx = (distX / distance) * bulletSpeed;
                    sb.dy = (distY / distance) * bulletSpeed;
                    sb.active = true;
                    sharkBullets.push_back(sb);
                    ss.lastBulletTime = currentTime;
                }

                if (currentTime - ss.spawnTime >= 15000 || CheckCollisionWithChitbox(ss)) {
                    ss.active = false;
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
                        menu->saveHighscores(score);
                        updateScoreTexture();
                        updateHighscoreTexture();
                    }
                }
                else if (CheckCollisionWithArc(ss)) {
                    ss.active = false;
                    score += 5;
                    updateScoreTexture();
                }
            }
        }

        // Cập nhật SharkBullet
        for (auto& sb : sharkBullets) {
            if (sb.active) {
                sb.x += sb.dx * deltaTime;
                sb.y += sb.dy * deltaTime;
                if (CheckCollisionWithArc(sb)) {
                    sb.active = false;
                }
                else if (CheckCollisionWithChitbox(sb)) {
                    sb.active = false;
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
                        menu->saveHighscores(score);
                        updateScoreTexture();
                        updateHighscoreTexture();
                    }
                }
            }
        }

        // Cập nhật tên lửa thường
        for (auto& t : targets) {
            if (t.active) {
                t.x += t.dx * deltaTime;
                t.y += t.dy * deltaTime;
                if (CheckCollisionWithArc(t)) {
                    t.active = false;
                    score++;
                    updateScoreTexture();
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
                        menu->saveHighscores(score);
                        updateScoreTexture();
                        updateHighscoreTexture();
                    }
                }
            }
        }

        // Cập nhật tên lửa nhanh
        for (auto& fm : fastMissiles) {
            if (fm.active) {
                fm.x += fm.dx * deltaTime;
                fm.y += fm.dy * deltaTime;
                if (CheckCollisionWithArc(fm)) {
                    fm.active = false;
                    score += 2;
                    updateScoreTexture();
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
                        menu->saveHighscores(score);
                        updateScoreTexture();
                        updateHighscoreTexture();
                    }
                }
            }
        }

        targets.erase(
            std::remove_if(targets.begin(), targets.end(), [](Target& t) { return !t.active; }),
            targets.end()
        );
        fastMissiles.erase(
            std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](Target& fm) { return !fm.active; }),
            fastMissiles.end()
        );
        spaceSharks.erase(
            std::remove_if(spaceSharks.begin(), spaceSharks.end(), [](SpaceShark& ss) { return !ss.active; }),
            spaceSharks.end()
        );
        sharkBullets.erase(
            std::remove_if(sharkBullets.begin(), sharkBullets.end(), [](SharkBullet& sb) { return !sb.active; }),
            sharkBullets.end()
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

        for (auto& life : lives) {
            Circle lifeCircle = {life.x, life.y, 10};
            SDL_SetRenderDrawColor(renderer, life.isRed ? 255 : 0, 0, life.isRed ? 0 : 255, 255);
            DrawCircle(renderer, lifeCircle);
        }

        for (auto& t : targets) {
            enemy->renderTarget(t);
        }
        for (auto& fm : fastMissiles) {
            enemy->renderFastMissile(fm);
        }
        for (auto& ss : spaceSharks) {
            enemy->renderSpaceShark(ss);
        }
        for (auto& sb : sharkBullets) {
            enemy->renderSharkBullet(sb);
        }
        if (showWarning) {
            enemy->renderWarning(warningX, warningY, warningStartTime, startTime, totalPausedTime);
        }
    }

    if (pauseButtonTexture) {
        SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton);
    }

    if (gameOver) {
        if (gameOverTextTexture) {
            int w, h;
            SDL_QueryTexture(gameOverTextTexture, NULL, NULL, &w, &h);
            SDL_Rect gameOverRect = {400 - w / 2, 100, w, h};
            SDL_RenderCopy(renderer, gameOverTextTexture, NULL, &gameOverRect);
        }

        if (scoreTexture) {
            int w, h;
            SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = {400 - w / 2, 260, w, h};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }

        if (highscoreTexture) {
            int w, h;
            SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = {400 - w / 2, 300, w, h};
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &restartButton);
        int w, h;
        SDL_QueryTexture(restartTexture, NULL, NULL, &w, &h);
        SDL_Rect restartTextRect = {restartButton.x + (restartButton.w - w) / 2, restartButton.y + (restartButton.h - h) / 2, w, h};
        SDL_RenderCopy(renderer, restartTexture, NULL, &restartTextRect);

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &backToMenuButton);
        SDL_QueryTexture(backToMenuTexture, NULL, NULL, &w, &h);
        SDL_Rect backToMenuTextRect = {backToMenuButton.x + (backToMenuButton.w - w) / 2, backToMenuButton.y + (backToMenuButton.h - h) / 2, w, h};
        SDL_RenderCopy(renderer, backToMenuTexture, NULL, &backToMenuTextRect);
    }
    else if (paused) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_Rect overlay = {0, 0, 800, 600};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (pausedTexture) {
            int w, h;
            SDL_QueryTexture(pausedTexture, NULL, NULL, &w, &h);
            SDL_Rect pausedRect = {800 / 2 - w / 2, 200 - h / 2, w, h};
            SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
        }

        if (scoreTexture) {
            int w, h;
            SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = {400 - w / 2, 260, w, h};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }

        if (highscoreTexture) {
            int w, h;
            SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = {400 - w / 2, 300, w, h};
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }

        if (volumeLabelTexture) {
            int w, h;
            SDL_QueryTexture(volumeLabelTexture, NULL, NULL, &w, &h);
            SDL_Rect volumeLabelRect = {400 - w / 2, 390, w, h};
            SDL_RenderCopy(renderer, volumeLabelTexture, NULL, &volumeLabelRect);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &volumeSlider);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &volumeKnob);

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &giveUpButton);
        int w, h;
        SDL_QueryTexture(giveUpTexture, NULL, NULL, &w, &h);
        SDL_Rect giveUpTextRect = {giveUpButton.x + (giveUpButton.w - w) / 2, giveUpButton.y + (giveUpButton.h - h) / 2, w, h};
        SDL_RenderCopy(renderer, giveUpTexture, NULL, &giveUpTextRect);
    }

    SDL_RenderPresent(renderer);
}

void Game::reset() {
    gameOver = false;
    paused = false;
    showWarning = false;
    isFirstFastMissile = true;
    warningStartTime = 0;
    warningX = 0;
    warningY = 0;
    targets.clear();
    fastMissiles.clear();
    spaceSharks.clear();
    sharkBullets.clear();
    for (auto& life : lives) {
        life.isRed = false;
    }
    missileCount = 1;
    waveCount = 0;
    maxMissileSpeed = defaultMissileSpeed;
    score = 0;
    nextSpawnTime = 2000;
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    arcStartAngle = -PI / 10.3;
    wavesUntilIncrease = 7 + (rand() % 6);
    pauseStartTime = 0;
    totalPausedTime = 0;
    justStarted = false;
    if (backToMenuTexture) {
        SDL_DestroyTexture(backToMenuTexture);
        backToMenuTexture = nullptr;
    }
    if (restartTexture) {
        SDL_DestroyTexture(restartTexture);
        restartTexture = nullptr;
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
        scoreTexture = nullptr;
    }
    if (highscoreTexture) {
        SDL_DestroyTexture(highscoreTexture);
        highscoreTexture = nullptr;
    }
    initTextures();
    updateScoreTexture();
    updateHighscoreTexture();
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

bool Game::CheckCollisionWithArc(SpaceShark& ss) {
    for (double angle = arcStartAngle; angle <= arcStartAngle + 2 * PI / 3; angle += 0.01) {
        int arcX = trajectory.x + trajectory.r * cos(angle);
        int arcY = trajectory.y + trajectory.r * sin(angle);
        int dist = (ss.x - arcX) * (ss.x - arcX) + (ss.y - arcY) * (ss.y - arcY);
        if (dist < 625) return true;
    }
    return false;
}

bool Game::CheckCollisionWithChitbox(SpaceShark& ss) {
    return (ss.x >= chitbox.x && ss.x <= chitbox.x + chitbox.w &&
            ss.y >= chitbox.y && ss.y <= chitbox.y + chitbox.h);
}

bool Game::CheckCollisionWithArc(SharkBullet& sb) {
    for (double angle = arcStartAngle; angle <= arcStartAngle + 2 * PI / 3; angle += 0.01) {
        int arcX = trajectory.x + trajectory.r * cos(angle);
        int arcY = trajectory.y + trajectory.r * sin(angle);
        int dist = (sb.x - arcX) * (sb.x - arcX) + (sb.y - arcY) * (sb.y - arcY);
        if (dist < 100) return true;
    }
    return false;
}

bool Game::CheckCollisionWithChitbox(SharkBullet& sb) {
    return (sb.x >= chitbox.x && sb.x <= chitbox.x + chitbox.w &&
            sb.y >= chitbox.y && sb.y <= chitbox.y + chitbox.h);
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
