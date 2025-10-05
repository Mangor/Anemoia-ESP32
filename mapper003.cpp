#include "mapper003.h"
#include "cartridge.h"

IRAM_ATTR bool Mapper003_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    data = state->PRG_bank[addr & 0x7FFF];
    return true;
}

IRAM_ATTR bool Mapper003_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x8000) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    uint8_t bank = data & 0x03;
    state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, bank, Mapper::ROM_TYPE::CHR_ROM);
    return true;
}

IRAM_ATTR bool Mapper003_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    data = state->ptr_CHR_bank_8K[addr];
    return true;
}

IRAM_ATTR bool Mapper003_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
	return false;
}

IRAM_ATTR uint8_t* Mapper003_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    return &state->ptr_CHR_bank_8K[addr];
}

const MapperVTable Mapper003_vtable = {
    Mapper003_cpuRead,
    Mapper003_cpuWrite,
    Mapper003_ppuRead,
    Mapper003_ppuWrite,
    Mapper003_ppuReadPtr,
    nullptr,
};

Mapper createMapper003(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &Mapper003_vtable; 
    Mapper003_state* state = new Mapper003_state;
    bankInit(&state->CHR_cache_8K, state->CHR_banks_8K, MAPPER003_NUM_CHR_BANKS_8K, 8*1024, cart);

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, 0, Mapper::ROM_TYPE::CHR_ROM);

    cart->loadPRGBank(state->PRG_bank, 32*1024, 0);

    mapper.state = state;
    return mapper;
}