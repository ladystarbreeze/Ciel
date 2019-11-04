#pragma once
#ifndef CIEL_NES_H
#define CIEL_NES_H


#include <memory>

#include "SDL2/SDL.h"

class CPU;
class MMU;
class PPU;

class NES
{
private:
    std::shared_ptr<MMU> mmu;
    std::shared_ptr<PPU> ppu;
    std::unique_ptr<CPU> cpu;

    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Texture *texture;
    SDL_Event event;
public:
    explicit NES(const char *cartridge_path);
    ~NES();

    uint8_t joy;
    uint8_t strobe;

    void init_sdl();
    void update_framebuffer(const uint8_t *framebuffer);

    void strobe_joypad();
    uint8_t get_key();

    void run();
};


#endif //CIEL_NES_H
