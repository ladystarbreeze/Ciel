#pragma once
#ifndef CIEL_MAPPER_H
#define CIEL_MAPPER_H


#include <cinttypes>

class Mapper
{
private:
public:
    [[nodiscard]] virtual uint8_t read_byte(uint16_t address) const = 0;
    [[nodiscard]] virtual uint8_t read_chr(uint16_t address) const = 0;
    [[nodiscard]] virtual uint16_t get_nt_addr(uint16_t address) const = 0;
    virtual void write_byte(uint8_t byte, uint16_t address) = 0;
    virtual void write_chr(uint8_t byte, uint16_t address) = 0;
};


#endif //CIEL_MAPPER_H
