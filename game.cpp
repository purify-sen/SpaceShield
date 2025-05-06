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
#include <memory> 


std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0); 
std::uniform_int_distribution<> dist_wave_increase(0, RANDOM_WAVES_UNTIL_INCREASE -1); 
std::uniform_int_distribution<> dist_wave_delay(0, RANDOM_WAVE_DELAY -1); 
std::uniform_int_distribution<> dist_side(0, 3); 
std::uniform_int_distribution<> dist_y_spawn(0, SCREEN_HEIGHT - 1); 
std::uniform_int_distribution<> dist_x_spawn(0, SCREEN_WIDTH - 1); 

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


Game::Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
           Mix_Chunk* sfxShieldHitIn, Mix_Chunk* sfxPlayerHitIn,
           Mix_Chunk* sfxGameOverIn, Mix_Chunk* sfxWarningIn,
           Mix_Chunk* sfxHealCollectIn, 
           Mix_Music* bgmGameIn,
           SDL_Texture* bgTexture)
    : renderer(r), enemy(e), menu(m),

      mspaceshipTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr),
      highscoreTexture(nullptr), pausedTexture(nullptr), backToMenuTexture(nullptr),
      restartTexture(nullptr), gameOverTextTexture(nullptr), volumeLabelTexture(nullptr),
      giveUpTexture(nullptr), backgroundTexture(bgTexture),

      allyShipTexture(nullptr), healItemTexture(nullptr),

      sfxShieldHit(sfxShieldHitIn), sfxPlayerHit(sfxPlayerHitIn),
      sfxGameOver(sfxGameOverIn), sfxWarning(sfxWarningIn),

      sfxHealCollect(sfxHealCollectIn), 
      bgmGame(bgmGameIn),
      gameOver(false), paused(false), showWarning(false),
      justStarted(false),

      startTime(0), pauseStartTime(0), totalPausedTime(0), warningStartTime(0),
      nextSpawnTime(INITIAL_SPAWN_DELAY), lastMissileSpawnTime(0),

      lastAllySpawnTime(0), 
 
      score(0), missileCount(INITIAL_MISSILE_COUNT), waveCount(0),
      spawnedMissilesInWave(0),

      warningX(0), warningY(0), arcStartAngle(INITIAL_SHIELD_START_ANGLE),

      volume(DEFAULT_VOLUME), sensitivity(static_cast<int>(DEFAULT_SENSITIVITY)), isDraggingVolume(false),

      chitbox(PLAYER_CHITBOX), pauseButton(PAUSE_BUTTON_RECT),
      backToMenuButton(BACK_TO_MENU_BUTTON_RECT_GAMEOVER), restartButton(RESTART_BUTTON_RECT),
      giveUpButton(GIVE_UP_BUTTON_RECT), volumeSlider(VOLUME_SLIDER_RECT),
      volumeKnob(VOLUME_KNOB_RECT),

      trajectory{TRAJECTORY_CENTER.x, TRAJECTORY_CENTER.y, TRAJECTORY_RADIUS}

{
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);

    lives.clear();
    for (int i = 0; i < PLAYER_LIVES; ++i) {
        Life life = {LIFE_ICON_START_X + i * LIFE_ICON_SPACING, LIFE_ICON_START_Y, false};
        lives.push_back(life);
    }


    setVolume(menu->volume);
    setSensitivity(menu->sensitivity);


    mspaceshipTexture = loadTexture(renderer, IMG_SPACESHIP);
    pauseButtonTexture = loadTexture(renderer, IMG_PAUSE_BUTTON);

    allyShipTexture = loadTexture(renderer, IMG_ALLY_SHIP);
    healItemTexture = loadTexture(renderer, IMG_HEAL_ITEM);


    if (!mspaceshipTexture) { std::cerr << "Error loading spaceship texture!" << std::endl;}
    if (!pauseButtonTexture) { std::cerr << "Error loading pause button texture!" << std::endl;}
    if (!allyShipTexture) { std::cerr << "Error loading ally ship texture!" << std::endl;}
    if (!healItemTexture) { std::cerr << "Error loading heal item texture!" << std::endl;}


    initTextures(); 
    updateScoreTexture();
    updateHighscoreTexture();
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
    if (allyShipTexture) SDL_DestroyTexture(allyShipTexture);
    if (healItemTexture) SDL_DestroyTexture(healItemTexture);
}


