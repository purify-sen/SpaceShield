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

// --- C++11 Random Number Generation ---
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);
std::uniform_int_distribution<> dist_wave_increase(0, RANDOM_WAVES_UNTIL_INCREASE -1);
std::uniform_int_distribution<> dist_wave_delay(0, RANDOM_WAVE_DELAY -1);
std::uniform_int_distribution<> dist_side(0, 3);
std::uniform_int_distribution<> dist_y_spawn(0, SCREEN_HEIGHT - 1);
std::uniform_int_distribution<> dist_x_spawn(0, SCREEN_WIDTH - 1);


// Game Constructor (updated to receive sounds)
Game::Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
           Mix_Chunk* sfxShieldHitIn, Mix_Chunk* sfxPlayerHitIn,
           Mix_Chunk* sfxGameOverIn, Mix_Chunk* sfxWarningIn, Mix_Music* bgmGameIn)
    : renderer(r), enemy(e), menu(m),
      // Textures
      mspaceshipTexture(nullptr), pauseButtonTexture(nullptr), scoreTexture(nullptr),
      highscoreTexture(nullptr), pausedTexture(nullptr), backToMenuTexture(nullptr),
      restartTexture(nullptr), gameOverTextTexture(nullptr), volumeLabelTexture(nullptr),
      giveUpTexture(nullptr),
      // Sounds (Store passed pointers)
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

    // *** Call setters in constructor ***
    setVolume(menu->volume);
    setSensitivity(menu->sensitivity); // Call setter

    // Load textures (spaceship, pause button)
    SDL_Surface* spaceshipSurface = IMG_Load(IMG_SPACESHIP.c_str());
    if (!spaceshipSurface) { std::cerr << "Failed to load spaceship: " << IMG_GetError() << std::endl; exit(1); }
    mspaceshipTexture = SDL_CreateTextureFromSurface(renderer, spaceshipSurface);
    SDL_FreeSurface(spaceshipSurface);
    if (!mspaceshipTexture) { std::cerr << "Failed to create spaceship texture: " << SDL_GetError() << std::endl; exit(1); }

    SDL_Surface* pauseButtonSurface = IMG_Load(IMG_PAUSE_BUTTON.c_str());
     if (!pauseButtonSurface) { std::cerr << "Failed to load pause button: " << IMG_GetError() << std::endl; exit(1); }
    pauseButtonTexture = SDL_CreateTextureFromSurface(renderer, pauseButtonSurface);
    SDL_FreeSurface(pauseButtonSurface);
    if (!pauseButtonTexture) { std::cerr << "Failed to create pause button texture: " << SDL_GetError() << std::endl; exit(1); }

    initTextures(); // Initialize text textures
    updateScoreTexture();
    updateHighscoreTexture();
    updatePausedTexture();
    updateGameOverTextTexture();
    updateVolumeLabelTexture();
}

Game::~Game() {
    // Free textures
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
    // Sounds are freed in main.cpp
}

// Initialize text-based textures
void Game::initTextures() {
    TTF_Font* fontLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    TTF_Font* fontXLarge = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_XLARGE);
    TTF_Font* fontNormal = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_NORMAL);

    if (!fontLarge || !fontXLarge || !fontNormal) {
        std::cerr << "TTF_OpenFont failed in Game::initTextures: " << TTF_GetError() << std::endl;
        if(fontLarge) TTF_CloseFont(fontLarge);
        if(fontXLarge) TTF_CloseFont(fontXLarge);
        if(fontNormal) TTF_CloseFont(fontNormal);
        return;
    }

    // Helper lambda to create texture from text
    auto createTexture = [&](const char* text, SDL_Texture*& texture, TTF_Font* font) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, TEXT_COLOR);
        if (!textSurface) {
            std::cerr << "TTF_RenderText_Solid failed for \"" << text << "\": " << TTF_GetError() << std::endl;
            return false;
        }
        if (texture) SDL_DestroyTexture(texture); // Free old texture if exists
        texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed for \"" << text << "\": " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    };

    createTexture("Back to Menu", backToMenuTexture, fontLarge);
    createTexture("Restart", restartTexture, fontLarge);
    createTexture("Give Up", giveUpTexture, fontLarge);
    createTexture("Paused", pausedTexture, fontLarge);
    createTexture("Game over", gameOverTextTexture, fontXLarge);
    createTexture("Volume", volumeLabelTexture, fontNormal);

    // Close fonts
    TTF_CloseFont(fontLarge);
    TTF_CloseFont(fontXLarge);
    TTF_CloseFont(fontNormal);
}

