#include "cartridge.h"
#include "bus.h"

Cartridge::Cartridge(const char* filename)
{
    struct cartridge_header
    {
        char name[4];
        uint8_t PRG_ROM_chunks;
        uint8_t CHR_ROM_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t PRG_RAM_size;
        uint8_t tv_system;
        uint8_t tv_system2;
        char unused[5];
    } header;

    // Load Cartridge ROM
    rom = SD.open(filename, FILE_READ);
    if (!rom) return;

    rom.read((uint8_t*)&header, sizeof(cartridge_header));
    if (header.mapper1 & 0x04) rom.seek(rom.position() + 512);

	mapper_ID = (header.mapper2 & 0xF0) | header.mapper1 >> 4;
    hardware_mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;

    // Check file format
    uint8_t file_type = 1;
    if ((header.mapper2 & 0x0C) == 0x08);
    switch (file_type)
    {
    case 1:
        number_PRG_banks = header.PRG_ROM_chunks;
        number_CHR_banks = header.CHR_ROM_chunks;

        // PRG_memory.resize(number_PRG_banks * 16384);
        // file.read((uint8_t*)PRG_memory.data(), PRG_memory.size());

        // if (number_CHR_banks == 0) CHR_memory.resize(8192);
        // else CHR_memory.resize(number_CHR_banks * 8192);
        // file.read((uint8_t*)CHR_memory.data(), CHR_memory.size());
        break;
    case 2:
        number_PRG_banks = ((header.PRG_RAM_size & 0x07) << 8) | header.PRG_ROM_chunks;
        number_CHR_banks = ((header.PRG_RAM_size & 0x38) << 8) | header.CHR_ROM_chunks;

        // PRG_memory.resize(number_PRG_banks * 16384);
        // file.read((uint8_t*)PRG_memory.data(), PRG_memory.size());

        // if (number_CHR_banks == 0) CHR_memory.resize(8192);
        // else CHR_memory.resize(number_CHR_banks * 8192);
        // file.read((uint8_t*)CHR_memory.data(), CHR_memory.size());
        break;
    }

    prg_base = sizeof(cartridge_header) + (header.mapper1 & 0x04 ? 512 : 0);
    chr_base = prg_base + (number_PRG_banks * 16384);
    switch (mapper_ID)
    {
        case 0: mapper = createMapper000(number_PRG_banks, number_CHR_banks, this); break;
        case 1: mapper = createMapper001(number_PRG_banks, number_CHR_banks, this); break;
        case 2: mapper = createMapper002(number_PRG_banks, number_CHR_banks, this); break;
        case 3: mapper = createMapper003(number_PRG_banks, number_CHR_banks, this); break;
        case 4: mapper = createMapper004(number_PRG_banks, number_CHR_banks, this); break;
    }
}

Cartridge::~Cartridge()
{

}


IRAM_ATTR bool Cartridge::cpuRead(uint16_t addr, uint8_t& data)
{
	return mapper.vtable->cpuRead(&mapper, addr, data);
}

IRAM_ATTR bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
	return mapper.vtable->cpuWrite(&mapper, addr, data);
}

IRAM_ATTR bool Cartridge::ppuRead(uint16_t addr, uint8_t& data)
{
	return mapper.vtable->ppuRead(&mapper, addr, data);
}

IRAM_ATTR bool Cartridge::ppuWrite(uint16_t addr, uint8_t data)
{
	return mapper.vtable->ppuWrite(&mapper, addr, data);	
}

IRAM_ATTR uint8_t* Cartridge::ppuReadPtr(uint16_t addr)
{
	return mapper.vtable->ppuReadPtr(&mapper, addr);
}

void Cartridge::ppuScanline()
{
    if (mapper.vtable->scanline)
        mapper.vtable->scanline(&mapper);
}

IRAM_ATTR void Cartridge::loadPRGBank(uint8_t* bank, uint16_t size, uint32_t offset)
{
    rom.seek(prg_base + offset);
    rom.read(bank, size);
}

IRAM_ATTR void Cartridge::loadCHRBank(uint8_t* bank, uint16_t size, uint32_t offset)
{
    rom.seek(chr_base + offset);
    rom.read(bank, size);
}

IRAM_ATTR void Cartridge::setMirrorMode(MIRROR mirror)
{
    bus->setPPUMirrorMode(mirror);
}

void Cartridge::IRQ()
{
    bus->IRQ();
}