void Game::initTextures() {
    TTF_Font* fontLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    TTF_Font* fontXLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_XLARGE);
    TTF_Font* fontNormal = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);


    if (!fontLarge || !fontXLarge || !fontNormal) {
        std::cerr << "TTF_OpenFont failed in Game::initTextures: " << TTF_GetError() << std::endl;
        if(fontLarge) TTF_CloseFont(fontLarge);
        if(fontXLarge) TTF_CloseFont(fontXLarge);
        if(fontNormal) TTF_CloseFont(fontNormal);

        pausedTexture = nullptr;
        backToMenuTexture = nullptr;
        restartTexture = nullptr;
        gameOverTextTexture = nullptr;
        volumeLabelTexture = nullptr;
        giveUpTexture = nullptr;
        scoreTexture = nullptr;
        highscoreTexture = nullptr;
        return;
    }


    auto createTextureHelper = [&](const char* text, SDL_Texture*& texture, TTF_Font* fontToUse) {
        if (!fontToUse) {
             std::cerr << "Error: Attempting to create texture \"" << text << "\" with a null font." << std::endl;
             texture = nullptr; return false;
        }
        SDL_Surface* textSurface = TTF_RenderText_Solid(fontToUse, text, TEXT_COLOR);
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
            texture = nullptr; return false; 
        }
        if (texture) SDL_DestroyTexture(texture); 
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    };

    if (!createTextureHelper("Back to Menu", backToMenuTexture, fontLarge)) { std::cerr << "Error creating back to menu texture." << std::endl; }
    if (!createTextureHelper("Restart", restartTexture, fontLarge)) { std::cerr << "Error creating restart texture." << std::endl; }
    if (!createTextureHelper("Give Up", giveUpTexture, fontLarge)) { std::cerr << "Error creating give up texture." << std::endl; }
    if (!createTextureHelper("Paused", pausedTexture, fontLarge)) { std::cerr << "Error creating paused texture." << std::endl; }
    if (!createTextureHelper("Game over", gameOverTextTexture, fontXLarge)) { std::cerr << "Error creating game over texture." << std::endl; }
    if (!createTextureHelper("Volume", volumeLabelTexture, fontNormal)) { std::cerr << "Error creating volume label texture." << std::endl; }

    TTF_CloseFont(fontLarge);
    TTF_CloseFont(fontXLarge);
    TTF_CloseFont(fontNormal);
}


void Game::updateScoreTexture() {
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_SMALL); 
    if (!font) {
        std::cerr << "Failed to open font for score (small): " << TTF_GetError() << std::endl;
         if (scoreTexture) SDL_DestroyTexture(scoreTexture);
         scoreTexture = nullptr;
        return;
    }
    std::stringstream ss;
    ss << "Score: " << score;
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, ss.str().c_str(), TEXT_COLOR);
    TTF_CloseFont(font); 
    if (!textSurface) {
        std::cerr << "Failed to render score text (small): " << TTF_GetError() << std::endl;
        if (scoreTexture) SDL_DestroyTexture(scoreTexture);
        scoreTexture = nullptr;
        return;
    }
    if (scoreTexture) SDL_DestroyTexture(scoreTexture); 
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!scoreTexture) {
        std::cerr << "Failed to create score texture (small): " << SDL_GetError() << std::endl;
    }
}


