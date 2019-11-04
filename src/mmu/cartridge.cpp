#include "cartridge.h"

#include "mappers/mappers.h"

#include <fstream>
#include <iterator>

const char ines_constant[] = { 0x4e, 0x45, 0x53, 0x1a };

Cartridge::Cartridge(const char *cartridge_path) :
cart_info()
{
    load_file(cartridge_path);
    parse_rom();
    print_rom_info();
    init_mapper();
};

Cartridge::~Cartridge()
= default;

void Cartridge::load_file(const char *path)
{
    printf("[Cartridge] Loading file \"%s\"...\n", path);

    std::ifstream file(path, std::ios::binary);
    std::streampos file_size;

    if (!file.is_open())
    {
        throw std::runtime_error("[Cartridge] Couldn't open file!");
    }

    file.unsetf(std::ios::skipws);
    file.seekg(0, std::ios::end);

    file_size = file.tellg();

    file.seekg(0, std::ios::beg);
    cart_data.resize(file_size);
    cart_data.insert(cart_data.begin(),
            std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>());

    printf("[Cartridge] Successfully loaded \"%s\"!\n", path);
}

void Cartridge::parse_rom()
{
    if (cart_data[0] != ines_constant[0] || cart_data[1] != ines_constant[1] ||
            cart_data[2] != ines_constant[2] || cart_data[3] != ines_constant[3])
    {
        throw std::runtime_error("[Cartridge] ROM not in iNES format!");
    }

    cart_info.chr_banks = cart_data[5];
    cart_info.prg_banks = cart_data[4];
    cart_info.mapper_number = (cart_data[7] & 0xf0u) | (cart_data[6] & 0xf0u) >> 4u;
}

void Cartridge::print_rom_info() const
{
    printf("[Cartridge] CHR-ROM size: %2u KiB\n", cart_info.chr_banks * 8);
    printf("[Cartridge] PRG-ROM size: %2u KiB\n", cart_info.prg_banks * 16);
    printf("[Cartridge] Mapper:       %03u\n", cart_info.mapper_number);
    printf("------------------------------------------------\n");
}

void Cartridge::init_mapper()
{
    switch (cart_info.mapper_number)
    {
        case 0:
            mapper = std::make_unique<NROM>(cart_data, cart_info.chr_banks, cart_info.prg_banks);
            break;
        case 7:
            mapper = std::make_unique<AxROM>(cart_data);
            break;
        default:
            throw std::runtime_error("[Cartridge] Unsupported mapper!");
    }
}