// Update score texture
void Game::updateScoreTexture() {
    std::stringstream ss;
    ss << "Score: " << score;
    std::string scoreStr = ss.str();
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
    if (!font) { std::cerr << "Failed to open font for score: " << TTF_GetError() << std::endl; return; }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), TEXT_COLOR);
    if (!textSurface) { std::cerr << "Failed to render score text: " << TTF_GetError() << std::endl; TTF_CloseFont(font); return; }
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
    if (!scoreTexture) { std::cerr << "Failed to create score texture: " << SDL_GetError() << std::endl; }
}

// Update highscore texture
void Game::updateHighscoreTexture() {
    std::stringstream ss;
    int highscore = menu->highscores.empty() ? 0 : menu->highscores[0];
    ss << "Highscore: " << highscore;
    std::string highscoreStr = ss.str();
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE_LARGE);
     if (!font) { std::cerr << "Failed to open font for highscore: " << TTF_GetError() << std::endl; return; }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, highscoreStr.c_str(), TEXT_COLOR);
    if (!textSurface) { std::cerr << "Failed to render highscore text: " << TTF_GetError() << std::endl; TTF_CloseFont(font); return; }
    if (highscoreTexture) SDL_DestroyTexture(highscoreTexture);
    highscoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
     if (!highscoreTexture) { std::cerr << "Failed to create highscore texture: " << SDL_GetError() << std::endl; }
}

void Game::updatePausedTexture() { /* No update needed as text is static */ }
void Game::updateGameOverTextTexture() { /* No update needed as text is static */ }
void Game::updateVolumeLabelTexture() { /* No update needed as text is static */ }

// Handle game input
void Game::handleInput(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePoint = {mouseX, mouseY};

        // Pause button click
        if (SDL_PointInRect(&mousePoint, &pauseButton)) {
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            if (!paused && !gameOver) { setGameStatePaused(); menu->gameState = MainMenu::PAUSED; }
            else if (paused) { setGameStatePlaying(); menu->gameState = MainMenu::PLAYING; }
        }

        // Handling when paused
        if (paused) {
            // Drag volume knob
            if (SDL_PointInRect(&mousePoint, &volumeKnob)) {
                isDraggingVolume = true;
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            }
            // Give Up button click
            if (SDL_PointInRect(&mousePoint, &giveUpButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                triggerGameOver();
                menu->gameState = MainMenu::GAME_OVER;
            }
        }

        // Handling when Game Over
        if (gameOver) {
            // Back to Menu button click
            if (SDL_PointInRect(&mousePoint, &backToMenuButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset();
                menu->gameState = MainMenu::MENU;
                 Mix_HaltMusic(); // Stop game music
                 if (menu->bgmMenu) Mix_PlayMusic(menu->bgmMenu, -1); // Play menu music
            }
            // Restart button click
            if (SDL_PointInRect(&mousePoint, &restartButton)) {
                 if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
                reset();
                startGame(); // Start new game (will play game music)
                menu->gameState = MainMenu::PLAYING;
            }
        }
    }
    // Mouse button release
    if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingVolume) {
            isDraggingVolume = false;
            menu->volume = volume; // Update volume in menu
            menu->saveSettings();  // Save settings
        }
     }
    // Mouse motion (dragging volume)
    if (event.type == SDL_MOUSEMOTION && isDraggingVolume) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        int newKnobX = mouseX - volumeKnob.w / 2;
        // Clamp knob position within slider bounds
        newKnobX = std::max(volumeSlider.x, std::min(newKnobX, volumeSlider.x + volumeSlider.w - volumeKnob.w));
        volumeKnob.x = newKnobX;
        // Calculate new volume value
        int newVolume = static_cast<int>(round(((float)(newKnobX - volumeSlider.x) / (volumeSlider.w - volumeKnob.w)) * 100.0f));
        newVolume = std::max(0, std::min(newVolume, 100)); // Clamp volume 0-100
        setVolume(newVolume); // Update volume and UI
     }
    // ESC key handling
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (!paused && !gameOver) { // If playing, pause
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            setGameStatePaused();
            menu->gameState = MainMenu::PAUSED;
        } else if (paused) { // If paused, resume
             if (menu->sfxButtonClick) Mix_PlayChannel(CHANNEL_SFX, menu->sfxButtonClick, 0);
            setGameStatePlaying();
            menu->gameState = MainMenu::PLAYING;
        }
        // ESC does nothing if game over
     }
}

