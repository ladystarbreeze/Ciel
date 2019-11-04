#include "ppu.h"

#include "..//mmu/mmu.h"

const uint8_t palette_data[64][3] = {
        // 0h, ...
        { 0x66, 0x66, 0x66 }, { 0x00, 0x2A, 0x88 }, { 0x14, 0x12, 0xA7 }, { 0x3B, 0x00, 0xA4 },
        { 0x5C, 0x00, 0x7E }, { 0x6E, 0x00, 0x40 }, { 0x6C, 0x06, 0x00 }, { 0x56, 0x1D, 0x00 },
        { 0x33, 0x35, 0x00 }, { 0x0B, 0x48, 0x00 }, { 0x00, 0x52, 0x00 }, { 0x00, 0x4F, 0x08 },
        { 0x00, 0x40, 0x4D }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },
        // 10h, ...
        { 0xAD, 0xAD, 0xAD }, { 0x15, 0x5F, 0xD9 }, { 0x42, 0x40, 0xFF }, { 0x75, 0x27, 0xFE },
        { 0xA0, 0x1A, 0xCC }, { 0xB7, 0x1E, 0x7B }, { 0xB5, 0x31, 0x20 }, { 0x99, 0x4E, 0x00 },
        { 0x6B, 0x6D, 0x00 }, { 0x38, 0x87, 0x00 }, { 0x0C, 0x93, 0x00 }, { 0x00, 0x8F, 0x32 },
        { 0x00, 0x7C, 0x8D }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },
        // 20h, ...
        { 0xFF, 0xFE, 0xFF }, { 0x64, 0xB0, 0xFF }, { 0x92, 0x90, 0xFF }, { 0xC6, 0x76, 0xFF },
        { 0xF3, 0x6A, 0xFF }, { 0xFE, 0x6E, 0xCC }, { 0xFE, 0x81, 0x70 }, { 0xEA, 0x9E, 0x22 },
        { 0xBC, 0xBE, 0x00 }, { 0x88, 0xD8, 0x00 }, { 0x5C, 0xE4, 0x30 }, { 0x45, 0xE0, 0x82 },
        { 0x48, 0xCD, 0xDE }, { 0x4F, 0x4F, 0x4F }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },
        // 30h, ...
        { 0xFF, 0xFE, 0xFF }, { 0xC0, 0xDF, 0xFF }, { 0xD3, 0xD2, 0xFF }, { 0xE8, 0xC8, 0xFF },
        { 0xFB, 0xC2, 0xFF }, { 0xFE, 0xC4, 0xEA }, { 0xFE, 0xCC, 0xC5 }, { 0xF7, 0xD8, 0xA5 },
        { 0xE4, 0xE5, 0x94 }, { 0xCF, 0xEF, 0x96 }, { 0xBD, 0xF4, 0xAB }, { 0xB3, 0xF3, 0xCC },
        { 0xB5, 0xEB, 0xF2 }, { 0xB8, 0xB8, 0xB8 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }
};

PPU::PPU(const std::shared_ptr<MMU> &mmu) :
regs(), s_regs(), bg(), spr(), internal_bus(0), ppu_cycle(0), scanline(0), first_write(true), suppress_vblank_flag(false),
even_frame(true)
{
    this->mmu = mmu;

    oam.resize(0x100);
    oam_2.resize(0x20);
    vram.resize(0x2000);
    framebuffer.resize(3 * 256 * 240);
}

PPU::~PPU()
= default;

void PPU::tick()
{
    ++ppu_cycle;

    if (ppu_cycle == 341)
    {
        if (scanline == 261 && (regs.ppumask & 0x8u) && !even_frame)
        {
            ppu_cycle = 1;
        }
        else
        {
            ppu_cycle = 0;
        }

        ++scanline;
    }

    if (scanline == 262)
    {
        scanline = 0;
        even_frame = !even_frame;
    }
}

void PPU::nmi_evaluation()
{
    if ((regs.ppustatus & 0x80u) && (regs.ppuctrl & 0x80u))
    {
        // printf("[PPU] NMI requested.\n");

        regs.ppuctrl &= ~(0x80u);
        mmu->nmi_pending = true;
    }
}

void PPU::v_increment()
{
    (regs.ppuctrl & 0x4u) ? s_regs.v += 32 : ++s_regs.v;
}

void PPU::x_increment()
{
    if ((s_regs.v & 0x1fu) == 31)
    {
        s_regs.v &= 0xffe0u;
        s_regs.v ^= 0x400u;
    }
    else
    {
        ++s_regs.v;
    }
}

