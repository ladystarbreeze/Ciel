#pragma once
#ifndef CIEL_PPU_H
#define CIEL_PPU_H


#include <memory>
#include <vector>

struct PPU_Registers
{
    uint8_t ppuctrl;
    uint8_t ppumask;
    uint8_t ppustatus;
    uint8_t oamaddr;
    uint8_t oamdata;
    uint8_t ppuscroll;
    uint8_t ppuaddr;
    uint8_t ppudata;
    uint8_t oamdma;
};

struct PPU_Scroll_Registers
{
    uint8_t x;
    uint16_t v;
    uint16_t t;
};

struct Background
{
    bool at_latch[2];
    uint8_t tile_high;
    uint8_t tile_low;
    uint8_t at_shifter[2];
    uint16_t tile_shifter[2];
    uint16_t attribute_address;
    uint16_t attribute_byte;
    uint16_t nametable_address;
    uint16_t nametable_byte;
    uint16_t tile_address;
};

struct Sprite
{
    struct
    {
        uint8_t hi;
        uint8_t lo;
    } sliver[8];

    uint8_t at_byte[8];
    uint8_t x_pos[8];

    bool sprite_zero_on_line;
};

struct Pixel
{
    Pixel(bool is_on, uint8_t color, bool priority) :
    is_on(is_on), color(color), priority(priority)
    {

    }

    bool is_on;
    uint8_t color;
    bool priority;
};

template <typename T>
constexpr inline bool nth_bit(T x, uint8_t n) { return (x >> n) & 1; }

template <typename T, typename T2>
constexpr inline bool in_range(T x, T2 min, T2 max) { return x >= T(min) && x <= T(max); }

template <typename T, typename T2>
constexpr inline bool in_range(T x, T2 val) { return x == val; }

class MMU;

class PPU
{
private:
    Background bg;
    Sprite spr;
    PPU_Registers regs;
    PPU_Scroll_Registers s_regs;

    std::shared_ptr<MMU> mmu;
    std::vector<uint8_t> oam;
    std::vector<uint8_t> oam_2;
    std::vector<uint8_t> vram;

    uint8_t internal_bus;
    uint16_t ppu_cycle;
    uint16_t scanline;

    bool first_write;
    bool suppress_vblank_flag;
    bool even_frame;

    inline void tick();
    inline void nmi_evaluation();

    inline void v_increment();
    inline void x_increment();
    inline void y_increment();

    [[nodiscard]] inline bool is_rendering() const;

    inline uint8_t read_memory(uint16_t address);
    inline void write_memory(uint8_t byte);

    void background_fetch();
    void sprite_fetch();
    Pixel background_pixel();
    Pixel sprite_pixel(Pixel &bg_pixel);

    inline void draw_pixel(uint8_t color, uint64_t offset);

    inline uint8_t read_ppustatus();
    inline uint8_t read_ppudata();
    inline void write_ppuctrl(uint8_t byte);
    inline void write_ppuscroll(uint8_t byte);
    inline void write_ppuaddr(uint8_t byte);
    inline void write_ppudata(uint8_t byte);
public:
    explicit PPU(const std::shared_ptr<MMU> &mmu);
    ~PPU();

    std::vector<uint8_t> framebuffer;

    uint8_t read_register(uint16_t address);
    void write_register(uint8_t byte, uint16_t address);

    void run_cycle();
};


#endif //CIEL_PPU_H
