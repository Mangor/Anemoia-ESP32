#include "ppu2C02.h"
#include "bus.h"

#define READ_PALETTE(x) palette_table[((x) & 0x1F) ^ (((x) & 0x13) == 0x10 ? 0x10 : 0x00)]
DMA_ATTR uint16_t Ppu2C02::display_buffer[SCANLINE_SIZE * SCANLINES_PER_BUFFER];

static constexpr uint16_t bitplane_lo[256] = 
{
    0x0000, 0x0001, 0x0100, 0x0101, 0x0004, 0x0005, 0x0104, 0x0105, 0x0400, 0x0401, 0x0500, 0x0501, 0x0404, 0x0405, 0x0504, 0x0505, 0x0010, 0x0011, 
    0x0110, 0x0111, 0x0014, 0x0015, 0x0114, 0x0115, 0x0410, 0x0411, 0x0510, 0x0511, 0x0414, 0x0415, 0x0514, 0x0515, 0x1000, 0x1001, 0x1100, 0x1101, 
    0x1004, 0x1005, 0x1104, 0x1105, 0x1400, 0x1401, 0x1500, 0x1501, 0x1404, 0x1405, 0x1504, 0x1505, 0x1010, 0x1011, 0x1110, 0x1111, 0x1014, 0x1015, 
    0x1114, 0x1115, 0x1410, 0x1411, 0x1510, 0x1511, 0x1414, 0x1415, 0x1514, 0x1515, 0x0040, 0x0041, 0x0140, 0x0141, 0x0044, 0x0045, 0x0144, 0x0145, 
    0x0440, 0x0441, 0x0540, 0x0541, 0x0444, 0x0445, 0x0544, 0x0545, 0x0050, 0x0051, 0x0150, 0x0151, 0x0054, 0x0055, 0x0154, 0x0155, 0x0450, 0x0451, 
    0x0550, 0x0551, 0x0454, 0x0455, 0x0554, 0x0555, 0x1040, 0x1041, 0x1140, 0x1141, 0x1044, 0x1045, 0x1144, 0x1145, 0x1440, 0x1441, 0x1540, 0x1541, 
    0x1444, 0x1445, 0x1544, 0x1545, 0x1050, 0x1051, 0x1150, 0x1151, 0x1054, 0x1055, 0x1154, 0x1155, 0x1450, 0x1451, 0x1550, 0x1551, 0x1454, 0x1455, 
    0x1554, 0x1555, 0x4000, 0x4001, 0x4100, 0x4101, 0x4004, 0x4005, 0x4104, 0x4105, 0x4400, 0x4401, 0x4500, 0x4501, 0x4404, 0x4405, 0x4504, 0x4505, 
    0x4010, 0x4011, 0x4110, 0x4111, 0x4014, 0x4015, 0x4114, 0x4115, 0x4410, 0x4411, 0x4510, 0x4511, 0x4414, 0x4415, 0x4514, 0x4515, 0x5000, 0x5001, 
    0x5100, 0x5101, 0x5004, 0x5005, 0x5104, 0x5105, 0x5400, 0x5401, 0x5500, 0x5501, 0x5404, 0x5405, 0x5504, 0x5505, 0x5010, 0x5011, 0x5110, 0x5111, 
    0x5014, 0x5015, 0x5114, 0x5115, 0x5410, 0x5411, 0x5510, 0x5511, 0x5414, 0x5415, 0x5514, 0x5515, 0x4040, 0x4041, 0x4140, 0x4141, 0x4044, 0x4045, 
    0x4144, 0x4145, 0x4440, 0x4441, 0x4540, 0x4541, 0x4444, 0x4445, 0x4544, 0x4545, 0x4050, 0x4051, 0x4150, 0x4151, 0x4054, 0x4055, 0x4154, 0x4155, 
    0x4450, 0x4451, 0x4550, 0x4551, 0x4454, 0x4455, 0x4554, 0x4555, 0x5040, 0x5041, 0x5140, 0x5141, 0x5044, 0x5045, 0x5144, 0x5145, 0x5440, 0x5441, 
    0x5540, 0x5541, 0x5444, 0x5445, 0x5544, 0x5545, 0x5050, 0x5051, 0x5150, 0x5151, 0x5054, 0x5055, 0x5154, 0x5155, 0x5450, 0x5451, 0x5550, 0x5551, 
    0x5454, 0x5455, 0x5554, 0x5555
};
static constexpr uint16_t bitplane_hi[256] = 
{ 
    0x0000, 0x0002, 0x0200, 0x0202, 0x0008, 0x000A, 0x0208, 0x020A, 0x0800, 0x0802, 0x0A00, 0x0A02, 0x0808, 0x080A, 0x0A08, 0x0A0A, 0x0020, 0x0022, 
    0x0220, 0x0222, 0x0028, 0x002A, 0x0228, 0x022A, 0x0820, 0x0822, 0x0A20, 0x0A22, 0x0828, 0x082A, 0x0A28, 0x0A2A, 0x2000, 0x2002, 0x2200, 0x2202, 
    0x2008, 0x200A, 0x2208, 0x220A, 0x2800, 0x2802, 0x2A00, 0x2A02, 0x2808, 0x280A, 0x2A08, 0x2A0A, 0x2020, 0x2022, 0x2220, 0x2222, 0x2028, 0x202A, 
    0x2228, 0x222A, 0x2820, 0x2822, 0x2A20, 0x2A22, 0x2828, 0x282A, 0x2A28, 0x2A2A, 0x0080, 0x0082, 0x0280, 0x0282, 0x0088, 0x008A, 0x0288, 0x028A, 
    0x0880, 0x0882, 0x0A80, 0x0A82, 0x0888, 0x088A, 0x0A88, 0x0A8A, 0x00A0, 0x00A2, 0x02A0, 0x02A2, 0x00A8, 0x00AA, 0x02A8, 0x02AA, 0x08A0, 0x08A2, 
    0x0AA0, 0x0AA2, 0x08A8, 0x08AA, 0x0AA8, 0x0AAA, 0x2080, 0x2082, 0x2280, 0x2282, 0x2088, 0x208A, 0x2288, 0x228A, 0x2880, 0x2882, 0x2A80, 0x2A82, 
    0x2888, 0x288A, 0x2A88, 0x2A8A, 0x20A0, 0x20A2, 0x22A0, 0x22A2, 0x20A8, 0x20AA, 0x22A8, 0x22AA, 0x28A0, 0x28A2, 0x2AA0, 0x2AA2, 0x28A8, 0x28AA, 
    0x2AA8, 0x2AAA, 0x8000, 0x8002, 0x8200, 0x8202, 0x8008, 0x800A, 0x8208, 0x820A, 0x8800, 0x8802, 0x8A00, 0x8A02, 0x8808, 0x880A, 0x8A08, 0x8A0A, 
    0x8020, 0x8022, 0x8220, 0x8222, 0x8028, 0x802A, 0x8228, 0x822A, 0x8820, 0x8822, 0x8A20, 0x8A22, 0x8828, 0x882A, 0x8A28, 0x8A2A, 0xA000, 0xA002, 
    0xA200, 0xA202, 0xA008, 0xA00A, 0xA208, 0xA20A, 0xA800, 0xA802, 0xAA00, 0xAA02, 0xA808, 0xA80A, 0xAA08, 0xAA0A, 0xA020, 0xA022, 0xA220, 0xA222, 
    0xA028, 0xA02A, 0xA228, 0xA22A, 0xA820, 0xA822, 0xAA20, 0xAA22, 0xA828, 0xA82A, 0xAA28, 0xAA2A, 0x8080, 0x8082, 0x8280, 0x8282, 0x8088, 0x808A, 
    0x8288, 0x828A, 0x8880, 0x8882, 0x8A80, 0x8A82, 0x8888, 0x888A, 0x8A88, 0x8A8A, 0x80A0, 0x80A2, 0x82A0, 0x82A2, 0x80A8, 0x80AA, 0x82A8, 0x82AA, 
    0x88A0, 0x88A2, 0x8AA0, 0x8AA2, 0x88A8, 0x88AA, 0x8AA8, 0x8AAA, 0xA080, 0xA082, 0xA280, 0xA282, 0xA088, 0xA08A, 0xA288, 0xA28A, 0xA880, 0xA882, 
    0xAA80, 0xAA82, 0xA888, 0xA88A, 0xAA88, 0xAA8A, 0xA0A0, 0xA0A2, 0xA2A0, 0xA2A2, 0xA0A8, 0xA0AA, 0xA2A8, 0xA2AA, 0xA8A0, 0xA8A2, 0xAAA0, 0xAAA2, 
    0xA8A8, 0xA8AA, 0xAAA8, 0xAAAA
};