// Update game logic
void Game::update(float deltaTime) {
    // Don't update if game over, not started, or paused
    if (gameOver || startTime == 0 || paused) return;

    Uint32 currentTime = SDL_GetTicks() - startTime - totalPausedTime;

    // Update shield control
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    float sensitivityFactor = MIN_SENSITIVITY_MULTIPLIER + (static_cast<float>(sensitivity) / 100.0f) * (MAX_SENSITIVITY_MULTIPLIER - MIN_SENSITIVITY_MULTIPLIER);
    if (keys[SDL_SCANCODE_A]) arcStartAngle -= SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;
    if (keys[SDL_SCANCODE_D]) arcStartAngle += SHIELD_ROTATION_SPEED_FACTOR * deltaTime * sensitivityFactor;

    // --- Enemy Spawning ---
    // Spawn SpaceShark
    if (waveCount >= WAVE_START_SHARK && (waveCount - WAVE_START_SHARK) % WAVE_INTERVAL_SHARK == 0 && spaceSharks.empty()) {
        SpaceShark ss;
        ss.radius = SHARK_INITIAL_RADIUS;
        ss.angle = static_cast<float>(dis(gen)) * 2.0f * PI; // Random angle
        ss.angularSpeed = SHARK_ANGULAR_SPEED;
        ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
        ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);
        ss.spawnTime = currentTime;
        ss.lastBulletTime = currentTime; // Start shooting interval
        ss.active = true;
        spaceSharks.push_back(ss);
    }

    // Spawn Fast Missile - Warning management
    if (waveCount >= WAVE_START_FAST_MISSILE && (waveCount - WAVE_START_FAST_MISSILE) % WAVE_INTERVAL_FAST_MISSILE == 0 && fastMissiles.empty() && !showWarning) {
        showWarning = true;
        warningStartTime = currentTime;
         // Play warning sound (looping)
         if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1);
        // Choose random edge for warning
        int side = dist_side(gen);
        switch (side) {
            case 0: warningX = LIFE_ICON_RADIUS; warningY = dist_y_spawn(gen); break; // Left
            case 1: warningX = SCREEN_WIDTH - LIFE_ICON_RADIUS; warningY = dist_y_spawn(gen); break; // Right
            case 2: warningX = dist_x_spawn(gen); warningY = LIFE_ICON_RADIUS; break; // Top
            case 3: warningX = dist_x_spawn(gen); warningY = SCREEN_HEIGHT - LIFE_ICON_RADIUS; break; // Bottom
        }
    }

    // Activate fast missile after warning duration
    if (showWarning && (currentTime - warningStartTime >= FAST_MISSILE_WARNING_DURATION)) {
        showWarning = false;
         // Stop warning sound
         Mix_HaltChannel(CHANNEL_WARNING);
        Target fm; // Fast missile uses Target struct
        fm.x = static_cast<float>(warningX);
        fm.y = static_cast<float>(warningY);
        // Calculate direction vector towards center
        float distX = static_cast<float>(TRAJECTORY_CENTER.x) - fm.x;
        float distY = static_cast<float>(TRAJECTORY_CENTER.y) - fm.y;
        float distance = sqrt(distX * distX + distY * distY);
        if (distance < 1e-6f) distance = 1.0f; // Avoid division by zero
        // Calculate fast missile speed
        float baseSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
        float missileSpeed = baseSpeed * FAST_MISSILE_SPEED_MULTIPLIER;
        fm.dx = (distX / distance) * missileSpeed;
        fm.dy = (distY / distance) * missileSpeed;
        fm.active = true;
        fastMissiles.push_back(fm);
    }

    // Spawn Normal Missiles (Targets) and advance wave
    bool waveAdvanced = false;
    // Only spawn if game not just started and it's time to spawn
    if (!justStarted && currentTime >= nextSpawnTime) {
        // Check if enough missiles have been spawned for this wave
        if (spawnedMissilesInWave < missileCount) {
            // Check minimum interval between missiles
            if (currentTime - lastMissileSpawnTime >= MISSILE_SPAWN_INTERVAL || spawnedMissilesInWave == 0) {
                Target t;
                // Choose random spawn edge
                int side = dist_side(gen);
                 switch (side) {
                    case 0: t.x = 0.0f; t.y = static_cast<float>(dist_y_spawn(gen)); break;
                    case 1: t.x = static_cast<float>(SCREEN_WIDTH); t.y = static_cast<float>(dist_y_spawn(gen)); break;
                    case 2: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = 0.0f; break;
                    case 3: t.x = static_cast<float>(dist_x_spawn(gen)); t.y = static_cast<float>(SCREEN_HEIGHT); break;
                }
                // Calculate direction vector towards center
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - t.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - t.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                // Calculate normal missile speed (with random factor)
                float missileSpeed = DEFAULT_MISSILE_SPEED * (1.0f + static_cast<float>(dis(gen)) * MAX_MISSILE_SPEED_RANDOM_FACTOR);
                t.dx = (distX / distance) * missileSpeed;
                t.dy = (distY / distance) * missileSpeed;
                t.active = true;
                targets.push_back(t);
                spawnedMissilesInWave++;
                lastMissileSpawnTime = currentTime;
            }
        }
        // If enough missiles spawned and wave not yet advanced -> advance wave
        else if (!waveAdvanced) {
            waveCount++;
            // Check if missile count needs to increase
            if (waveCount % wavesUntilIncrease == 0) {
                missileCount++;
                if (missileCount > MAX_MISSILE_COUNT) missileCount = MAX_MISSILE_COUNT;
                // Recalculate random waves until next increase
                wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen);
            }
            // Set time for the next wave
            nextSpawnTime = currentTime + BASE_WAVE_DELAY + dist_wave_delay(gen);
            spawnedMissilesInWave = 0; // Reset counter for the new wave
            waveAdvanced = true; // Mark wave as advanced for this frame
        }
    }
    // Reset justStarted flag after the first update
    if (justStarted) justStarted = false;

    // --- Update Enemy Positions and States ---
    // Update SpaceSharks
    for (auto& ss : spaceSharks) {
        if (ss.active) {
            // Update position based on spiral trajectory
            ss.angle += ss.angularSpeed * deltaTime;
            ss.radius += SHARK_SPIRAL_SPEED * deltaTime; // Radius decreases
            if (ss.radius < SHARK_MIN_RADIUS) ss.radius = SHARK_MIN_RADIUS; // Clamp minimum radius
            ss.x = TRAJECTORY_CENTER.x + ss.radius * cos(ss.angle);
            ss.y = TRAJECTORY_CENTER.y + ss.radius * sin(ss.angle);

            // Fire shark bullets at intervals
            if (currentTime - ss.lastBulletTime >= SHARK_BULLET_INTERVAL) {
                SharkBullet sb;
                sb.x = ss.x; sb.y = ss.y;
                // Aim bullet towards the center (player ship)
                float distX = static_cast<float>(TRAJECTORY_CENTER.x) - sb.x;
                float distY = static_cast<float>(TRAJECTORY_CENTER.y) - sb.y;
                float distance = sqrt(distX * distX + distY * distY);
                if (distance < 1e-6f) distance = 1.0f;
                // Shark bullet speed
                float bulletSpeed = DEFAULT_MISSILE_SPEED * SHARK_BULLET_SPEED_MULTIPLIER;
                sb.dx = (distX / distance) * bulletSpeed;
                sb.dy = (distY / distance) * bulletSpeed;
                sb.active = true;
                sharkBullets.push_back(sb);
                ss.lastBulletTime = currentTime; // Reset firing timer
            }

            // Check collisions or lifetime expiration
            if (CheckCollisionWithChitbox(ss)) { // Collision with player ship
                ss.active = false;
                HandleHit(); // Player takes damage
            } else if (CheckCollisionWithArc(ss)) { // Collision with shield
                ss.active = false;
                score += SCORE_PER_SHARK; // Add score
                updateScoreTexture();
                 // Play shield hit sound
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            } else if (currentTime - ss.spawnTime >= SHARK_LIFETIME) { // Lifetime expired
                 ss.active = false;
            }
        }
    }

    // Update SharkBullets
    for (auto& sb : sharkBullets) {
        if (sb.active) {
            sb.x += sb.dx * deltaTime; sb.y += sb.dy * deltaTime;
            // Check collisions
            if (CheckCollisionWithChitbox(sb)) { // Collision with player ship
                sb.active = false;
                HandleHit(); // Player takes damage
            } else if (CheckCollisionWithArc(sb)) { // Collision with shield
                sb.active = false;
                 // Play shield hit sound
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            }
            // Optional: Remove bullet if it goes off-screen
            else if (sb.x < -SHARK_BULLET_WIDTH || sb.x > SCREEN_WIDTH + SHARK_BULLET_WIDTH ||
                       sb.y < -SHARK_BULLET_HEIGHT || sb.y > SCREEN_HEIGHT + SHARK_BULLET_HEIGHT) {
                sb.active = false;
            }
        }
    }

    // Update Normal Missiles (Targets)
    for (auto& t : targets) {
        if (t.active) {
            t.x += t.dx * deltaTime; t.y += t.dy * deltaTime;
            // Check collisions
            if (CheckCollisionWithChitbox(t)) { // Collision with player ship
                t.active = false;
                HandleHit(); // Player takes damage
            } else if (CheckCollisionWithArc(t)) { // Collision with shield
                t.active = false;
                score += SCORE_PER_MISSILE; // Add score
                updateScoreTexture();
                 // Play shield hit sound
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            }
        }
    }

    // Update Fast Missiles
    for (auto& fm : fastMissiles) {
        if (fm.active) {
            fm.x += fm.dx * deltaTime; fm.y += fm.dy * deltaTime;
            // Check collisions
            if (CheckCollisionWithChitbox(fm)) { // Collision with player ship
                fm.active = false;
                HandleHit(); // Player takes damage
            } else if (CheckCollisionWithArc(fm)) { // Collision with shield
                fm.active = false;
                score += SCORE_PER_FAST_MISSILE; // Add score
                updateScoreTexture();
                 // Play shield hit sound
                 if (sfxShieldHit) Mix_PlayChannel(CHANNEL_SFX, sfxShieldHit, 0);
            }
        }
    }

    // --- Remove Inactive Objects ---
    // Use erase-remove idiom for efficiency
    targets.erase(std::remove_if(targets.begin(), targets.end(), [](const Target& t){ return !t.active; }), targets.end());
    fastMissiles.erase(std::remove_if(fastMissiles.begin(), fastMissiles.end(), [](const Target& fm){ return !fm.active; }), fastMissiles.end());
    spaceSharks.erase(std::remove_if(spaceSharks.begin(), spaceSharks.end(), [](const SpaceShark& ss){ return !ss.active; }), spaceSharks.end());
    sharkBullets.erase(std::remove_if(sharkBullets.begin(), sharkBullets.end(), [](const SharkBullet& sb){ return !sb.active; }), sharkBullets.end());
}

