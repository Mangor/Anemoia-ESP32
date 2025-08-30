#include "bus.h"

uint16_t buffer[256];
Bus::Bus()
{
    memset(RAM, 0, sizeof(RAM));
    memset(controller, 0, sizeof(controller));
    memset(controller_state, 0, sizeof(controller_state));
    cpu.connectBus(this);
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
    // 1 frame == 321 dots * 261 scanlines
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
        cpu.scanlineClock();

        #ifndef FRAMESKIP
            renderScanline(ppu_scanline + 1);
        #else
            if (frame_latch) { renderScanline(ppu_scanline + 1); }
            else { ppu.fakeSpriteHit(ppu_scanline + 1); }
        #endif
        cpu.scanlineClock();
        cpu.clock();

        #ifndef FRAMESKIP
            renderScanline(ppu_scanline + 2);
        #else
            if (frame_latch) { renderScanline(ppu_scanline + 2); }
            else { ppu.fakeSpriteHit(ppu_scanline + 2); }
        #endif
        cpu.scanlineClock();
        cpu.clock();
    }

    // Setup for the next frame
    // Same reason as scanlines 0-239, 2/3 of scanlines will have an extra CPU clock. 
    // Scanline 240
    cpu.scanlineClock();

    // Scanline 241
    ppu.setVBlank();
    if (ppu.nmi)
    {
        ppu.nmi = false;
        cpu.NMI();
    }
    cpu.scanlineClock();
    cpu.clock();

    // Scanline 242
    cpu.scanlineClock();
    cpu.clock();

    // Scanline 243-258
    for (ppu_scanline = 243; ppu_scanline < 259; ppu_scanline += 3)
    {
        cpu.scanlineClock();

        cpu.scanlineClock();
        cpu.clock();

        cpu.scanlineClock();
        cpu.clock();
    }

    // Scanline 259-261
    cpu.scanlineClock();

    cpu.scanlineClock();
    cpu.clock();

    ppu.clearVBlank();
    cpu.scanlineClock();
    cpu.clock();


    frame_latch = !frame_latch;
}

//     static uint32_t frame_counter = 0;

//     //PROFILE_BEGIN(total)
//     if (ppu_scanline < 240)
//     {
//         if (system_clock_counter == 1)
//         {
//             //PROFILE_BEGIN(scroll);
//             ppu.transferScroll(ppu_scanline);
//             //PROFILE_END(scroll);

//             //PROFILE_BEGIN(ppuRender);
//             //ppu.renderScanline();
//             //ptr_screen->pushImageDMA(32, ppu_scanline, 256, 1, ppu.ptr_buffer);
//             //PROFILE_END(ppuRender);
//         }

//         else if (system_clock_counter == 256)
//         {
//             //PROFILE_BEGIN(incrementY);
//             ppu.incrementY();
//             //PROFILE_END(incrementY);
//         }
//     }
//    else if (system_clock_counter == 1 && ppu_scanline == 241)
//     {
//         //PROFILE_BEGIN(setVBlank);
//         ppu.setVBlank();
//         if (ppu.nmi)
//         {
//             ppu.nmi = false;
//             cpu.NMI();
//         }
//         //PROFILE_END(setVBlank);
//     }
//     else if (system_clock_counter == 1 && ppu_scanline == 261)
//     {
//         //PROFILE_BEGIN(clearVBlank);
//         ppu.clearVBlank();
//         //PROFILE_END(clearVBlank);
//     }


//     if (system_clock_counter == next_cpu_cycle)
//     {
//         next_cpu_cycle += 3;
//         if (!OAM_DMA_transfer)
//         {
//             //PROFILE_BEGIN(cpu);
//             //cpu.clock();     
//             //PROFILE_END(cpu);
//         }
//         else
//         {
//             //PROFILE_BEGIN(dma);
//             if (OAM_DMA_transfer)
//             {
//                 if (OAM_DMA_alignment && (system_clock_counter & 1)) OAM_DMA_alignment = false;
//                 else 
//                 {
//                     if ((system_clock_counter & 1) == 0) OAM_DMA_data = cpuRead(OAM_DMA_page << 8 | OAM_DMA_addr);
//                     else
//                     {
//                         ppu.ptr_sprite[OAM_DMA_addr] = OAM_DMA_data;
//                         OAM_DMA_addr++;
//                         if (OAM_DMA_addr == 0x00)
//                         {
//                             OAM_DMA_transfer = false;
//                             OAM_DMA_alignment = true;
//                         }
//                     }
//                 }
//             }
//             //PROFILE_END(dma);
//         }
//     }
//     // apu.clock();

//     //if (apu.IRQ) cpu.IRQ();

//     system_clock_counter++;
//     if (system_clock_counter == 341)
//     {
//         system_clock_counter = 0;
//         next_cpu_cycle -= 341;
//         ppu_scanline++;
//         if (ppu_scanline == 262) 
//         {
//             ppu_scanline = 0;
//             frame_counter++;

//             // if (frame_counter % 60 == 0)
//             // {
//             //     PROFILE_REPORT(ppuRender);
//             //     PROFILE_REPORT(cpu);
//             //     PROFILE_REPORT(scroll);
//             //     PROFILE_REPORT(dma);
//             //     PROFILE_REPORT(incrementY);
//             //     PROFILE_REPORT(setVBlank);
//             //     PROFILE_REPORT(clearVBlank);
//             //     PROFILE_REPORT(total);

//             //     PROFILE_PERCENT(ppuRender, total);
//             //     PROFILE_PERCENT(cpu, total);
//             //     PROFILE_PERCENT(scroll, total);
//             //     PROFILE_PERCENT(dma, total);
//             //     PROFILE_PERCENT(incrementY, total);
//             //     PROFILE_PERCENT(setVBlank, total);
//             //     PROFILE_PERCENT(clearVBlank, total);

//             //     PROFILE_RESET(ppuRender);
//             //     PROFILE_RESET(cpu);
//             //     PROFILE_RESET(scroll);
//             //     PROFILE_RESET(dma);
//             //     PROFILE_RESET(incrementY);
//             //     PROFILE_RESET(setVBlank);
//             //     PROFILE_RESET(clearVBlank);
//             //     PROFILE_RESET(total);
//             // }
//         }
//     }
//     //PROFILE_END(total);
//}

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

inline IRAM_ATTR void Bus::renderScanline(uint16_t scanline)
{
    ppu.transferScroll(scanline);
    ppu.renderScanline();
    ppu.renderSprites(scanline);
    ptr_screen->pushImageDMA(32, scanline, 256, 1, ppu.ptr_buffer, buffer);
    ppu.incrementY();
}