void Game::updateHighscoreTexture() {
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_SMALL); 
     if (!font) {
         std::cerr << "Failed to open font for highscore (small): " << TTF_GetError() << std::endl;
         if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
         highscoreTexture = nullptr;
         return;
     }
    std::stringstream ss;
    int highscore = (menu && !menu->highscores.empty()) ? menu->highscores[0] : 0;
    ss << "Highscore: " << highscore;
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, ss.str().c_str(), TEXT_COLOR);
    TTF_CloseFont(font);
    if (!textSurface) {
        std::cerr << "Failed to render highscore text (small): " << TTF_GetError() << std::endl;
         if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
         highscoreTexture = nullptr;
        return;
    }
    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
     if (!highscoreTexture) {
         std::cerr << "Failed to create highscore texture (small): " << SDL_GetError() << std::endl;
     }
}

void Game::updatePausedTexture() { }
void Game::updateGameOverTextTexture() { }
void Game::updateVolumeLabelTexture() { }

void Game::handleInput(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        if (!gameOver && SDL_PointInRect(&mousePoint, &pauseButton)) {
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            if (!paused) { setGameStatePaused(); menu->gameState = MainMenu::PAUSED; }
            else { setGameStatePlaying(); menu->gameState = MainMenu::PLAYING; }
            return; 
        }

        if (paused) {
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                isDraggingVolume = true;
                return; 
            }
            if (SDL_PointInRect(&mousePoint, &giveUpButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                triggerGameOver(); 
                menu->gameState = MainMenu::GAME_OVER; 
                return; 
            }
        }

        if (gameOver) {
            if (SDL_PointInRect(&mousePoint, &backToMenuButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset(); 
                menu->gameState = MainMenu::MENU; 
                 Mix_HaltMusic(); 
                 if (menu->bgmMenu) Mix_PlayMusic(menu->bgmMenu, -1); 
                return; 
            }
            if (SDL_PointInRect(&mousePoint, &restartButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset(); 
                startGame(); 
                menu->gameState = MainMenu::PLAYING; 
                return; 
            }
        }
    }
    if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingVolume) {
            isDraggingVolume = false;
            if (menu) {
                menu->volume = volume;
                menu->saveSettings();
            }
        }
     }
    if (event.type == SDL_MOUSEMOTION && isDraggingVolume) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        int newKnobX = mouseX - volumeKnob.w / 2;
        int knobRange = volumeSlider.w - volumeKnob.w;
        if (knobRange > 0) { 
            newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + knobRange));
            volumeKnob.x = newKnobX;
            int newVolume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / knobRange) * 100.0f));
            newVolume = std::max(0, std::min(newVolume, 100));
            setVolume(newVolume); 
        }
     }
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (!gameOver) { 
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

void Game::update(float deltaTime) {
    if (gameOver || startTime == 0 || paused) return;

    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    const Uint8* keys = SDL_GetKeyboardState(NULL);
    float sensitivityFactor = MIN_SENSITIVITY_MULTIPLIER + (static_cast<float>(sensitivity) / 100.0f) * (MAX_SENSITIVITY_MULTIPLIER - MIN_SENSITIVITY_MULTIPLIER);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    arcStartAngle = fmod(arcStartAngle, 2.0f * PI);
    if (arcStartAngle < 0) arcStartAngle += 2.0f * PI;

    if (currentTime - lastAllySpawnTime >= ALLY_SPAWN_INTERVAL) {
        SpawnAlly();
        lastAllySpawnTime = currentTime; 
    }

    if (waveCount >= WAVE_START_SHARK && (waveCount - WAVE_START_SHARK) % WAVE_INTERVAL_SHARK == 0 && spaceSharks.empty()) {
        SpaceShark ss;
        ss.radius = SHARK_INITIAL_RADIUS;
        ss.angle = static_cast<float>(dis(gen)) * 2.0f * PI; 
        ss.angularSpeed = (dis(gen) > 0.5 ? 1.0f : -1.0f) * SHARK_ANGULAR_SPEED;
        ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
        ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);
        ss.spawnTime = currentTime;
        ss.lastBulletTime = currentTime; 
        ss.active = true;
        spaceSharks.push_back(ss);
    }

    if (waveCount >= WAVE_START_FAST_MISSILE && (waveCount - WAVE_START_FAST_MISSILE) % WAVE_INTERVAL_FAST_MISSILE == 0 && fastMissiles.empty() && !showWarning) {
        showWarning = true;
        warningStartTime = currentTime;
         if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1); 
        int side = dist_side(gen);
        switch (side) {
            case 0: warningX = WARNING_ICON_WIDTH / 2; warningY = dist_y_spawn(gen); break; 
            case 1: warningX = SCREEN_WIDTH - WARNING_ICON_WIDTH / 2; warningY = dist_y_spawn(gen); break; 
            case 2: warningX = dist_x_spawn(gen); warningY = WARNING_ICON_HEIGHT / 2; break; 
            case 3: warningX = dist_x_spawn(gen); warningY = SCREEN_HEIGHT - WARNING_ICON_HEIGHT / 2; break; 
        }
    }

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

    bool waveAdvanced = false; 
    if (!justStarted && currentTime >= nextSpawnTime) {
        if (spawnedMissilesInWave < missileCount) {
            if (currentTime - lastMissileSpawnTime >= MISSILE_SPAWN_INTERVAL || spawnedMissilesInWave == 0) {
                Target t;
                int side = dist_side(gen);
                 switch (side) {
                    case 0: t.x = 0.0f - MISSILE_WIDTH; t.y = static_cast<float>(dist_y_spawn(gen)); break; 
                    case 1: t.x = static_cast<float>(SCREEN_WIDTH); t.y = static_cast<float>(dist_y_spawn(gen)); break; 
                    case 2: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = 0.0f - MISSILE_HEIGHT; break; 
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
            if (waveCount > 0 && waveCount % wavesUntilIncrease == 0) { 
                missileCount++;
                if (missileCount > MAX_MISSILE_COUNT) missileCount = MAX_MISSILE_COUNT; 
                wavesUntilIncrease = waveCount + BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
            }
            nextSpawnTime = currentTime + BASE_WAVE_DELAY + dist_wave_delay(gen);
            spawnedMissilesInWave = 0; 
            waveAdvanced = true; 
        }
    }
    if (justStarted) justStarted = false;
    for (auto& ally : allies) {
        if (ally.active) {
            ally.x += ally.speed * deltaTime;

            if (ally.x > SCREEN_WIDTH) {
                ally.active = false; 
                continue; 
            }
            if (!ally.droppingHeal && ally.x >= chitbox.x && ally.x <= chitbox.x + chitbox.w) {
                HealItem heal;
                heal.x = ally.x + ALLY_WIDTH / 2 - HEAL_ITEM_WIDTH / 2;
                heal.y = ally.y + ALLY_HEIGHT; 
                heal.speed = HEAL_ITEM_DROP_SPEED;
                heal.active = true;
                healItems.push_back(heal); 
                ally.droppingHeal = true; 
            }
        }
    }

    for (auto& heal : healItems) {
        if (heal.active) {
            heal.y += heal.speed * deltaTime;

            if (heal.y > SCREEN_HEIGHT) {
                heal.active = false;
                continue;
            }

            if (CheckCollisionWithChitbox(heal)) {
                 HandleHealCollection(heal); 
            }
        }
    }


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
            }
            else if (CheckCollisionWithArc(ss)) {
                ss.active = false; 
                score += SCORE_PER_SHARK; 
                updateScoreTexture(); 
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); 
            }
            else if (currentTime - ss.spawnTime >= SHARK_LIFETIME) {
                 ss.active = false; 
            }
        }
    }

    for (auto& sb : sharkBullets) {
        if (sb.active) {
            sb.x += sb.dx * deltaTime; sb.y += sb.dy * deltaTime;
            if (CheckCollisionWithChitbox(sb)) {
                sb.active = false; 
                HandleHit(); 
            }
            else if (CheckCollisionWithArc(sb)) {
                sb.active = false; 
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); 
            }
            else if (sb.x < -SHARK_BULLET_WIDTH || sb.x > SCREEN_WIDTH + SHARK_BULLET_WIDTH ||
                       sb.y < -SHARK_BULLET_HEIGHT || sb.y > SCREEN_HEIGHT + SHARK_BULLET_HEIGHT) {
                sb.active = false; 
            }
        }
    }

    for (auto& t : targets) {
        if (t.active) {
            t.x += t.dx * deltaTime; t.y += t.dy * deltaTime;
            if (CheckCollisionWithChitbox(t)) {
                t.active = false; 
                HandleHit(); 
            }
            else if (CheckCollisionWithArc(t)) {
                t.active = false; 
                score += SCORE_PER_MISSILE; 
                updateScoreTexture(); 
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); 
            }
        }
    }

    for (auto& fm : fastMissiles) {
        if (fm.active) {
            fm.x += fm.dx * deltaTime; fm.y += fm.dy * deltaTime;
            if (CheckCollisionWithChitbox(fm)) {
                fm.active = false; 
                HandleHit(); 
            }
            else if (CheckCollisionWithArc(fm)) {
                fm.active = false; 
                score += SCORE_PER_FAST_MISSILE; 
                updateScoreTexture(); 
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0); 
            }
        }
    }

    targets.erase(std::remove_if(targets.begin(), targets.end(), [](const Target& t){ return !t.active; }), targets.end());
    fastMissiles.erase(std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](const Target& fm){ return !fm.active; }), fastMissiles.end());
    spaceSharks.erase(std::remove_if(spaceSharks.begin(), spaceSharks.end(), [](const SpaceShark& ss){ return !ss.active; }), spaceSharks.end());
    sharkBullets.erase(std::remove_if(sharkBullets.begin(), sharkBullets.end(), [](const SharkBullet& sb){ return !sb.active; }), sharkBullets.end());
    allies.erase(std::remove_if(allies.begin(), allies.end(), [](const AllyShip& a){ return !a.active; }), allies.end());
    healItems.erase(std::remove_if(healItems.begin(), healItems.end(), [](const HealItem& h){ return !h.active; }), healItems.end());
}

