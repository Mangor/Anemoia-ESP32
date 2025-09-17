#ifndef MAPPER_H
#define MAPPER_H

#include <cstring>
#include <stdint.h>
#include <stdlib.h>

#include "Arduino.h"

class Cartridge;
struct MapperVTable;

class Mapper
{
public: 
    enum ROM_TYPE
    {
        PRG_ROM,
        CHR_ROM
    };

    const MapperVTable* vtable = nullptr;
    void* state = nullptr;
};

struct MapperVTable
{
    bool (*cpuRead)(Mapper* mapper, uint16_t addr, uint8_t& data);
    bool (*cpuWrite)(Mapper* mapper, uint16_t addr, uint8_t data);
    bool (*ppuRead)(Mapper* mapper, uint16_t addr, uint8_t& data);
    bool (*ppuWrite)(Mapper* mapper, uint16_t addr, uint8_t data);
    uint8_t* (*ppuReadPtr)(Mapper* mapper, uint16_t addr);
    void (*scanline)(Mapper* mapper);
};

struct Bank
{
    uint8_t bank_id;
    uint8_t* bank_ptr;
    uint32_t last_used;
    uint32_t size;
};

struct BankCache
{
    Bank* banks;
    uint8_t num_banks;
    uint32_t tick;
    Cartridge* cart;
};

void bankInit(BankCache* cache, Bank* banks, uint8_t num_banks, uint32_t bank_size, Cartridge* cart);
uint8_t* getBank(BankCache* cache, uint8_t bank_id, Mapper::ROM_TYPE rom);

#endif