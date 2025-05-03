#include "mainmenu.h"
#include "game.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

MainMenu::MainMenu(SDL_Renderer* r, TTF_Font* f) 
    : renderer(r), font(f), titleTexture(nullptr), playButtonTexture(nullptr), 
      highscoreButtonTexture(nullptr), settingsButtonTexture(nullptr), exitButtonTexture(nullptr), 
      highscoreTitleTexture(nullptr), highscoreListTexture(nullptr), settingsTitleTexture(nullptr), 
      backButtonTexture(nullptr), volumeTexture(nullptr), sensitivityTexture(nullptr), 
      volume(100), sensitivity(50), isDraggingKnob(false), isDraggingSensitivityKnob(false), gameState(MENU) {
    SDL_Color textColor = {255, 255, 255, 255};

    SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Space Shield", textColor);
    titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_FreeSurface(titleSurface);

    SDL_Surface* playSurface = TTF_RenderText_Solid(font, "Play", textColor);
    playButtonTexture = SDL_CreateTextureFromSurface(renderer, playSurface);
    SDL_FreeSurface(playSurface);

    SDL_Surface* highscoreSurface = TTF_RenderText_Solid(font, "Highscore", textColor);
    highscoreButtonTexture = SDL_CreateTextureFromSurface(renderer, highscoreSurface);
    SDL_FreeSurface(highscoreSurface);

    SDL_Surface* settingsSurface = TTF_RenderText_Solid(font, "Settings", textColor);
    settingsButtonTexture = SDL_CreateTextureFromSurface(renderer, settingsSurface);
    SDL_FreeSurface(settingsSurface);

    SDL_Surface* exitSurface = TTF_RenderText_Solid(font, "Exit", textColor);
    exitButtonTexture = SDL_CreateTextureFromSurface(renderer, exitSurface);
    SDL_FreeSurface(exitSurface);

    SDL_Surface* highscoreTitleSurface = TTF_RenderText_Solid(font, "Highscore", textColor);
    highscoreTitleTexture = SDL_CreateTextureFromSurface(renderer, highscoreTitleSurface);
    SDL_FreeSurface(highscoreTitleSurface);

    SDL_Surface* settingsTitleSurface = TTF_RenderText_Solid(font, "Settings", textColor);
    settingsTitleTexture = SDL_CreateTextureFromSurface(renderer, settingsTitleSurface);
    SDL_FreeSurface(settingsTitleSurface);

    SDL_Surface* backSurface = TTF_RenderText_Solid(font, "Back", textColor);
    backButtonTexture = SDL_CreateTextureFromSurface(renderer, backSurface);
    SDL_FreeSurface(backSurface);

    SDL_Surface* volumeSurface = TTF_RenderText_Solid(font, "Volume: 100", textColor);
    volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
    SDL_FreeSurface(volumeSurface);

    SDL_Surface* sensitivitySurface = TTF_RenderText_Solid(font, "Sensitivity: 50", textColor);
    sensitivityTexture = SDL_CreateTextureFromSurface(renderer, sensitivitySurface);
    SDL_FreeSurface(sensitivitySurface);

    Mix_Volume(-1, volume * 128 / 100);
    loadHighscores();
    loadSettings();
    updateHighscoreListTexture();
    updateVolumeTexture();
    updateSensitivityTexture();
}

MainMenu::~MainMenu() {
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (playButtonTexture) SDL_DestroyTexture(playButtonTexture);
    if (highscoreButtonTexture) SDL_DestroyTexture(highscoreButtonTexture);
    if (settingsButtonTexture) SDL_DestroyTexture(settingsButtonTexture);
    if (exitButtonTexture) SDL_DestroyTexture(exitButtonTexture);
    if (highscoreTitleTexture) SDL_DestroyTexture(highscoreTitleTexture);
    if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
    if (settingsTitleTexture) SDL_DestroyTexture(settingsTitleTexture);
    if (backButtonTexture) SDL_DestroyTexture(backButtonTexture);
    if (volumeTexture) SDL_DestroyTexture(volumeTexture);
    if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
}

void MainMenu::loadHighscores() {
    std::ifstream file(playerDataFile);
    highscores.clear();
    if (!file.is_open()) {
        std::cerr << "Failed to open " << playerDataFile << " for reading" << std::endl;
        while (highscores.size() < 5) {
            highscores.push_back(0);
        }
        return;
    }
    int score;
    while (file >> score && highscores.size() < 5) {
        highscores.push_back(score);
    }
    file.close();
    while (highscores.size() < 5) {
        highscores.push_back(0);
    }
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
}