void PPU::y_increment()
{
    static uint8_t coarse_y;

    if ((s_regs.v & 0x7000u) != 0x7000)
    {
        s_regs.v += 0x1000u;
    }
    else
    {
        s_regs.v &= ~(0x7000u);
        coarse_y = (s_regs.v & 0x3e0u) >> 5u;

        if (coarse_y == 29)
        {
            coarse_y = 0;
            s_regs.v ^= 0x800u;
        }
        else if (coarse_y == 31)
        {
            coarse_y = 0;
        }
        else
        {
            ++coarse_y;
        }

        s_regs.v = (s_regs.v & ~(0x3e0u)) | (uint16_t)(coarse_y << 5u);
    }
}

bool PPU::is_rendering() const
{
    return regs.ppumask & 0x18u;
}

uint8_t PPU::read_memory(const uint16_t address)
{
    if (address < 0x2000)
    {
        return mmu->read_chr(address);
    }
    else if (address >= 0x2000 && address < 0x3f00)
    {
        return vram[mmu->get_nt_addr(address)];
    }

    return vram[address - 0x2000u];
}

void PPU::write_memory(const uint8_t byte)
{
    if (s_regs.v < 0x2000)
    {
        mmu->write_chr(byte, s_regs.v);
    }
    else
    {
        if (s_regs.v == 0x3f10 || s_regs.v == 0x3f14 || s_regs.v == 0x3f18 || s_regs.v == 0x3f1c)
        {
            vram[(s_regs.v - 0x2000u) & 0xff0fu] = byte;
        }
        else
        {
            vram[s_regs.v - 0x2000u] = byte;
        }
    }
}

void PPU::background_fetch()
{
    if (ppu_cycle == 0)
    {

    }
    else if ((ppu_cycle >= 1 && ppu_cycle < 257) || (ppu_cycle >= 321 && ppu_cycle < 337))
    {
        switch (ppu_cycle % 8)
        {
            case 0:
                bg.tile_address = ((uint16_t)(regs.ppuctrl >> 4u) & 1u) * 0x1000u +
                                  bg.nametable_byte * 16u + ((uint16_t)(s_regs.v >> 12u) & 0x7u);
                bg.tile_high = read_memory(bg.tile_address + 8);

                if (is_rendering())
                {
                    x_increment();
                }
                break;
            case 1:
                bg.tile_shifter[0] &= 0xff00u;
                bg.tile_shifter[0] |= bg.tile_low;
                bg.tile_shifter[1] &= 0xff00u;
                bg.tile_shifter[1] |= bg.tile_high;
                bg.at_latch[0] = bg.attribute_byte & 0x1u;
                bg.at_latch[1] = bg.attribute_byte & 0x2u;
                break;
            case 2:
                bg.nametable_address = 0x2000u | (s_regs.v & 0xfffu);
                bg.nametable_byte = read_memory(bg.nametable_address);
                break;
            case 4:
                bg.attribute_address = 0x23c0u | ((s_regs.v) & 0xc00u) |
                                       ((uint16_t)(s_regs.v >> 4u) & 0x38u) | ((uint16_t)(s_regs.v >> 2u) & 0x7u);
                bg.attribute_byte = read_memory(bg.attribute_address);

                if ((uint8_t)(s_regs.v >> 5u) & 2u)
                {
                    bg.attribute_byte >>= 4u;
                }
                if (s_regs.v & 2u)
                {
                    bg.attribute_byte >>= 2u;
                }
                break;
            case 6:
                bg.tile_address = ((uint16_t)(regs.ppuctrl >> 4u) & 1u) * 0x1000u +
                                  bg.nametable_byte * 16u + ((uint16_t)(s_regs.v >> 12u) & 0x7u);
                bg.tile_low = read_memory(bg.tile_address);
                break;
        }
    }
    else if (ppu_cycle >= 337 && ppu_cycle < 341)
    {
        if (ppu_cycle % 2 == 0)
        {
            bg.nametable_byte = read_memory(0x2000u | (s_regs.v & 0xfffu));
        }
    }

    if (ppu_cycle == 256 && is_rendering())
    {
        y_increment();
    }

    if (is_rendering())
    {
        if (ppu_cycle == 257)
        {
            s_regs.v = (s_regs.v & 0xfbe0u) | (s_regs.t & 0x41fu);
        }
        if (scanline == 261 && (ppu_cycle >= 280 && ppu_cycle < 305))
        {
            s_regs.v = (s_regs.v & 0x841fu) | (s_regs.t & 0x7be0u);
        }
    }
}

