#include "mmu.h"

#include "cartridge.h"
#include "mappers/mappers.h"
#include "..//ppu/ppu.h"
#include "..//nes.h"

MMU::MMU(const std::shared_ptr<PPU> &ppu, NES *nes, const char *cartridge_path) :
nmi_pending(false), vblank(false), oam_dma(false), oam_hi(0)
{
    this->ppu = ppu;
    this->nes = nes;
    cart = std::make_unique<Cartridge>(cartridge_path);

    ram.resize(0x800);
}

MMU::~MMU()
= default;

void MMU::set_ppu(const std::shared_ptr<PPU> &ppu_)
{
    this->ppu = ppu_;
}

void MMU::update_framebuffer(const uint8_t *framebuffer)
{
    nes->update_framebuffer(framebuffer);
}

uint8_t MMU::read_byte(const uint16_t address)
{
    if (address < 0x2000)
    {
        return ram[address % 0x800u];
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        return ppu->read_register(address % 8);
    }
    else if (address >= 0x4000 && address < 0x4018)
    {
        switch (address)
        {
            case 0x4000:
                printf("[MMU] Read from APU Channel 1 Volume\n");
                return 0;
            case 0x4001:
                printf("[MMU] Read from APU Channel 1 Sweep\n");
                return 0;
            case 0x4002:
                printf("[MMU] Read from APU Channel 1 Frequency\n");
                return 0;
            case 0x4003:
                printf("[MMU] Read from APU Channel 1 Length\n");
                return 0;
            case 0x4004:
                printf("[MMU] Read from APU Channel 2 Volume\n");
                return 0;
            case 0x4005:
                printf("[MMU] Read from APU Channel 2 Sweep\n");
                return 0;
            case 0x4006:
                printf("[MMU] Read from APU Channel 2 Frequency\n");
                return 0;
            case 0x4007:
                printf("[MMU] Read from APU Channel 2 Length\n");
                return 0;
            case 0x4008:
                printf("[MMU] Read from APU Channel 3 Linear Counter\n");
                return 0;
            case 0x4009:
                printf("[MMU] Read from APU Channel 3 N/A\n");
                return 0;
            case 0x400a:
                printf("[MMU] Read from APU Channel 3 Frequency\n");
                return 0;
            case 0x400b:
                printf("[MMU] Read from APU Channel 3 Length\n");
                return 0;
            case 0x400c:
                printf("[MMU] Read from APU Channel 4 Volume\n");
                return 0;
            case 0x400d:
                printf("[MMU] Read from APU Channel 4 N/A\n");
                return 0;
            case 0x400e:
                printf("[MMU] Read from APU Channel 4 Frequency\n");
                return 0;
            case 0x400f:
                printf("[MMU] Read from APU Channel 4 Length\n");
                return 0;
            case 0x4010:
                printf("[MMU] Read from DMC Frequency\n");
                return 0;
            case 0x4011:
                printf("[MMU] Read from DMC Delta Counter\n");
                return 0;
            case 0x4012:
                printf("[MMU] Read from DMC Address Load\n");
                return 0;
            case 0x4013:
                printf("[MMU] Read from DMC Length\n");
                return 0;
            case 0x4014:
                printf("[MMU] Read from OAMDMA\n");
                return 0;
            case 0x4015:
                printf("[MMU] Read from DMC Length Counter\n");
                return 0;
            case 0x4016:
                // printf("[MMU] Read from Joypad #1\n");
                return nes->get_key();
            case 0x4017:
                // printf("[MMU] Read from Frame Counter\n");
                return 0x0;
            default:
                printf("[MMU] Address: %04X", address);
                throw std::runtime_error("[MMU] Unhandled IO read!");
        }
    }
    else if (address >= 0x4020 && address < 0x8000)
    {
        return 0;
    }
    else if (address >= 0x8000)
    {
        return cart->mapper->read_byte(address);
    }

    printf("[MMU] Address: %04Xh", address);
    throw std::runtime_error("[MMU] Invalid byte read!");
}