void MainMenu::saveHighscores(int score) {
    std::filesystem::create_directories("playerdata");

    highscores.push_back(score);
    std::sort(highscores.begin(), highscores.end(), std::greater<int>());
    if (highscores.size() > 5) highscores.resize(5);

    std::ofstream file(playerDataFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << playerDataFile << " for writing" << std::endl;
        return;
    }
    for (int s : highscores) {
        file << s << "\n";
    }
    file << "Volume: " << volume << "\n";
    file << "Sensitivity: " << sensitivity << "\n";
    file.close();
}

void MainMenu::loadSettings() {
    std::ifstream file(playerDataFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("Volume: ") == 0) {
                volume = std::stoi(line.substr(8));
                volumeKnob.x = volumeSlider.x + (volume * volumeSlider.w / 100) - volumeKnob.w / 2;
                Mix_Volume(-1, volume * 128 / 100);
            }
            else if (line.find("Sensitivity: ") == 0) {
                sensitivity = std::stoi(line.substr(13));
                sensitivityKnob.x = sensitivitySlider.x + (sensitivity * sensitivitySlider.w / 100) - sensitivityKnob.w / 2;
            }
        }
        file.close();
    }
}

void MainMenu::saveSettings() {
    std::filesystem::create_directories("playerdata");

    std::ofstream file(playerDataFile);
    if (file.is_open()) {
        for (int s : highscores) {
            file << s << "\n";
        }
        file << "Volume: " << volume << "\n";
        file << "Sensitivity: " << sensitivity << "\n";
        file.close();
    }
}

void MainMenu::updateHighscoreListTexture() {
    std::stringstream ss;
    for (size_t i = 0; i < highscores.size(); ++i) {
        ss << (i + 1) << ". " << highscores[i];
        if (i < highscores.size() - 1) ss << "\n";
    }
    std::string highscoreListStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, highscoreListStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (highscoreListTexture) SDL_DestroyTexture(highscoreListTexture);
    highscoreListTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
}

void MainMenu::updateVolumeTexture() {
    std::stringstream ss;
    ss << "Volume: " << volume;
    std::string volumeStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, volumeStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (volumeTexture) SDL_DestroyTexture(volumeTexture);
    volumeTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
}

void MainMenu::updateSensitivityTexture() {
    std::stringstream ss;
    ss << "Sensitivity: " << sensitivity;
    std::string sensitivityStr = ss.str();

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, sensitivityStr.c_str(), textColor);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl;
        return;
    }

    if (sensitivityTexture) SDL_DestroyTexture(sensitivityTexture);
    sensitivityTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
}

void MainMenu::handleInput(SDL_Event& event, bool& running, Game& game) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (gameState == MENU) {
            if (mouseX >= playButton.x && mouseX <= playButton.x + playButton.w &&
                mouseY >= playButton.y && mouseY <= playButton.y + playButton.h) {
                gameState = PLAYING;
                game.startGame();
                game.reset();
            }
            else if (mouseX >= highscoreButton.x && mouseX <= highscoreButton.x + highscoreButton.w &&
                     mouseY >= highscoreButton.y && mouseY <= highscoreButton.y + highscoreButton.h) {
                gameState = HIGHSCORE;
            }
            else if (mouseX >= settingsButton.x && mouseX <= settingsButton.x + settingsButton.w &&
                     mouseY >= settingsButton.y && mouseY <= settingsButton.y + highscoreButton.h) {
                gameState = SETTINGS;
            }
            else if (mouseX >= exitButton.x && mouseX <= exitButton.x + exitButton.w &&
                     mouseY >= exitButton.y && mouseY <= exitButton.y + exitButton.h) {
                running = false;
            }
        }
        else if (gameState == HIGHSCORE || gameState == SETTINGS) {
            if (mouseX >= backButton.x && mouseX <= backButton.x + backButton.w &&
                mouseY >= backButton.y && mouseY <= backButton.y + backButton.h) {
                saveSettings();
                gameState = MENU;
                isDraggingKnob = false;
                isDraggingSensitivityKnob = false;
            }
        }

        if (gameState == SETTINGS) {
            if (mouseX >= volumeKnob.x && mouseX <= volumeKnob.x + volumeKnob.w &&
                mouseY >= volumeKnob.y && mouseY <= volumeKnob.y + volumeKnob.h) {
                isDraggingKnob = true;
            }
            if (mouseX >= sensitivityKnob.x && mouseX <= sensitivityKnob.x + sensitivityKnob.w &&
                mouseY >= sensitivityKnob.y && mouseY <= sensitivityKnob.y + sensitivityKnob.h) {
                isDraggingSensitivityKnob = true;
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        if (isDraggingKnob || isDraggingSensitivityKnob) {
            saveSettings();
        }
        isDraggingKnob = false;
        isDraggingSensitivityKnob = false;
    }
    else if (event.type == SDL_MOUSEMOTION && isDraggingKnob) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        int newX = mouseX;
        if (newX < volumeSlider.x) newX = volumeSlider.x;
        if (newX > volumeSlider.x + volumeSlider.w) newX = volumeSlider.x + volumeSlider.w;

        volumeKnob.x = newX - volumeKnob.w / 2;

        volume = ((newX - volumeSlider.x) * 100) / volumeSlider.w;
        Mix_Volume(-1, volume * 128 / 100);
        updateVolumeTexture();
        game.setVolume(volume); // Đồng bộ volume với Game
    }
    else if (event.type == SDL_MOUSEMOTION && isDraggingSensitivityKnob) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        int newX = mouseX;
        if (newX < sensitivitySlider.x) newX = sensitivitySlider.x;
        if (newX > sensitivitySlider.x + sensitivitySlider.w) newX = sensitivitySlider.x + sensitivitySlider.w;

        sensitivityKnob.x = newX - sensitivityKnob.w / 2;

        sensitivity = ((newX - sensitivitySlider.x) * 100) / sensitivitySlider.w;
        updateSensitivityTexture();
        game.setSensitivity(sensitivity);
    }
}

