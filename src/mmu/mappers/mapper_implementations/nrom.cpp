#include "nrom.h"

NROM::NROM(const std::vector<uint8_t> &cart_data, const uint8_t chr_banks, const uint8_t prg_banks) :
chr_banks(chr_banks), prg_banks(prg_banks)
{
    this->cart_data = cart_data;

    if (chr_banks == 0)
    {
        chr_ram.resize(0x2000);
    }
}

NROM::~NROM()
= default;

uint8_t NROM::read_byte(const uint16_t address) const
{
    if (prg_banks == 1)
    {
        return cart_data[0x10u + (address % 0x4000u)];
    }

    return cart_data[0x10u + (address - 0x8000)];
}

uint8_t NROM::read_chr(const uint16_t address) const
{
    if (chr_banks == 0)
    {
        return chr_ram[address];
    }

    if (prg_banks == 1)
    {
        return cart_data[0x4010u + address];
    }

    return cart_data[0x8010u + address];
}

uint16_t NROM::get_nt_addr(const uint16_t address) const
{
    return address - 0x2000u;
}

void NROM::write_byte(const uint8_t byte, const uint16_t address)
{

}

void NROM::write_chr(uint8_t byte, uint16_t address)
{
    if (chr_banks == 0)
    {
        chr_ram[address] = byte;
    }
}