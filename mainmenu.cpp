#include "mainmenu.h"
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
      backButtonTexture(nullptr), volumeTexture(nullptr), volume(64), isDraggingKnob(false), gameState(MENU) {
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

    Mix_Volume(-1, volume);
    loadHighscores();
    updateHighscoreListTexture();
    updateVolumeTexture();
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
}

void MainMenu::loadHighscores() {
    std::ifstream file(playerDataFile);
    highscores.clear();
    if (file.is_open()) {
        int score;
        while (file >> score && highscores.size() < 5) {
            highscores.push_back(score);
        }
        file.close();
    }
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
    if (file.is_open()) {
        for (int s : highscores) {
            file << s << "\n";
        }
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

void MainMenu::handleInput(SDL_Event& event, bool& running) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (gameState == MENU) {
            if (mouseX >= playButton.x && mouseX <= playButton.x + playButton.w &&
                mouseY >= playButton.y && mouseY <= playButton.y + playButton.h) {
                gameState = PLAYING;
            }
            else if (mouseX >= highscoreButton.x && mouseX <= highscoreButton.x + highscoreButton.w &&
                     mouseY >= highscoreButton.y && mouseY <= highscoreButton.y + highscoreButton.h) {
                gameState = HIGHSCORE;
            }
            else if (mouseX >= settingsButton.x && mouseX <= settingsButton.x + settingsButton.w &&
                     mouseY >= settingsButton.y && mouseY <= settingsButton.y + settingsButton.h) {
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
                gameState = MENU;
                isDraggingKnob = false;
            }
        }

        if (gameState == SETTINGS) {
            if (mouseX >= volumeKnob.x && mouseX <= volumeKnob.x + volumeKnob.w &&
                mouseY >= volumeKnob.y && mouseY <= volumeKnob.y + volumeKnob.h) {
                isDraggingKnob = true;
            }
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

void MainMenu::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

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
            int w, h;
            SDL_QueryTexture(playButtonTexture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {playButton.x + (playButton.w - w) / 2, playButton.y + (playButton.h - h) / 2, w, h};
            SDL_RenderCopy(renderer, playButtonTexture, NULL, &textRect);
        }
        if (highscoreButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &highscoreButton);
            int w, h;
            SDL_QueryTexture(highscoreButtonTexture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {highscoreButton.x + (highscoreButton.w - w) / 2, highscoreButton.y + (highscoreButton.h - h) / 2, w, h};
            SDL_RenderCopy(renderer, highscoreButtonTexture, NULL, &textRect);
        }
        if (settingsButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &settingsButton);
            int w, h;
            SDL_QueryTexture(settingsButtonTexture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {settingsButton.x + (highscoreButton.w - w) / 2, settingsButton.y + (settingsButton.h - h) / 2, w, h};
            SDL_RenderCopy( renderer, settingsButtonTexture, NULL, &textRect);
        }
        if (exitButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &exitButton);
            int w, h;
            SDL_QueryTexture(exitButtonTexture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {exitButton.x + (exitButton.w - w) / 2, exitButton.y + (exitButton.h - h) / 2, w, h};
            SDL_RenderCopy(renderer, exitButtonTexture, NULL, &textRect);
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
            int w, h;
            SDL_QueryTexture(backButtonTexture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {backButton.x + (backButton.w - w) / 2, backButton.y + (backButton.h - h) / 2, w, h};
            SDL_RenderCopy(renderer, backButtonTexture, NULL, &textRect);
        }
    }
    else if (gameState == SETTINGS) {
        if (settingsTitleTexture) {
            int w, h;
            SDL_QueryTexture(settingsTitleTexture, NULL, NULL, &w, &h);
            SDL_Rect titleRect = {400 - w / 2, 100, w, h};
            SDL_RenderCopy(renderer, settingsTitleTexture, NULL, &titleRect);
        }
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &volumeSlider);

        SDL_SetRenderDrawColor(renderer, isDraggingKnob ? 255 : 200, isDraggingKnob ? 255 : 200, isDraggingKnob ? 0 : 255, 255);
        SDL_RenderFillRect(renderer, &volumeKnob);

        if (volumeTexture) {
            int w, h;
            SDL_QueryTexture(volumeTexture, NULL, NULL, &w, &h);
            SDL_Rect volumeRect = {400 - w / 2, volumeSlider.y - h - 20, w, h};
            SDL_RenderCopy(renderer, volumeTexture, NULL, &volumeRect);
        }
        if (backButtonTexture) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &backButton);
            int w, h;
            SDL_QueryTexture(backButtonTexture, NULL, NULL, &w, &h);
            SDL_Rect textRect = {backButton.x + (backButton.w - w) / 2, backButton.y + (backButton.h - h) / 2, w, h};
            SDL_RenderCopy(renderer, backButtonTexture, NULL, &textRect);
        }
    }

    SDL_RenderPresent(renderer);
}