uint8_t MMU::read_chr(const uint16_t address) const
{
    return cart->mapper->read_chr(address);
}

uint16_t MMU::get_nt_addr(const uint16_t address) const
{
    return cart->mapper->get_nt_addr(address);
}

void MMU::write_byte(const uint8_t byte, const uint16_t address)
{
    if (address < 0x2000)
    {
        ram[address % 0x800u] = byte;
        return;
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        ppu->write_register(byte, address % 8);
        return;
    }
    else if (address >= 0x4000 && address < 0x4018)
    {
        switch (address)
        {
            case 0x4000:
                // printf("[MMU] APU Channel 1 Volume = %02X\n", byte);
                break;
            case 0x4001:
                // printf("[MMU] APU Channel 1 Sweep = %02X\n", byte);
                break;
            case 0x4002:
                // printf("[MMU] APU Channel 1 Frequency = %02X\n", byte);
                break;
            case 0x4003:
                // printf("[MMU] APU Channel 1 Length = %02X\n", byte);
                break;
            case 0x4004:
                // printf("[MMU] APU Channel 2 Volume = %02X\n", byte);
                break;
            case 0x4005:
                // printf("[MMU] APU Channel 2 Sweep = %02X\n", byte);
                break;
            case 0x4006:
                // printf("[MMU] APU Channel 2 Frequency = %02X\n", byte);
                break;
            case 0x4007:
                // printf("[MMU] APU Channel 2 Length = %02X\n", byte);
                break;
            case 0x4008:
                // printf("[MMU] APU Channel 3 Linear Counter = %02X\n", byte);
                break;
            case 0x4009:
                // printf("[MMU] APU Channel 3 N/A = %02X\n", byte);
                break;
            case 0x400a:
                // printf("[MMU] APU Channel 3 Frequency = %02X\n", byte);
                break;
            case 0x400b:
                // printf("[MMU] APU Channel 3 Length = %02X\n", byte);
                break;
            case 0x400c:
                // printf("[MMU] APU Channel 4 Volume = %02X\n", byte);
                break;
            case 0x400d:
                // printf("[MMU] APU Channel 4 N/A = %02X\n", byte);
                break;
            case 0x400e:
                // printf("[MMU] APU Channel 4 Frequency = %02X\n", byte);
                break;
            case 0x400f:
                // printf("[MMU] APU Channel 4 Length = %02X\n", byte);
                break;
            case 0x4010:
                // printf("[MMU] APU Channel 5 Delta Frequency = %02X\n", byte);
                break;
            case 0x4011:
                // printf("[MMU] APU Channel 5 Delta Counter = %02X\n", byte);
                break;
            case 0x4012:
                // printf("[MMU] DMC Address Load = %02X\n", byte);
                break;
            case 0x4013:
                // printf("[MMU] DMC Length = %02X\n", byte);
                break;
            case 0x4014:
                // printf("[MMU] OAMDMA = %02X\n", byte);

                oam_dma = true;
                oam_hi = byte;
                break;
            case 0x4015:
                // printf("[MMU] DMC Length Counter = %02X\n", byte);
                break;
            case 0x4016:
                // printf("[MMU] Joypad #1 = %02X\n", byte);
                nes->strobe = byte;
                break;
            case 0x4017:
                // printf("[MMU] Joypad #2 = %02X\n", byte);
                break;
            default:
                printf("[MMU] Address: %04X", address);
                throw std::runtime_error("[MMU] Unhandled IO write!");
        }

        return;
    }
    else if (address >= 0x6000 && address < 0x8000)
    {
        printf("[MMU] SRAM = %c\n", byte);
        return;
    }
    else if (address >= 0x8000)
    {
        cart->mapper->write_byte(byte, address);
        return;
    }

    printf("[MMU] Address: %04Xh, byte: %02Xh", address, byte);
    throw std::runtime_error("[MMU] Invalid byte store!");
}

void MMU::write_chr(const uint8_t byte, const uint16_t address)
{
    cart->mapper->write_chr(byte, address);
}