Ppu2C02::Ppu2C02()
{
    memset(nametable, 0, sizeof(nametable));
    memset(palette_table, 0, sizeof(palette_table));
    memset(scanline_buffer, 0, sizeof(scanline_buffer));
    memset(sprite, 0, sizeof(sprite));
}

Ppu2C02::~Ppu2C02()
{

}

inline IRAM_ATTR void Ppu2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;
    
    if (cart->ppuWrite(addr, data)) return;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        ptr_nametable[(addr >> 10) & 3][addr & 0x03FF] = data;
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x001F;
        switch (addr)
        {
        case 0x0010: addr = 0x0000; break;
        case 0x0014: addr = 0x0004; break;
        case 0x0018: addr = 0x0008; break;
        case 0x001C: addr = 0x000C; break;
        }
        palette_table[addr] = data;
    }
}

inline IRAM_ATTR uint8_t Ppu2C02::ppuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;
    
    if (cart->ppuRead(addr, data)) return data;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        data = ptr_nametable[(addr >> 10) & 3][addr & 0x03FF];
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x001F;
        switch (addr)
        {
        case 0x0010: addr = 0x0000; break;
        case 0x0014: addr = 0x0004; break;
        case 0x0018: addr = 0x0008; break;
        case 0x001C: addr = 0x000C; break;
        }
        data = palette_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }

    return data;
}