void Game::render() {
    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL); 
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

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
                        SDL_RenderDrawPoint(renderer, lifeCircle.x + dx - lifeCircle.r, lifeCircle.y + dy - lifeCircle.r);
                    }
                }
            }
        }

        for (const auto& t : targets) { enemy->renderTarget(t); }
        for (const auto& fm : fastMissiles) { enemy->renderFastMissile(fm); }
        for (const auto& ss : spaceSharks) { enemy->renderSpaceShark(ss); }
        for (const auto& sb : sharkBullets) { enemy->renderSharkBullet(sb); }

        if (showWarning) {
            enemy->renderWarning(static_cast<float>(warningX), static_cast<float>(warningY), warningStartTime, startTime, totalPausedTime);
        }

        for (const auto& ally : allies) {
            if (ally.active && allyShipTexture) {
                SDL_Rect allyRect = { (int)ally.x, (int)ally.y, ALLY_WIDTH, ALLY_HEIGHT };
                SDL_RenderCopy(renderer, allyShipTexture, NULL, &allyRect);
            }
        }
        for (const auto& heal : healItems) {
            if (heal.active && healItemTexture) {
                SDL_Rect healRect = { (int)heal.x, (int)heal.y, HEAL_ITEM_WIDTH, HEAL_ITEM_HEIGHT };
                SDL_RenderCopy(renderer, healItemTexture, NULL, &healRect);
            }
        }

        if (scoreTexture) {
            int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = {SCREEN_WIDTH - w - INGAME_SCORE_TEXT_PADDING_X, 
                                  INGAME_SCORE_TEXT_Y,                          
                                  w, h};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);

            if (highscoreTexture) {
                int hw, hh; SDL_QueryTexture(highscoreTexture, NULL, NULL, &hw, &hh);
                SDL_Rect highscoreRect = {SCREEN_WIDTH - hw - INGAME_SCORE_TEXT_PADDING_X,      
                                          scoreRect.y + h + INGAME_HIGHSCORE_TEXT_Y_OFFSET, 
                                          hw, hh};
                SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
            }
        }

    } 

    if (!gameOver && pauseButtonTexture) { SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton); }

    auto renderTextureCentered = [&](SDL_Texture* texture, const SDL_Rect& rect) {
        if (!texture) {
            std::cerr << "Warning: Trying to render a null texture centered." << std::endl;
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &rect);
            return;
        }
        int w, h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
    };
    auto renderTextureAt = [&](SDL_Texture* texture, int x, int y, bool centerAlign = true) {
        if (!texture) {
             std::cerr << "Warning: Trying to render a null texture at (" << x << ", " << y << ")." << std::endl;
             SDL_Rect placeholder = { x - 50, y - 15, 100, 30};
             SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); SDL_RenderFillRect(renderer, &placeholder);
             return;
        }
        int w, h; SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect destRect = { x, y, w, h };
        if (centerAlign) { destRect.x = x - w / 2; } 
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
    };


    if (gameOver) {
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); 
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); 

        renderTextureAt(gameOverTextTexture, SCREEN_WIDTH / 2, GAMEOVER_TITLE_Y);
        renderTextureAt(scoreTexture, SCREEN_WIDTH / 2, SCORE_LABEL_Y); 
        renderTextureAt(highscoreTexture, SCREEN_WIDTH / 2, HIGHSCORE_LABEL_Y); 

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &restartButton);
        renderTextureCentered(restartTexture, restartButton);

        SDL_RenderFillRect(renderer, &backToMenuButton);
        renderTextureCentered(backToMenuTexture, backToMenuButton);
    }
    else if (paused) {
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        renderTextureAt(pausedTexture, SCREEN_WIDTH / 2, PAUSED_TITLE_Y);
        renderTextureAt(scoreTexture, SCREEN_WIDTH / 2, SCORE_LABEL_Y); 
        renderTextureAt(highscoreTexture, SCREEN_WIDTH / 2, HIGHSCORE_LABEL_Y); 

        renderTextureAt(volumeLabelTexture, volumeSlider.x, VOLUME_LABEL_Y, false);
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider); 
        const SDL_Color& knobColor = isDraggingVolume ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR; 
        SDL_SetRenderDrawColor(renderer, knobColor.r, knobColor.g, knobColor.b, knobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob); 

        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &giveUpButton);
        renderTextureCentered(giveUpTexture, giveUpButton);
    }

    SDL_RenderPresent(renderer); 
}


