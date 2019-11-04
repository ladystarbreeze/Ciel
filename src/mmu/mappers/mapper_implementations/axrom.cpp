#include "axrom.h"

AxROM::AxROM(const std::vector<uint8_t> &cart_data) :
bank_select(0)
{
    this->cart_data = cart_data;

    chr_ram.resize(0x2000);
}

AxROM::~AxROM()
= default;

uint8_t AxROM::read_byte(const uint16_t address) const
{
    return cart_data[0x10u + (address - 0x8000u) + (0x8000u * (bank_select & 0x7u))];
}

uint8_t AxROM::read_chr(const uint16_t address) const
{
    return chr_ram[address];
}

uint16_t AxROM::get_nt_addr(const uint16_t address) const
{
    return (address % 0x400) + (0x400 * ((bank_select & 0x10u) != 0));
}

void AxROM::write_byte(const uint8_t byte, const uint16_t address)
{
    bank_select = byte;
}

void AxROM::write_chr(const uint8_t byte, const uint16_t address)
{
    chr_ram[address] = byte;
}