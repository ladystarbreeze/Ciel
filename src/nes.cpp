#include "nes.h"

#include "cpu/cpu.h"
#include "mmu/mmu.h"
#include "ppu/ppu.h"

NES::NES(const char *cartridge_path) :
renderer(nullptr), window(nullptr), texture(nullptr), event(), joy(0), strobe(0)
{
    printf("------------------------------------------------\n");
    printf("----------- Ciel NES Emulator v0.1.0 -----------\n");
    printf("------------------------------------------------\n");

    mmu = std::make_shared<MMU>(nullptr, this, cartridge_path);
    ppu = std::make_shared<PPU>(mmu);
    cpu = std::make_unique<CPU>(mmu);

    mmu->set_ppu(ppu);

    init_sdl();
}

NES::~NES()
= default;

void NES::init_sdl()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_CreateWindowAndRenderer(256, 240, 0, &window, &renderer);
    SDL_SetWindowSize(window, 512, 480);
    SDL_RenderSetLogicalSize(renderer, 512, 480);
    SDL_SetWindowResizable(window, SDL_FALSE);
    SDL_SetWindowTitle(window, "Ciel NES emulator v0.1.0");

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 240);
}

void NES::update_framebuffer(const uint8_t *framebuffer)
{
    SDL_UpdateTexture(texture, nullptr, framebuffer, 256 * sizeof(uint8_t) * 3);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void NES::strobe_joypad()
{
    //printf("----------------------------------------\n");
    const uint8_t *keyboard_state = SDL_GetKeyboardState(nullptr);

    SDL_PollEvent(&event);

    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
    {
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_x)])
        {
            joy |= 0x80u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_y)])
        {
            joy |= 0x40u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_BACKSPACE)])
        {
            joy |= 0x20u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_KP_ENTER)])
        {
            joy |= 0x10u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_UP)])
        {
            joy |= 0x8u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_DOWN)])
        {
            joy |= 0x4u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_LEFT)])
        {
            joy |= 0x2u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_RIGHT)])
        {
            joy |= 0x1u;
        }
    }
}

uint8_t NES::get_key()
{
    uint8_t key = (joy & 0x80u) != 0;

    joy <<= 1u;

    //printf("BLARG\n");

    return key | 0x40u;
}

void NES::run()
{
    while (cpu->is_running)
    {
        try
        {
            ppu->run_cycle();
            cpu->run_cycle();
            ppu->run_cycle();
            ppu->run_cycle();

            if (strobe != 0)
            {
                strobe_joypad();
            }
        }
        catch (const std::runtime_error& error)
        {
            printf("\n[Ciel] Runtime error!\n");
            printf("%s\n", error.what());

            cpu->is_running = false;
        }
    }
}