// Render the game
void Game::render() {
    // Clear screen (black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // --- Render when playing ---
    if (!gameOver && !paused) {
        // Render player ship
        if (mspaceshipTexture) { SDL_RenderCopy(renderer, mspaceshipTexture, NULL, &chitbox); }
        // Render trajectory circle and shield arc
        SDL_SetRenderDrawColor(renderer, TRAJECTORY_CIRCLE_COLOR.r, TRAJECTORY_CIRCLE_COLOR.g, TRAJECTORY_CIRCLE_COLOR.b, TRAJECTORY_CIRCLE_COLOR.a);
        DrawCircle(renderer, trajectory);
        DrawArc(renderer, trajectory, arcStartAngle, SHIELD_ARC_ANGLE); // Draws the shield

        // Render lives icons
        for (const auto& life : lives) {
            Circle lifeCircle = {life.x + LIFE_ICON_RADIUS, life.y + LIFE_ICON_RADIUS, LIFE_ICON_RADIUS};
            const SDL_Color& color = life.isRed ? LIFE_ICON_INACTIVE_COLOR : LIFE_ICON_ACTIVE_COLOR;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            // Draw filled circle for life icon
            for (int w = 0; w < lifeCircle.r * 2; w++) {
                for (int h = 0; h < lifeCircle.r * 2; h++) {
                    int dx = lifeCircle.r - w; int dy = lifeCircle.r - h;
                    if ((dx*dx + dy*dy) <= (lifeCircle.r * lifeCircle.r)) {
                        SDL_RenderDrawPoint(renderer, lifeCircle.x + dx, lifeCircle.y + dy);
                    }
                }
            }
        }

        // Render enemies (using Enemy class's render methods)
        for (const auto& t : targets) { enemy->renderTarget(t); }
        for (const auto& fm : fastMissiles) { enemy->renderFastMissile(fm); }
        for (const auto& ss : spaceSharks) { enemy->renderSpaceShark(ss); }
        for (const auto& sb : sharkBullets) { enemy->renderSharkBullet(sb); }

        // Render warning icon if active
        if (showWarning) { enemy->renderWarning(static_cast<float>(warningX), static_cast<float>(warningY), warningStartTime, startTime, totalPausedTime); }

        // Render score and highscore textures in top-right corner
        if (scoreTexture) {
            int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = {SCREEN_WIDTH - w - 10, 60, w, h};
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
         if (highscoreTexture) {
            int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = {SCREEN_WIDTH - w - 10, 60 + h + 5, w, h}; // Position below score
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }
    } // End rendering when playing

    // --- Render Pause Button (always visible unless game over) ---
    if (!gameOver && pauseButtonTexture) { SDL_RenderCopy(renderer, pauseButtonTexture, NULL, &pauseButton); }

    // --- Render Game Over Screen ---
    if (gameOver) {
        // Dim overlay
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Enable alpha blending
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Disable alpha blending

        // Render "Game Over" title
        if (gameOverTextTexture) {
            int w, h; SDL_QueryTexture(gameOverTextTexture, NULL, NULL, &w, &h);
            SDL_Rect gameOverRect = { (SCREEN_WIDTH - w) / 2, GAMEOVER_TITLE_Y, w, h };
            SDL_RenderCopy(renderer, gameOverTextTexture, NULL, &gameOverRect);
        }
        // Render final score
        if (scoreTexture) {
            int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = { (SCREEN_WIDTH - w) / 2, SCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
        // Render highscore
        if (highscoreTexture) {
            int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = { (SCREEN_WIDTH - w) / 2, HIGHSCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }

        // Render Restart button
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &restartButton);
        if (restartTexture) {
            int w, h; SDL_QueryTexture(restartTexture, NULL, NULL, &w, &h);
            SDL_Rect restartTextRect = { restartButton.x + (restartButton.w - w) / 2, restartButton.y + (restartButton.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, restartTexture, NULL, &restartTextRect);
        }
        // Render Back to Menu button
        SDL_RenderFillRect(renderer, &backToMenuButton);
        if (backToMenuTexture) {
            int w, h; SDL_QueryTexture(backToMenuTexture, NULL, NULL, &w, &h);
            SDL_Rect backToMenuTextRect = { backToMenuButton.x + (backToMenuButton.w - w) / 2, backToMenuButton.y + (backToMenuButton.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, backToMenuTexture, NULL, &backToMenuTextRect);
        }
    }
    // --- Render Pause Screen ---
    else if (paused) {
        // Dim overlay
        SDL_SetRenderDrawColor(renderer, PAUSE_OVERLAY_COLOR.r, PAUSE_OVERLAY_COLOR.g, PAUSE_OVERLAY_COLOR.b, PAUSE_OVERLAY_COLOR.a);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Render "Paused" title
        if (pausedTexture) {
            int w, h; SDL_QueryTexture(pausedTexture, NULL, NULL, &w, &h);
            SDL_Rect pausedRect = { (SCREEN_WIDTH - w) / 2, PAUSED_TITLE_Y, w, h };
            SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
        }
        // Render current score
        if (scoreTexture) {
            int w, h; SDL_QueryTexture(scoreTexture, NULL, NULL, &w, &h);
            SDL_Rect scoreRect = { (SCREEN_WIDTH - w) / 2, SCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
        // Render highscore
        if (highscoreTexture) {
            int w, h; SDL_QueryTexture(highscoreTexture, NULL, NULL, &w, &h);
            SDL_Rect highscoreRect = { (SCREEN_WIDTH - w) / 2, HIGHSCORE_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
        }
        // Render Volume label
        if (volumeLabelTexture) {
            int w, h; SDL_QueryTexture(volumeLabelTexture, NULL, NULL, &w, &h);
            SDL_Rect volumeLabelRect = { (SCREEN_WIDTH - w) / 2, VOLUME_LABEL_Y, w, h };
            SDL_RenderCopy(renderer, volumeLabelTexture, NULL, &volumeLabelRect);
        }

        // Render Volume slider background
        SDL_SetRenderDrawColor(renderer, SLIDER_BG_COLOR.r, SLIDER_BG_COLOR.g, SLIDER_BG_COLOR.b, SLIDER_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &volumeSlider);
        // Render Volume knob (changes color when dragged)
        const SDL_Color& knobColor = isDraggingVolume ? SLIDER_KNOB_DRAG_COLOR : SLIDER_KNOB_COLOR;
        SDL_SetRenderDrawColor(renderer, knobColor.r, knobColor.g, knobColor.b, knobColor.a);
        SDL_RenderFillRect(renderer, &volumeKnob);

        // Render Give Up button
        SDL_SetRenderDrawColor(renderer, BUTTON_COLOR.r, BUTTON_COLOR.g, BUTTON_COLOR.b, BUTTON_COLOR.a);
        SDL_RenderFillRect(renderer, &giveUpButton);
        if (giveUpTexture) {
            int w, h; SDL_QueryTexture(giveUpTexture, NULL, NULL, &w, &h);
            SDL_Rect giveUpTextRect = { giveUpButton.x + (giveUpButton.w - w) / 2, giveUpButton.y + (giveUpButton.h - h) / 2, w, h };
            SDL_RenderCopy(renderer, giveUpTexture, NULL, &giveUpTextRect);
        }
    }

    // Present the rendered frame
    SDL_RenderPresent(renderer);
}

// Reset game state
void Game::reset() {
    gameOver = false;
    paused = false;
    showWarning = false;
    warningStartTime = 0;
    warningX = 0; warningY = 0;
    targets.clear(); fastMissiles.clear(); spaceSharks.clear(); sharkBullets.clear();
    for (auto& life : lives) life.isRed = false; // Reset lives
    missileCount = INITIAL_MISSILE_COUNT;
    waveCount = 0;
    score = 0;
    nextSpawnTime = INITIAL_SPAWN_DELAY; // Initial delay before first wave
    spawnedMissilesInWave = 0;
    lastMissileSpawnTime = 0;
    arcStartAngle = INITIAL_SHIELD_START_ANGLE;
    wavesUntilIncrease = BASE_WAVES_UNTIL_INCREASE + dist_wave_increase(gen); // Rerandomize
    startTime = 0; // Will be set by startGame()
    pauseStartTime = 0;
    totalPausedTime = 0;
    justStarted = false; // Will be set by startGame()
    updateScoreTexture();
    updateHighscoreTexture();
    // *** Call setters in reset ***
    setVolume(menu->volume);
    setSensitivity(menu->sensitivity); // Call setter

    // Stop game music and any warning sounds
    Mix_HaltMusic();
    Mix_HaltChannel(CHANNEL_WARNING);
}

// Start a new game
void Game::startGame() {
    startTime = SDL_GetTicks(); // Record start time
    totalPausedTime = 0;      // Reset total paused time
    pauseStartTime = 0;
    justStarted = true;       // Flag for first update cycle
    gameOver = false;
    paused = false;
    updateScoreTexture();     // Update score display (to 0)
    updateHighscoreTexture(); // Update highscore display

    // Stop any currently playing music (e.g., menu music)
    Mix_HaltMusic();
    // Play game music (looping)
    if (bgmGame) {
        Mix_PlayMusic(bgmGame, -1);
    }
}

// --- Collision Checking Functions ---
// (Keep the CheckCollision functions as they were)
bool Game::CheckCollisionWithArc(const Target& t) {
    float targetCenterX = t.x;
    float targetCenterY = t.y;
    float dx = targetCenterX - trajectory.x;
    float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float outerRadiusSq = (trajectory.r + 10.0f) * (trajectory.r + 10.0f); // Add some margin
    float innerRadiusSq = (trajectory.r - 10.0f) * (trajectory.r - 10.0f); // Add some margin
    if (distSq > outerRadiusSq || distSq < innerRadiusSq) return false; // Quick distance check

    float targetAngle = atan2(dy, dx);
    // Normalize angles to [0, 2*PI) for easier comparison
    float normalizedArcStart = fmod(arcStartAngle, 2.0f * PI);
    if (normalizedArcStart < 0) normalizedArcStart += 2.0f * PI;
    float normalizedArcEnd = fmod(arcStartAngle + SHIELD_ARC_ANGLE, 2.0f * PI);
     if (normalizedArcEnd < 0) normalizedArcEnd += 2.0f * PI;
    float normalizedTargetAngle = fmod(targetAngle, 2.0f * PI);
    if (normalizedTargetAngle < 0) normalizedTargetAngle += 2.0f * PI;

    // Check if target angle is within the shield arc angle
    if (normalizedArcStart <= normalizedArcEnd) { // Arc doesn't cross 0 angle
        if (normalizedTargetAngle >= normalizedArcStart && normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq; // Check distance again
        }
    } else { // Arc crosses 0 angle (e.g., from 300 to 60 degrees)
        if (normalizedTargetAngle >= normalizedArcStart || normalizedTargetAngle <= normalizedArcEnd) {
             return distSq <= outerRadiusSq && distSq >= innerRadiusSq;
        }
    }
    return false;
}
bool Game::CheckCollisionWithChitbox(const Target& t) {
     // Simple AABB collision check using a small hitbox around the target's center
     SDL_Rect targetRect = { (int)t.x - 2, (int)t.y - 2, 5, 5 }; // Small hitbox
    return SDL_HasIntersection(&targetRect, &chitbox);
}
bool Game::CheckCollisionWithArc(const SpaceShark& ss) {
    // Similar logic to Target collision, but using shark's collision radius
    float targetCenterX = ss.x; float targetCenterY = ss.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float collisionRadius = sqrt(SHARK_COLLISION_RADIUS_SQ);
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius);
     if (innerRadiusSq < 0) innerRadiusSq = 0; // Avoid negative radius
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
    // AABB collision between shark's visual rect and player's chitbox
    SDL_Rect sharkRect = { (int)ss.x - SHARK_CENTER.x, (int)ss.y - SHARK_CENTER.y, SHARK_WIDTH, SHARK_HEIGHT };
    return SDL_HasIntersection(&sharkRect, &chitbox);
}
bool Game::CheckCollisionWithArc(const SharkBullet& sb) {
    // Similar logic to Target collision, using shark bullet's collision radius
    float targetCenterX = sb.x; float targetCenterY = sb.y;
    float dx = targetCenterX - trajectory.x; float dy = targetCenterY - trajectory.y;
    float distSq = dx * dx + dy * dy;
    float collisionRadius = sqrt(SHARK_BULLET_COLLISION_RADIUS_SQ);
    float outerRadiusSq = (trajectory.r + collisionRadius) * (trajectory.r + collisionRadius);
    float innerRadiusSq = (trajectory.r - collisionRadius) * (trajectory.r - collisionRadius);
     if (innerRadiusSq < 0) innerRadiusSq = 0;
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
    // AABB collision between bullet's visual rect and player's chitbox
    SDL_Rect bulletRect = { (int)sb.x - SHARK_BULLET_CENTER.x, (int)sb.y - SHARK_BULLET_CENTER.y, SHARK_BULLET_WIDTH, SHARK_BULLET_HEIGHT };
    return SDL_HasIntersection(&bulletRect, &chitbox);
}

// --- Utility Functions ---
// Draw a circle outline
void Game::DrawCircle(SDL_Renderer* renderer, const Circle& c) {
    SDL_Point points[CIRCLE_SEGMENTS + 1];
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float rad = (2.0f * PI * i) / CIRCLE_SEGMENTS;
        points[i].x = c.x + static_cast<int>(c.r * cos(rad));
        points[i].y = c.y + static_cast<int>(c.r * sin(rad));
    }
    SDL_RenderDrawLines(renderer, points, CIRCLE_SEGMENTS + 1);
}
// Draw an arc outline (for the shield)
void Game::DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle) {
    SDL_SetRenderDrawColor(renderer, SHIELD_ARC_COLOR.r, SHIELD_ARC_COLOR.g, SHIELD_ARC_COLOR.b, SHIELD_ARC_COLOR.a); // Set shield color
    SDL_Point points[ARC_SEGMENTS + 1];
    for (int i = 0; i <= ARC_SEGMENTS; ++i) {
        double angle = startAngle + (arcAngle * i / ARC_SEGMENTS);
        points[i].x = c.x + static_cast<int>(c.r * cos(angle));
        points[i].y = c.y + static_cast<int>(c.r * sin(angle));
    }
    SDL_RenderDrawLines(renderer, points, ARC_SEGMENTS + 1);
}

// Handle player getting hit
void Game::HandleHit() {
    if (gameOver) return; // Don't handle hits if already game over

    // Play player hit sound effect
    if (sfxPlayerHit) Mix_PlayChannel(CHANNEL_SFX, sfxPlayerHit, 0);

    // *** FIX: Restore life handling logic ***
    // Find the first active life (not red) and mark it as lost (red)
    for (auto& life : lives) {
        if (!life.isRed) {
            life.isRed = true;
            goto checkGameOver; // Exit loop after finding and marking one life
        }
    }

checkGameOver: // Label to jump to after processing hit
    // *** FIX: Restore game over check logic ***
    // Check if all lives are lost
    bool allRed = true; // *** Define allRed variable here ***
    for (const auto& life : lives) {
        if (!life.isRed) {
            allRed = false; // Found an active life
            break;
        }
    }

    // If all lives are lost, trigger game over
    if (allRed) { // *** Now allRed is defined and has a value ***
        triggerGameOver(); // triggerGameOver handles game over sound
        menu->gameState = MainMenu::GAME_OVER; // Update state for MainMenu
    }
}

// Setter for volume (updates knob position and mixer volume)
void Game::setVolume(int vol) {
    if (vol >= 0 && vol <= 100) {
        volume = vol;
        // Update knob position based on volume
        int knobRange = volumeSlider.w - volumeKnob.w;
        volumeKnob.x = volumeSlider.x + static_cast<int>(round(((float)volume / 100.0f) * knobRange));
        // Set actual mixer volume for music and sound effects
        Mix_VolumeMusic(volume * MIX_MAX_VOLUME / 100); // MIX_MAX_VOLUME is usually 128
        Mix_Volume(-1, volume * MIX_MAX_VOLUME / 100); // -1 applies to all chunk channels
    }
 }

// Functions for MainMenu to manage game state
void Game::setGameStatePlaying() {
     if (paused) { // Only resume if currently paused
        if (pauseStartTime != 0) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime; // Add paused duration
        }
        paused = false;
        pauseStartTime = 0;
        Mix_ResumeMusic(); // Resume game music
        if (showWarning) { // Resume warning sound if warning was active
             if (sfxWarning) Mix_PlayChannel(CHANNEL_WARNING, sfxWarning, -1);
        }
     }
}
void Game::setGameStatePaused() {
    if (!paused && !gameOver) { // Only pause if currently playing
        pauseStartTime = SDL_GetTicks(); // Record pause start time
        paused = true;
         Mix_PauseMusic(); // Pause game music
         Mix_HaltChannel(CHANNEL_WARNING); // Stop warning sound when paused
    }
}
void Game::triggerGameOver() {
    if (!gameOver) { // Only trigger once
         gameOver = true;
         // Stop game music, stop warning sound, play game over sound
         Mix_HaltMusic();
         Mix_HaltChannel(CHANNEL_WARNING);
         if (sfxGameOver) Mix_PlayChannel(CHANNEL_SFX, sfxGameOver, 0);

         // Save highscore immediately
         menu->saveHighscores(score);
         updateScoreTexture(); // Update display
         updateHighscoreTexture();
         // If paused when game over is triggered, reset pause state
         if(paused) {
            totalPausedTime += SDL_GetTicks() - pauseStartTime;
            pauseStartTime = 0;
            paused = false;
         }
    }
}

// *** FIX: Add definition for setSensitivity ***
// Setter for sensitivity
void Game::setSensitivity(int sens) {
    if (sens >= 0 && sens <= 100) { // Clamp sensitivity 0-100
        sensitivity = sens;
        // No immediate action needed here; the sensitivity value
        // is read directly in the update() function when calculating shield rotation.
    }
}