IRAM_ATTR void Ppu2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0x2000: // PPUCTRL
        control.reg = data;
        t.nametable_x = control.nametable_x;
        t.nametable_y = control.nametable_y;
        break;
    case 0x2001: // PPUMASK
        mask.reg = data;
        break;
    case 0x2003: // OAMADDR
        OAMADDR = data;
        break;
    case 0x2004: // OAMDATA
        ptr_sprite[OAMADDR++] = data;
        break;
    case 0x2005: // PPUSCROLL
        if (w == 0)
        {
            x = data & 0x07;
            t.coarse_x = data >> 3;
        }
        else
        {
            t.fine_y = data & 0x07;
            t.coarse_y = data >> 3;
        }
        w = ~w;
        break;
    case 0x2006: // PPUADDR
        if (w == 0)
        {
            t.reg = (t.reg & 0x00FF) | (uint16_t)((data & 0x3F) << 8);
        }
        else
        {
            t.reg = (t.reg & 0xFF00) | data;
            v.reg = t.reg;
        }
        w = ~w;
        break;
    case 0x2007: // PPUDATA
        ppuWrite(v.reg, data);
        v.reg += (control.VRAM_addr_increment ? 32 : 1);
        break;
    }
}

IRAM_ATTR uint8_t Ppu2C02::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    switch (addr)
    {
    case 0x2002: // PPUSTATUS
        data = status.reg & 0xE0;
        status.VBlank = 0;
        w = 0;
        break;
    case 0x2004: // OAMDATA
        data = ptr_sprite[OAMADDR];
        break;
    case 0x2007: // PPUDATA
        data = PPUDATA_buffer;
        PPUDATA_buffer = ppuRead(v.reg);

        if (v.reg >= 0x3F00 && v.reg <= 0x3FFF) data = PPUDATA_buffer;
        v.reg += (control.VRAM_addr_increment ? 32 : 1);
        break;
    }

    return data;
}

IRAM_ATTR void Ppu2C02::setVBlank()
{
    status.VBlank = 1;
    if (control.Vblank_NMI) bus->NMI();
}