void Game::reset() {
    gameOver = false;
    paused = false;
    showWarning = false;
    warningStartTime = 0;
    warningX = 0; warningY = 0;
    targets.clear(); fastMissiles.clear(); spaceSharks.clear(); sharkBullets.clear();
    allies.clear();
    healItems.clear();
    for (auto& life : lives) life.isRed = false;
    missileCount = INITIAL_MISSILE_COUNT;
    waveCount = 0;
    score = 0;
    nextSpawnTime = INITIAL_SPAWN_DELAY; 
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    lastAllySpawnTime = 0;
    arcStartAngle = INITIAL_SHIELD_START_ANGLE; 
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
    startTime = 0; 
    pauseStartTime = 0;
    totalPausedTime = 0;
    justStarted = false; 
    isDraggingVolume = false; 
    updateScoreTexture();
    updateHighscoreTexture();
    if (menu) {
        setVolume(menu->volume);
        setSensitivity(menu->sensitivity);
    }
    Mix_HaltMusic();
    Mix_HaltChannel(CHANNEL_WARNING); 
}

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
    } else {
        std::cerr << "Warning: Game BGM not loaded, cannot play." << std::endl;
    }
}

bool Game::CheckCollisionWithArc(const Target& t) {
    if (!t.active) return false;
    float targetCenterX = t.x; float targetCenterY = t.y; 
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy; 

    float collisionRadius = sqrt(MISSILE_COLLISION_RADIUS_SQ); 

    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius);
    if (innerRadiusSq < 0) innerRadiusSq = 0;

    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false;

    float targetAngle = atan2(dy, dx);
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI); if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI); if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI); if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;

    if (normalizedArcStart <= normalizedArcEnd) {
        return (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd);
    }
    else {
        return (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd);
    }
    return false; 
}
bool Game::CheckCollisionWithChitbox(const Target& t) {
     if (!t.active) return false;
     SDL_Rect targetRect = { (int)(t.x - 2), (int)(t.y - 2), 5, 5 };
    return SDL_HasIntersection(&targetRect, &chitbox);
}
bool Game::CheckCollisionWithArc(const SpaceShark& ss) {
    if (!ss.active) return false;
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
    if (normalizedArcStart <= normalizedArcEnd) { return (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd); }
    else { return (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd); }
    return false;
}
bool Game::CheckCollisionWithChitbox(const SpaceShark& ss) {
    if (!ss.active) return false;
    SDL_Rect sharkRect = { (int)(ss.x - SHARK_CENTER.x), (int)(ss.y - SHARK_CENTER.y), SHARK_WIDTH, SHARK_HEIGHT };
    return SDL_HasIntersection(&sharkRect, &chitbox);
}
bool Game::CheckCollisionWithArc(const SharkBullet& sb) {
    if (!sb.active) return false;
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
    if (normalizedArcStart <= normalizedArcEnd) { return (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd); }
    else { return (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd); }
    return false;
}
bool Game::CheckCollisionWithChitbox(const SharkBullet& sb) {
    if (!sb.active) return false;
    SDL_Rect bulletRect = { (int)(sb.x - SHARK_BULLET_CENTER.x), (int)(sb.y - SHARK_BULLET_CENTER.y), SHARK_BULLET_WIDTH, SHARK_BULLET_HEIGHT };
    return SDL_HasIntersection(&bulletRect, &chitbox);
}

