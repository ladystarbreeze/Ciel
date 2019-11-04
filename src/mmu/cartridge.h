#pragma once
#ifndef CIEL_CARTRIDGE_H
#define CIEL_CARTRIDGE_H


#include <cinttypes>
#include <memory>
#include <vector>

struct Cartridge_Information
{
    uint8_t chr_banks;
    uint8_t prg_banks;
    uint16_t mapper_number;
};

class Mapper;

class Cartridge
{
private:
    Cartridge_Information cart_info;
    std::vector<uint8_t> cart_data;

    void load_file(const char *path);
    void parse_rom();
    void print_rom_info() const;
    void init_mapper();
public:
    explicit Cartridge(const char *cartridge_path);
    ~Cartridge();

    std::unique_ptr<Mapper> mapper;
};


#endif //CIEL_CARTRIDGE_H