IRAM_ATTR void Ppu2C02::clearVBlank()
{
    status.VBlank = 0;
    status.sprite_zero_hit = 0;
    status.sprite_overflow = 0;
}

IRAM_ATTR void Ppu2C02::incrementY()
{
    if (!(mask.render_background || mask.render_sprite)) return;

    if (v.fine_y < 7)
    {
        v.fine_y++;
        return;
    }
    
    v.fine_y = 0;
    if (v.coarse_y == 29)
    {
        v.coarse_y = 0;
        v.nametable_y = ~v.nametable_y;
    }
    else if (v.coarse_y == 31) v.coarse_y = 0;
    else v.coarse_y++;
}

IRAM_ATTR void Ppu2C02::renderScanline()
{   
    // Show transparency pixel if not rendering background
    if (!mask.render_background)
    {
        memset(scanline_buffer, nes_palette[palette_table[0]], BUFFER_SIZE * sizeof(uint16_t));
        return;
    }

    uint16_t bg_color = nes_palette[palette_table[0]];
    ptr_buffer = scanline_buffer;
    ptr_scanline_meta = scanline_metadata;
    x_tile = v.coarse_x;
    y_tile = v.coarse_y;
    offset = (control.background_table_addr ? 0x1000 : 0x0000) + v.fine_y;
    nametable_index = (v.reg >> 10) & 3;

    nametable_byte_base = v.reg & 0x03E0;
    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base + x_tile];

    attribute_byte_base = 0x03C0 + ((y_tile & 0x1C) << 1);
    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base + (x_tile >> 2)];
    attribute_byte = *ptr_attribute++;
    attribute_shift = ((y_tile & 2) << 1) + (x_tile & 2);
    attribute = ((attribute_byte >> attribute_shift) & 3) << 2;

    static constexpr uint8_t pixel_shift[8] = { 14, 6, 12, 4, 10, 2, 8, 0 }; // Shifts to get the bits of a pixel
    for (int tile = 0; tile < 33; tile++)
    {
        tile_index = *ptr_tile++;
        ptr_pattern_tile = cart->ppuReadPtr(offset + (tile_index << 4)); 

        // draw to framebuffer
        uint16_t pattern = bitplane_hi[ptr_pattern_tile[8]] | bitplane_lo[ptr_pattern_tile[0]];
        uint16_t tile_palette[4];
        tile_palette[0] = bg_color;
        for (int t = 1; t < 4; t++) tile_palette[t] = nes_palette[READ_PALETTE(attribute + t)];
        for (int i = 0; i < 8; i++)
        {   
            uint8_t pixel = (pattern >> pixel_shift[i]) & 3;
            *ptr_buffer++ = tile_palette[pixel];
            *ptr_scanline_meta++ = (pixel == 0) ? 0x80 : 0x00; // Store if pixel is transparent for sprite rendering
        }

        x_tile++;
        if ((x_tile & 1) == 0)
        {
            if ((x_tile & 3) == 0)
            {
                if (x_tile == 32)
                {
                    // switch nametable
                    x_tile = 0;
                    nametable_index ^= 1;

                    // recalculate pointer to tile and attribute data
                    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base];
                    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base];
                }

                attribute_byte = *ptr_attribute++;
            }

            attribute_shift ^= 2;
            attribute = ((attribute_byte >> attribute_shift) & 0x03) << 2;
        }
    }
}

