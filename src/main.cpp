#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <filesystem>

const int WINDOW_WIDTH = 300;
const int WINDOW_HEIGHT = 400;

// Function to change the CPU governor
void changeCpuGovernor(const std::string& governor) {
    std::string command = "echo '" + governor + "' | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor";
    system(command.c_str());
}

// Function to get the current CPU governor
std::string getCurrentCpuGovernor() {
    std::ifstream governorFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    std::string governor;
    if (governorFile.is_open()) {
        governorFile >> governor;
        governorFile.close();
    } else {
        std::cerr << "Failed to open governor file." << std::endl;
    }
    return governor;
}

// Function to load font with fallback
TTF_Font* loadFont(const std::string& fontName, int fontSize) {
    // Check current directory first
    if (std::filesystem::exists(fontName)) {
        return TTF_OpenFont(fontName.c_str(), fontSize);
    }
    std::string fallbackPath = "/usr/share/fonts/" + fontName;
    return TTF_OpenFont(fallbackPath.c_str(), fontSize);
}

void drawRoundedButton(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, int width, int height, SDL_Color bgColor, SDL_Color textColor, SDL_Color borderColor) {
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_Rect buttonRect = { x, y, width, height };
    SDL_RenderFillRect(renderer, &buttonRect);

    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &buttonRect);

    SDL_Surface* surface = TTF_RenderText_Solid(font, text, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect textRect = { x + (width - surface->w) / 2, y + (height - surface->h) / 2, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void drawText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color textColor) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect textRect = { x, y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0) {
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Kernel Drive", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = loadFont("ob.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;
    std::string currentGovernor = getCurrentCpuGovernor();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;

                if (mouseX >= 50 && mouseX <= 250 && mouseY >= 50 && mouseY <= 110) {
                    std::cout << "Battery mode selected!" << std::endl;
                    changeCpuGovernor("powersave");
                    currentGovernor = getCurrentCpuGovernor();
                } else if (mouseX >= 50 && mouseX <= 250 && mouseY >= 130 && mouseY <= 190) {
                    std::cout << "Performance mode selected!" << std::endl;
                    changeCpuGovernor("performance");
                    currentGovernor = getCurrentCpuGovernor();
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);

        SDL_Color batteryButtonColor = { 0, 150, 136, 255 };
        SDL_Color performanceButtonColor = { 255, 87, 34, 255 };
        SDL_Color textColor = { 255, 255, 255, 255 };
        SDL_Color borderColor = { 0, 0, 0, 255 };
        SDL_Color labelColor = { 0, 0, 0, 255 };

        drawText(renderer, font, "MODE", 50, 20, labelColor);
        drawRoundedButton(renderer, font, "BATTERY", 50, 50, 200, 60, batteryButtonColor, textColor, borderColor);
        drawRoundedButton(renderer, font, "POWER", 50, 130, 200, 60, performanceButtonColor, textColor, borderColor);
        drawText(renderer, font, "STATUS", 50, 260, labelColor);

        std::string governorText = currentGovernor;
        drawRoundedButton(renderer, font, governorText.c_str(), 50, 290, 200, 50, { 100, 100, 100, 255 }, { 255, 255, 255, 255 }, { 0, 0, 0, 255 });

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
