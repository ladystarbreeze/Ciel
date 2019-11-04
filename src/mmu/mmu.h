#pragma once
#ifndef CIEL_MMU_H
#define CIEL_MMU_H


#include <memory>
#include <vector>

class Cartridge;
class NES;
class PPU;

class MMU
{
private:
    std::shared_ptr<PPU> ppu;
    std::unique_ptr<Cartridge> cart;
    std::vector<uint8_t> ram;
    NES *nes;
public:
    MMU(const std::shared_ptr<PPU> &ppu, NES *nes, const char *cartridge_path);
    ~MMU();

    uint8_t oam_hi;

    bool nmi_pending;
    bool oam_dma;
    bool vblank;

    void set_ppu(const std::shared_ptr<PPU> &ppu_);

    void update_framebuffer(const uint8_t *framebuffer);

    [[nodiscard]] uint8_t read_byte(uint16_t address);
    [[nodiscard]] uint8_t read_chr(uint16_t address) const;
    [[nodiscard]] uint16_t get_nt_addr(uint16_t address) const;
    void write_byte(uint8_t byte, uint16_t address);
    void write_chr(uint8_t byte, uint16_t address);
};


#endif //CIEL_MMU_H