void Ppu2C02::renderSprites(uint16_t scanline)
{
    if (!mask.render_sprite) 
    {
        finishScanline(scanline);
        return;
    }

    OAM* ptr_sprite_OAM;
    uint8_t sprite_size;
    uint8_t sprite_count = 0;

    uint16_t bg_color = nes_palette[palette_table[0]];
    uint16_t tile_palette[4];
    tile_palette[0] = bg_color;

    ptr_sprite_OAM = sprite;
    offset = (control.sprite_table_addr ? 0x1000 : 0);

    sprite_size = (control.sprite_size ? 16 : 8);

    uint16_t* buffer_offset = scanline_buffer + x;
    uint8_t* metadata_offset = scanline_metadata + x;
    for (int i = 0; i < 64; i++, ptr_sprite_OAM++)
    {
        uint8_t sprite_x, sprite_y;
        sprite_y = ptr_sprite_OAM->y + 1;
        // Check if sprite is in scanline
        if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_size)) 
            || (sprite_y == 0) || (sprite_y >= 240))
            continue;

        int16_t y_offset;
        uint16_t tile_addr;

        sprite_x = ptr_sprite_OAM->x;
        tile_index = ptr_sprite_OAM->index;
        attribute_byte = ptr_sprite_OAM->attribute;
        attribute = ((attribute_byte & 0x03) << 2);
    
        ptr_buffer = buffer_offset + sprite_x;
        ptr_scanline_meta = metadata_offset + sprite_x;

        // If 8x16 sprite mode
        tile_addr = (control.sprite_size) ? ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4) : offset + (tile_index << 4);
        ptr_tile = cart->ppuReadPtr(tile_addr);

        y_offset = scanline - sprite_y;
        if (y_offset > 7) y_offset += 8;
        if (attribute_byte & 0x80) // If flip sprite vertically
        {
            y_offset -= (control.sprite_size) ? 23 : 7;
            ptr_tile -= y_offset;
        }
        else ptr_tile += y_offset;

        // Draw to buffer
        uint16_t pattern = bitplane_hi[ptr_tile[8]] | bitplane_lo[ptr_tile[0]];
        if (pattern)
        {
            uint8_t pixel[8];
            uint8_t palette_offset = 16 + attribute;
            for (int t = 1; t < 4; t++) tile_palette[t] = nes_palette[READ_PALETTE(palette_offset + t)];

            if (attribute_byte & 0x40) // If flip sprite horizontally
            {
                pixel[7] = (pattern >> 14) & 3;
                pixel[6] = (pattern >> 6) & 3;
                pixel[5] = (pattern >> 12) & 3;
                pixel[4] = (pattern >> 4) & 3;
                pixel[3] = (pattern >> 10) & 3;
                pixel[2] = (pattern >> 2) & 3;
                pixel[1] = (pattern >> 8) & 3;
                pixel[0] = pattern & 3;
            }
            else
            {
                pixel[0] = (pattern >> 14) & 3;
                pixel[1] = (pattern >> 6) & 3;
                pixel[2] = (pattern >> 12) & 3;
                pixel[3] = (pattern >> 4) & 3;
                pixel[4] = (pattern >> 10) & 3;
                pixel[5] = (pattern >> 2) & 3;
                pixel[6] = (pattern >> 8) & 3;
                pixel[7] = pattern & 3;
            }

            // Check for sprite 0 hit
            if (i == 0 && status.sprite_zero_hit == 0)
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j] && ((ptr_scanline_meta[j] & 0x80) == 0))
                    {
                        status.sprite_zero_hit = true;
                        break;
                    }
                }
            }

            // Render sprite pixels on scanline buffer
            if (attribute_byte & 0x20) // Sprite Priorty : 1 - behind background | 0 - in front of background
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j])
                    {
                        if (ptr_scanline_meta[j] & 0x80) ptr_buffer[j] = tile_palette[pixel[j]];
                        ptr_scanline_meta[j] |= 0x40;
                    }
                }
            }
            else
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j] && ((ptr_scanline_meta[j] & 0x40) == 0))
                    {
                        ptr_buffer[j] = tile_palette[pixel[j]];;
                        ptr_scanline_meta[j] |= 0x40;
                    }
                }
            }
        }

        // If sprite overflow, break
        if (++sprite_count == 8) { status.sprite_overflow = true; break; }
    }

    ptr_buffer = buffer_offset;
    cart->ppuScanline();
    finishScanline(scanline);
}