bool Game::CheckCollisionWithChitbox(const HealItem& hi) {
    if (!hi.active) return false;
    SDL_Rect healRect = { (int)hi.x, (int)hi.y, HEAL_ITEM_WIDTH, HEAL_ITEM_HEIGHT };
    return SDL_HasIntersection(&healRect, &chitbox);
}


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
        if (menu) menu->gameState = MainMenu::GAME_OVER; 
    }
}

void Game::HandleHealCollection(HealItem& heal) {
    if (!heal.active) return; 

    heal.active = false; 

    if (sfxHealCollect) {
        Mix_PlayChannel(CHANNEL_SFX, sfxHealCollect, 0);
    }

    for (auto& life : lives) {
        if (life.isRed) {
            life.isRed = false; 
            break; 
        }
    }
}


void Game::SpawnAlly() {
    AllyShip ally;
    ally.x = 0.0f - ALLY_WIDTH; 
    ally.y = 10.0f;         
    ally.speed = ALLY_SPEED;
    ally.active = true;
    ally.droppingHeal = false; 
    allies.push_back(ally);   
}


void Game::setVolume(int vol) {
    if (vol >= 0 && vol <= 100) {
        volume = vol;
        int knobRange = volumeSlider.w - volumeKnob.w;
         if (knobRange > 0) {
             volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRange));
        } else {
            volumeKnob.x = volumeSlider.x; 
        }
        Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100); 
        Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100); 
    }
 }

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
         if (menu) menu->saveHighscores(score);
         updateScoreTexture();
         updateHighscoreTexture();
         if(paused) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            pauseStartTime = 0;
            paused = false; 
         }
    }
}

void Game::setSensitivity(int sens) {
    if (sens >= 0 && sens <= 100) {
        sensitivity = sens;
    }
}

bool Game::isDraggingVolumeSlider() const {
    return isDraggingVolume;
}