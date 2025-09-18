#include "bus.h"

Bus::Bus()
{
    memset(RAM, 0, sizeof(RAM));
    memset(controller, 0, sizeof(controller));
    memset(controller_state, 0, sizeof(controller_state));
    cpu.connectBus(this);
    ppu.connectBus(this);
}

Bus::~Bus()
{
}

IRAM_ATTR void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
    if (cart->cpuWrite(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {
        RAM[addr & 0x07FF] = data;
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        ppu.cpuWrite(addr, data);
    }
    else if ((addr & 0xF000) == 0x4000 && (addr <= 0x4013 || addr == 0x4015 || addr == 0x4017))
    {
        cpu.apuWrite(addr, data);
    }
    else if (addr == 0x4014)
    {
        cpu.OAM_DMA(data);
    }
    else if (addr == 0x4016)
    {
        controller_strobe = data & 1;
        if (controller_strobe)
        {
            controller_state[0] = controller[0];
            controller_state[1] = controller[1];
        }
    }
}

IRAM_ATTR uint8_t Bus::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;

    if (cart->cpuRead(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {   
        data = RAM[addr & 0x07FF];
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        data = ppu.cpuRead(addr);
    }
    else if (addr == 0x4016 || addr == 0x4017)
    {
        uint8_t value = controller_state[addr & 1] & 1;
        if (!controller_strobe)
            controller_state[addr & 1] >>= 1;
        data = value | 0x40;
    }
    return data;
}

void Bus::reset()
{
    ptr_screen->fillScreen(TFT_BLACK);
	for (auto& i : RAM) i = 0x00;
	//cart->reset();
	cpu.reset();
	ppu.reset();
	//apu.reset();
	system_clock_counter = 0;
}

void Bus::resetClock()
{
	system_clock_counter = 0;
}

IRAM_ATTR void Bus::clock()
{
    // 1 frame == 341 dots * 261 scanlines
    // Visible scanlines 0-239
    
    // Rendering 3 scanlines at a time because 1 CPU clock == 3 PPU clocks
    // and there's only 341 ppu clocks (dots) in a scanline, which is not divisible by 3.
    // Using a counter/for loop with += 341 & -= 3 is too big of a performance hit.
    // 1 scanline == ~113.67 CPU clocks, so for every 3 scanlines, two scanlines will have an extra CPU clock
    for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
    {
        #ifndef FRAMESKIP
            renderScanline(ppu_scanline);
        #else
            if (frame_latch) { renderScanline(ppu_scanline); }
            else { ppu.fakeSpriteHit(ppu_scanline); }
        #endif
        cpu.clock(113);

        #ifndef FRAMESKIP
            renderScanline(ppu_scanline + 1);
        #else
            if (frame_latch) { renderScanline(ppu_scanline + 1); }
            else { ppu.fakeSpriteHit(ppu_scanline + 1); }
        #endif
        cpu.clock(114);

        #ifndef FRAMESKIP
            renderScanline(ppu_scanline + 2);
        #else
            if (frame_latch) { renderScanline(ppu_scanline + 2); }
            else { ppu.fakeSpriteHit(ppu_scanline + 2); }
        #endif
        cpu.clock(114);
    }

    // Setup for the next frame
    // Same reason as scanlines 0-239, 2/3 of scanlines will have an extra CPU clock. 
    // Scanline 240
    cpu.clock(113);

    // Scanline 241
    ppu.setVBlank();
    cpu.clock(114);

    // Scanline 242
    cpu.clock(114);

    // Scanline 243-258
    for (ppu_scanline = 243; ppu_scanline < 259; ppu_scanline += 3)
    {
        cpu.clock(113);

        cpu.clock(114);

        cpu.clock(114);
    }

    // Scanline 259-261
    cpu.clock(113);

    cpu.clock(114);

    ppu.clearVBlank();
    cpu.clock(114);


    frame_latch = !frame_latch;
}

IRAM_ATTR void Bus::setPPUMirrorMode(Cartridge::MIRROR mirror)
{
    ppu.setMirror(mirror);
}

IRAM_ATTR void Bus::OAM_Write(uint8_t addr, uint8_t data)
{
    ppu.ptr_sprite[addr] = data;
}

void Bus::insertCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    ppu.connectCartridge(cartridge);
    cart->connectBus(this);
}

void Bus::connectScreen(TFT_eSPI* screen)
{
    ptr_screen = screen;
}

inline void Bus::renderScanline(uint16_t scanline)
{
    ppu.transferScroll(scanline);
    ppu.renderScanline();
    ppu.renderSprites(scanline);
    ppu.incrementY();
}

IRAM_ATTR void Bus::renderImage(uint16_t scanline)
{
    ptr_screen->pushImageDMA(32, scanline, 256, SCANLINES_PER_BUFFER, ppu.ptr_display);
} 

IRAM_ATTR void Bus::IRQ()
{
    cpu.IRQ();
}

IRAM_ATTR void Bus::NMI()
{
    cpu.NMI();
}