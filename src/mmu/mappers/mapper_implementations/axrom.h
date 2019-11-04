#pragma once
#ifndef CIEL_AXROM_H
#define CIEL_AXROM_H


#include "..//mapper_interface/mapper.h"

#include <vector>

class AxROM : public Mapper
{
private:
    std::vector<uint8_t> cart_data;
    std::vector<uint8_t> chr_ram;
    uint8_t bank_select;
public:
    explicit AxROM(const std::vector<uint8_t> &cart_data);
    ~AxROM();

    [[nodiscard]] uint8_t read_byte(uint16_t address) const override;
    [[nodiscard]] uint8_t read_chr(uint16_t address) const override;
    [[nodiscard]] uint16_t get_nt_addr(uint16_t address) const override;
    void write_byte(uint8_t byte, uint16_t address) override;
    void write_chr(uint8_t byte, uint16_t address) override;
};


#endif //CIEL_AXROM_H
