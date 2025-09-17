#include "mapper000.h"
#include "cartridge.h"

IRAM_ATTR bool mapper000_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    mapper000_state* state = (mapper000_state*)mapper->state;
    uint16_t offset = addr & 0x3FFF;
    uint8_t bank_id = (addr >> 14) & 1;

    data = state->PRG_banks[bank_id][offset];
    return true;
}

IRAM_ATTR bool mapper000_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
	return false;
}

IRAM_ATTR bool mapper000_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    mapper000_state* state = (mapper000_state*)mapper->state;
    data = state->CHR_bank[addr];
    return true;
}

IRAM_ATTR bool mapper000_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;

    mapper000_state* state = (mapper000_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_bank[addr] = data;
        return true;
    }

	return false;
}

IRAM_ATTR uint8_t* mapper000_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;
    
    mapper000_state* state = (mapper000_state*)mapper->state;
    return &state->CHR_bank[addr];
}

const MapperVTable mapper000_vtable = {
    mapper000_cpuRead,
    mapper000_cpuWrite,
    mapper000_ppuRead,
    mapper000_ppuWrite,
    mapper000_ppuReadPtr,
    nullptr,
};

Mapper createMapper000(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &mapper000_vtable; 

    mapper000_state* state = new mapper000_state;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    state->PRG_banks[0] = &state->PRG_ROM[0];
    state->PRG_banks[1] = (state->number_PRG_banks > 1) ? &state->PRG_ROM[16*1024] : &state->PRG_ROM[0];
    state->CHR_bank = &state->CHR_ROM[0];

    cart->loadPRGBank(state->PRG_banks[0], 16*1024, 0);
    if (state->number_PRG_banks > 1) cart->loadPRGBank(state->PRG_banks[1], 16*1024, 16*1024);
    cart->loadCHRBank(state->CHR_bank, 8*1024, 0);

    mapper.state = state;
    return mapper;
}