void Ppu2C02::fakeSpriteHit(uint16_t scanline)
{
    if (!mask.render_sprite || status.sprite_zero_hit) return;

    uint8_t sprite_size;
    offset = (control.sprite_table_addr ? 0x1000 : 0);
    sprite_size = (control.sprite_size ? 16 : 8);

    uint8_t sprite_x, sprite_y;
    sprite_y = sprite[0].y + 1;
    // Check if sprite is in scanline
    if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_size)) 
        || (sprite_y == 0) || (sprite_y >= 240))
        return;

    int16_t y_offset;
    uint16_t tile_addr;

    sprite_x = sprite[0].x;
    tile_index = sprite[0].index;
    attribute_byte = sprite[0].attribute;

    tile_addr = (control.sprite_size) ? ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4) : offset + (tile_index << 4);
    ptr_tile = cart->ppuReadPtr(tile_addr);

    y_offset = scanline - sprite_y;
    if (y_offset > 7) y_offset += 8;

    if (attribute_byte & 0x80) // If flip sprite vertically
    {
        y_offset -= (control.sprite_size) ? 23 : 7;
        ptr_tile -= y_offset;
    }
    else ptr_tile += y_offset;

    // Draw to buffer
    uint16_t pattern = bitplane_hi[ptr_tile[8]] | bitplane_lo[ptr_tile[0]];
    if (pattern)
    {
        status.sprite_zero_hit = true;
        return;
        // uint8_t pixel[8];
        // if (attribute_byte & 0x40) // If flip sprite horizontally
        // {
        //     pixel[7] = (pattern >> 14) & 3;
        //     pixel[6] = (pattern >> 6) & 3;
        //     pixel[5] = (pattern >> 12) & 3;
        //     pixel[4] = (pattern >> 4) & 3;
        //     pixel[3] = (pattern >> 10) & 3;
        //     pixel[2] = (pattern >> 2) & 3;
        //     pixel[1] = (pattern >> 8) & 3;
        //     pixel[0] = pattern & 3;
        // }
        // else
        // {
        //     pixel[0] = (pattern >> 14) & 3;
        //     pixel[1] = (pattern >> 6) & 3;
        //     pixel[2] = (pattern >> 12) & 3;
        //     pixel[3] = (pattern >> 4) & 3;
        //     pixel[4] = (pattern >> 10) & 3;
        //     pixel[5] = (pattern >> 2) & 3;
        //     pixel[6] = (pattern >> 8) & 3;
        //     pixel[7] = pattern & 3;
        // }

        // for (int i = 0; i < 8; i++)
        // {
        //     if (pixel[i]) 
        //     {
        //         status.sprite_zero_hit = true;
        //         return;
        //     }
        // }
    }
    cart->ppuScanline();
}

inline void Ppu2C02::finishScanline(uint16_t scanline)
{
    memcpy(ptr_display + (scanline_counter * SCANLINE_SIZE), ptr_buffer, SCANLINE_SIZE * sizeof(uint16_t));
    scanline_counter++;
    if (scanline_counter >= SCANLINES_PER_BUFFER) 
    { 
        bus->renderImage(scanline - (SCANLINES_PER_BUFFER - 1));
        scanline_counter = 0;
    }
}

void Ppu2C02::reset()
{
    status.reg = 0x00;
	mask.reg = 0x00;
	control.reg = 0x00;
	t.reg = 0x00;
	v.reg = 0x00;
	x = 0x00;
    w = 0x00;
    OAMADDR = 0x00;
    OAMDATA = 0x00;
	PPUDATA_buffer = 0x00;
    nametable_byte = 0x00;
    attribute_byte = 0x00;
}

void Ppu2C02::connectCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    setMirror((Cartridge::MIRROR)cart->hardware_mirror);
}

void Ppu2C02::setMirror(Cartridge::MIRROR mirror)
{
    switch (mirror)
    {
        case Cartridge::MIRROR::VERTICAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0400];
            ptr_nametable[2] = &nametable[0x0000];
            ptr_nametable[3] = &nametable[0x0400];
            break;

        case Cartridge::MIRROR::HORIZONTAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0000];
            ptr_nametable[2] = &nametable[0x0400];
            ptr_nametable[3] = &nametable[0x0400];
            break;

        case Cartridge::MIRROR::ONESCREEN_LOW:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0000];
            break;

        case Cartridge::MIRROR::ONESCREEN_HIGH:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0400];
            break;
    }
}