void PPU::sprite_fetch()
{
    if (ppu_cycle == 0)
    {
        for (uint8_t addr = 0; addr < 32; addr++)
        {
            oam_2[addr] = 0xff;
        }

        spr.sprite_zero_on_line = false;

        uint8_t oam_2_addr = 0;

        for (uint8_t sprite = 0; sprite < 64; sprite++)
        {
            uint8_t y_pos = oam[sprite * 4];
            bool on_this_line = in_range(int(y_pos), int(scanline) - (((regs.ppustatus & 0x20u) != 0) ? 16 : 8), int(scanline) - 1);

            if (!on_this_line)
            {
                continue;
            }

            if (sprite == 0)
            {
                spr.sprite_zero_on_line = true;
            }

            if (oam_2_addr < 32)
            {
                oam_2[oam_2_addr++] = oam[sprite * 4];
                oam_2[oam_2_addr++] = oam[sprite * 4 + 1];
                oam_2[oam_2_addr++] = oam[sprite * 4 + 2];
                oam_2[oam_2_addr++] = oam[sprite * 4 + 3];
            }
            else
            {
                if (is_rendering())
                {
                    regs.ppustatus |= 0x20u;
                }
            }
        }
    }

    if (ppu_cycle >= 257 && ppu_cycle < 321)
    {
        if (ppu_cycle % 8 == 1 || ppu_cycle % 8 == 3)
        {
            return;
        }

        const uint16_t sprite = (ppu_cycle - 257) / 8;

        uint8_t y_pos = oam_2[sprite * 4];
        uint8_t tile_index = oam_2[sprite * 4 + 1];
        uint8_t attribute = oam_2[sprite * 4 + 2];
        uint8_t x_pos = oam_2[sprite * 4 + 3];

        const bool is_dummy = (y_pos == 0xff && x_pos == 0xff && tile_index == 0xff && attribute == 0xff);
        uint16_t spr_row = scanline - y_pos - 1;
        const uint8_t spr_height = ((regs.ppuctrl & 0x20u) != 0) ? 16 : 8;

        if ((attribute & 0x80u) != 0)
        {
            spr_row = spr_height - 1 - spr_row;
        }

        const bool spr_table = ((regs.ppuctrl & 0x20u) == 0) ? ((regs.ppuctrl & 0x8u) != 0) : tile_index & 1u;

        if ((regs.ppuctrl & 0x20u) != 0)
        {
            tile_index &= 0xfeu;

            if (spr_row > 7)
            {
                ++tile_index;
                spr_row -= 8;
            }
        }

        if (is_dummy)
        {
            spr_row = 0;
        }

        const uint16_t tile_addr = (0x1000 * spr_table) + (tile_index * 16) + spr_row;

        if (ppu_cycle % 8 == 5)
        {
            spr.sliver[sprite].lo = read_memory(tile_addr);
        }
        if (ppu_cycle % 8 == 7)
        {
            spr.sliver[sprite].lo = read_memory(tile_addr + 8);
        }
    }
}

Pixel PPU::background_pixel()
{
    static uint8_t palette;
    static uint8_t type;

    palette = (((uint8_t)(bg.at_shifter[1] >> (7u - s_regs.x)) & 1u) << 1u) |
              (((uint8_t)(bg.at_shifter[0] >> (7u - s_regs.x)) & 1u));
    type = (((uint8_t)(bg.tile_shifter[1] >> (15u - s_regs.x)) & 1u) << 1u) |
           (((uint8_t)(bg.tile_shifter[0] >> (15u - s_regs.x)) & 1u));

    if ((ppu_cycle >= 1 && ppu_cycle < 257) || (ppu_cycle >= 321 && ppu_cycle < 337))
    {
        bg.tile_shifter[0] <<= 1u;
        bg.tile_shifter[1] <<= 1u;
        bg.at_shifter[0] <<= 1u;
        bg.at_shifter[0] |= (uint8_t)bg.at_latch[0];
        bg.at_shifter[1] <<= 1u;
        bg.at_shifter[1] |= (uint8_t)bg.at_latch[1];
    }

    if (((regs.ppumask & 0x2u) == 0) && ppu_cycle < 8)
    {
        return Pixel(false, 0, false);
    }

    if (((regs.ppumask & 0x8u) == 0) || scanline >= 240)
    {
        return Pixel(false, 0, false);
    }

    return Pixel(type, read_memory(0x3f00u + palette * 4u + type), false);
}

