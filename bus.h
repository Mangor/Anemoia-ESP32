#ifndef BUS_H
#define BUS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdint.h>
#include "config.h"
#include "cartridge.h"
#include "cpu6502.h"
#include "ppu2C02.h"

class Bus
{
public:
    Bus();
    ~Bus();

public:
    Cpu6502 cpu;
    Ppu2C02 ppu;
    Cartridge* cart;
    uint8_t RAM[2048];
    uint8_t controller[2];

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void setPPUMirrorMode(Cartridge::MIRROR mirror);

    void insertCartridge(Cartridge* cartridge);
    void connectScreen(TFT_eSPI* screen);
    void reset();
    void resetClock();
    void clock();
    void IRQ();
    void OAM_Write(uint8_t addr, uint8_t data);
    uint16_t ppu_scanline = 0;
    void renderImage(uint16_t scanline);

private:
    void cpuClock();
    void renderScanline(uint16_t scanline);
    TFT_eSPI* ptr_screen;
    uint16_t system_clock_counter = 0;
    uint16_t cpu_clock = 0;
    uint8_t controller_state[2];
    uint8_t controller_strobe = 0x00;
    bool frame_latch = false;
};

#endif