void MainMenu::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    auto renderTexture = [&](SDL_Texture* texture, SDL_Rect& rect) {
        if (texture) {
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
        }
    };

    if (gameState == MENU) {
        if (titleTexture) {
            int w, h;
            SDL_QueryTexture(titleTexture, NULL, NULL, &w, &h);
            SDL_Rect titleRect = {400 - w / 2, 100, w, h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        }
        if (playButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &playButton);
            renderTexture(playButtonTexture, playButton);
        }
        if (highscoreButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &highscoreButton);
            renderTexture(highscoreButtonTexture, highscoreButton);
        }
        if (settingsButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &settingsButton);
            renderTexture(settingsButtonTexture, settingsButton);
        }
        if (exitButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &exitButton);
            renderTexture(exitButtonTexture, exitButton);
        }
    }
    else if (gameState == HIGHSCORE) {
        if (highscoreTitleTexture) {
            int w, h;
            SDL_QueryTexture(highscoreTitleTexture, NULL, NULL, &w, &h);
            SDL_Rect titleRect = {400 - w / 2, 100, w, h};
            SDL_RenderCopy(renderer, highscoreTitleTexture, NULL, &titleRect);
        }
        if (highscoreListTexture) {
            int w, h;
            SDL_QueryTexture(highscoreListTexture, NULL, NULL, &w, &h);
            SDL_Rect listRect = {400 - w / 2, 200, w, h};
            SDL_RenderCopy(renderer, highscoreListTexture, NULL, &listRect);
        }
        if (backButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &backButton);
            renderTexture(backButtonTexture, backButton);
        }
    }
    else if (gameState == SETTINGS) {
        if (settingsTitleTexture) {
            int w, h;
            SDL_QueryTexture(settingsTitleTexture, NULL, NULL, &w, &h);
            SDL_Rect titleRect = {400 - w / 2, 100, w, h};
            SDL_RenderCopy(renderer, settingsTitleTexture, NULL, &titleRect);
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &volumeSlider);

        SDL_SetRenderDrawColor(renderer, isDraggingKnob ? 255 : 200, isDraggingKnob ? 255 : 200, isDraggingKnob ? 0 : 255, 255);
        SDL_RenderFillRect(renderer, &volumeKnob);

        if (volumeTexture) {
            int w, h;
            SDL_QueryTexture(volumeTexture, NULL, NULL, &w, &h);
            SDL_Rect volumeRect = {volumeSlider.x, volumeSlider.y - 40, w, h};
            SDL_RenderCopy(renderer, volumeTexture, NULL, &volumeRect);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &sensitivitySlider);

        SDL_SetRenderDrawColor(renderer, isDraggingSensitivityKnob ? 255 : 200, isDraggingSensitivityKnob ? 255 : 200, isDraggingSensitivityKnob ? 0 : 255, 255);
        SDL_RenderFillRect(renderer, &sensitivityKnob);

        if (sensitivityTexture) {
            int w, h;
            SDL_QueryTexture(sensitivityTexture, NULL, NULL, &w, &h);
            SDL_Rect sensitivityRect = {sensitivitySlider.x, sensitivitySlider.y - 40, w, h};
            SDL_RenderCopy(renderer, sensitivityTexture, NULL, &sensitivityRect);
        }

        if (backButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &backButton);
            renderTexture(backButtonTexture, backButton);
        }
    }

    SDL_RenderPresent(renderer);
}