Pixel PPU::sprite_pixel(Pixel &bg_pixel)
{
    const int x = ppu_cycle;

    if (((regs.ppumask & 0x10u) == 0) || (((regs.ppumask & 0x4u) == 0) && x < 8))
    {
        return Pixel(false, 0, false);
    }

    for (uint8_t sprite = 0; sprite < 8; sprite++)
    {
        uint8_t y_pos = oam_2[sprite * 4];
        uint8_t tile_index = oam_2[sprite * 4 + 1];
        uint8_t attribute = oam_2[sprite * 4 + 2];
        uint8_t x_pos = oam_2[sprite * 4 + 3];

        if (y_pos == 0xff && x_pos == 0xff && tile_index == 0xff && attribute == 0xff)
        {
            break;
        }

        bool x_in_range = in_range(int(x_pos), int(x) - 7, int(x));

        if (!x_in_range)
        {
            continue;
        }

        uint16_t spr_row = scanline - y_pos - 1;
        uint8_t spr_col = 7 - (x - x_pos);

        const uint8_t sprite_height = ((regs.ppuctrl & 0x20u) != 0) ? 16 : 8;

        if ((attribute & 0x80u) != 0)
        {
            spr_row = sprite_height - 1 - spr_row;
        }

        if ((attribute & 0x40u) != 0)
        {
            spr_col = 8 - 1 - spr_col;
        }

        bool spr_table = ((regs.ppuctrl & 0x20u) == 0) ? ((regs.ppuctrl & 0x8u) != 0) : tile_index & 1u;

        if ((regs.ppuctrl & 0x20u) != 0)
        {
            tile_index &= 0xfeu;

            if (spr_row > 7)
            {
                ++tile_index;
                spr_row -= 8;
            }
        }

        uint16_t tile_addr = (0x1000 * spr_table) + (tile_index * 16) + spr_row;
        uint8_t lo_bp = read_memory(tile_addr);
        uint8_t hi_bp = read_memory(tile_addr + 8);
        uint8_t type = nth_bit(lo_bp, spr_col) + (nth_bit(hi_bp, spr_col) << 1u);

        if (type == 0)
        {
            continue;
        }

        if (sprite == 0 && spr.sprite_zero_on_line && is_rendering() && ((regs.ppustatus & 0x40u) == 0) &&
                (x_pos != 0xff && x < 0xff) && bg_pixel.is_on)
        {
            regs.ppustatus |= 0x40u;
        }

        return Pixel(true, read_memory(0x3f10 + (attribute & 0x7u) * 4 + type), (attribute & 0x20u) != 0);
    }

    return Pixel(false, 0, false);
}

void PPU::draw_pixel(uint8_t color, uint64_t offset)
{
    framebuffer[offset] = palette_data[color][0];
    framebuffer[offset + 1u] = palette_data[color][1];
    framebuffer[offset + 2u] = palette_data[color][2];
}

uint8_t PPU::read_ppustatus()
{
    static uint8_t old_ppustatus;

    old_ppustatus = (regs.ppustatus & 0xe0u) | (internal_bus & 0x1fu);
    regs.ppustatus &= ~(0x80u);
    first_write = true;

    if (scanline == 241)
    {
        switch (ppu_cycle)
        {
            case 0:
                old_ppustatus &= ~(0x80u);
                suppress_vblank_flag = true;
                break;
            case 1:
            case 2:
                suppress_vblank_flag = true;
                break;
        }
    }

    internal_bus = old_ppustatus;

    return old_ppustatus;
}

uint8_t PPU::read_ppudata()
{
    static uint8_t buffer;

    if (s_regs.v < 0x3f00)
    {
        buffer = regs.ppudata;
        regs.ppudata = read_memory(s_regs.v);
    }
    else
    {
        buffer = read_memory(s_regs.v);
        regs.ppudata = buffer;
    }

    v_increment();

    return buffer;
}

void PPU::write_ppuctrl(const uint8_t byte)
{
    regs.ppuctrl = byte;
    s_regs.t = (s_regs.t & 0xf3ffu) | ((byte & 0x3u) << 10u);
}

void PPU::write_ppuscroll(const uint8_t byte)
{
    if (first_write)
    {
        s_regs.t = (s_regs.t & 0xffe0u) | (uint16_t)(byte  >> 3u);
        s_regs.x = (byte & 0x7u);
    }
    else
    {
        s_regs.t = (s_regs.t & 0x8fffu) | ((byte & 0x7u) << 12u);
        s_regs.t = (s_regs.t & 0xfc1fu) | ((byte & 0xf8u) << 2u);
    }

    first_write = !first_write;
}

void PPU::write_ppuaddr(const uint8_t byte)
{
    if (first_write)
    {
        s_regs.t = (s_regs.t & 0x80ffu) | ((byte & 0x3fu) << 8u);
    }
    else
    {
        s_regs.t = (s_regs.t & 0xff00u) | byte;
        s_regs.v = s_regs.t;
    }

    first_write = !first_write;
}

void PPU::write_ppudata(const uint8_t byte)
{
    regs.ppudata = byte;

    write_memory(byte);
    v_increment();
}

uint8_t PPU::read_register(const uint16_t address)
{
    switch (address)
    {
        case 0:
        case 1:
        case 3:
        case 5:
        case 6:
            printf("[PPU] Read from write-only register\n");
            return internal_bus;
        case 2:
            // happens very often
            // printf("[PPU] [Read] PPUSTATUS\n");
            return read_ppustatus();
        case 4:
            // printf("[PPU] [Read] OAMDATA");
            return oam[regs.oamaddr];
        case 7:
            // printf("[PPU] [Read] PPUDATA\n");
            return read_ppudata();
        default:
            return 0;
    }
}

void PPU::write_register(const uint8_t byte, const uint16_t address)
{
    switch (address)
    {
        case 0:
            // printf("[PPU] [Write] PPUCTRL = %02Xh\n", byte);
            write_ppuctrl(byte);
            break;
        case 1:
            // printf("[PPU] [Write] PPUMASK = %02Xh\n", byte);

            regs.ppumask = byte;
            break;
        case 2:
            // printf("[PPU] [Write] Invalid write to PPUSTATUS!\n");
            break;
        case 3:
            // printf("[PPU] [Write] OAMADDR = %02Xh\n", byte);

            regs.oamaddr = byte;
            break;
        case 4:
            // printf("[PPU] OAMDATA = %02Xh\n", byte);

            oam[regs.oamaddr++] = byte;
            break;
        case 5:
            // printf("[PPU] [Write] PPUSCROLL = %02Xh\n", byte);
            write_ppuscroll(byte);
            break;
        case 6:
            // printf("[PPU] [Write] PPUADDR = %02Xh\n", byte);
            write_ppuaddr(byte);
            break;
        case 7:
            // +printf("[PPU] [Write] PPUDATA = %02Xh\n", byte);
            write_ppudata(byte);
            break;
        default:
            printf("[PPU] Register %04Xh", address + 0x2000);
            throw std::runtime_error("[PPU] Write to invalid register!");
    }

    internal_bus = byte;
}

void PPU::run_cycle()
{
    static uint8_t color;
    static Pixel bg_pixel = Pixel(false, 0, false);
    static Pixel spr_pixel = Pixel(false, 0, false);

    if (scanline < 240 || scanline == 261)
    {
        bg_pixel = background_pixel();
        spr_pixel = sprite_pixel(bg_pixel);

        if (is_rendering())
        {
            background_fetch();
            sprite_fetch();
        }

        if (!bg_pixel.is_on && !spr_pixel.is_on)
        {
            color = read_memory(0x3f00);
        }
        else if (!bg_pixel.is_on && spr_pixel.is_on)
        {
            color = spr_pixel.color;
        }
        else if (bg_pixel.is_on && !spr_pixel.is_on)
        {
            color = bg_pixel.color;
        }
        else
        {
            color = (spr_pixel.priority) ? bg_pixel.color : spr_pixel.color;
        }

        if (ppu_cycle < 256 && scanline != 261)
        {
            draw_pixel(color, (256 * 3 * scanline) + (3 * ppu_cycle));
        }

        if (ppu_cycle >= 257 && ppu_cycle < 321)
        {
            regs.oamaddr = 0;
        }

        if (scanline == 261)
        {
            if (ppu_cycle == 1)
            {
                regs.ppustatus &= 0x1fu;
                mmu->nmi_pending = false;
            }

        }
    }
    else if (scanline == 241)
    {
        if (ppu_cycle == 1)
        {
            mmu->update_framebuffer(framebuffer.data());

            if (!suppress_vblank_flag)
            {
                regs.ppustatus |= 0x80u;
            }
            else
            {
                suppress_vblank_flag = false;
            }

            mmu->vblank = true;
        }
    }

    nmi_evaluation();
    tick();
}