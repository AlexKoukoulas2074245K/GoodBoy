#include "cartridge.h"
#include "display.h"
#include "joypad.h"
#include "logging.h"
#include "memory.h"
#include "timer.h"
#include "types.h"

#include <cassert>
#include <memory.h>
#include <stdio.h>


static const byte bios[256] =
{
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
	0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
	0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
	0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
	0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
	0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
	0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

Memory::Memory(Display& display, Cartridge& cartridge, Joypad& joypad, Timer& timer)
	: display_(display)
	, cartridge_(cartridge)
	, joypad_(joypad)
	, timer_(timer)
{
	memset(mem_, 0, sizeof(mem_));
	memcpy(mem_, bios, sizeof(bios));
	inBios_ = true;
}

sbyte Memory::readSByteAt(const word address) const 
{
	return static_cast<sbyte>(readAt(address)); 
}

word Memory::readWordAt(const word address) const 
{
	return (readAt(address + 1) << 8 | readAt(address)); 
}

byte Memory::readByteAt(const word address) const 
{
	return readAt(address);
}

void Memory::writeWordAt(const word address, const word w) 
{
	writeAt(address, w & 0x00FF);
	writeAt(address + 1, w >> 8);
}

void Memory::writeByteAt(const word address, const byte b) 
{
	writeAt(address, b);
}

byte Memory::readAt(const word address) const
{
	//assert(!(address >= ECHO_WRAM_START_ADDRESS && address <= ECHO_WRAM_END_ADDRESS)); // echo ram writing is prohibited
	assert(!(address >= UNUSABLE_START_ADDRESS && address <= UNUSABLE_END_ADDRESS));   // unusable memory writing is prohibited

	
	if (address < CARTRIDGE_HEADER_ADDRESS && inBios_) return mem_[address];
	if (address <= ROM_BANK_1_N_END_ADDRESS) return cartridge_.readByteAt(address);

	if (address >= VRAM_START_ADDRESS && address <= VRAM_END_ADDRESS)
		return display_.readByteAt(address);
	else if (address >= OAM_START_ADDRESS && address <= OAM_END_ADDRESS)
		return display_.readByteAt(address);
	else if (address == JOYPAD_ADDRESS)
		return joypad_.readByteAt(address);
	else if (address >= SERIAL_TRANSFER_START_ADDRESS && address <= SERIAL_TRANSFER_END_ADDRESS) {}
		//log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at SERIAL (" + getHexWord(SERIAL_TRANSFER_START_ADDRESS) + "-" + getHexWord(SERIAL_TRANSFER_END_ADDRESS) + ")").c_str());
	else if (address >= TIMER_START_ADDRESS && address <= TIMER_END_ADDRESS)
		return timer_.readByteAt(address);
	else if (address >= SOUND_START_ADDRESS && address <= SOUND_END_ADDRESS) {}
		//log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at SOUND (" + getHexWord(SOUND_START_ADDRESS) + "-" + getHexWord(SOUND_END_ADDRESS) + ")").c_str());
	else if (address >= WAVE_START_ADDRESS && address <= WAVE_END_ADDRESS) {}
		//log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at WAVE (" + getHexWord(WAVE_START_ADDRESS) + "-" + getHexWord(WAVE_END_ADDRESS) + ")").c_str());
	else if (address >= LCD_START_ADDRESS && address <= LCD_END_ADDRESS)
		return display_.readByteAt(address);
	else if (address == VRAM_BANK_SELECT_ADDRESS)
		log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at VRAM_BANK_SELECT (" + getHexWord(VRAM_BANK_SELECT_ADDRESS) + ")").c_str());
	else if (address == DISABLE_BOOT_ROM_ADDRESS)
		log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at DISABLE_BOOT_ROM_ADDRESS (" + getHexWord(DISABLE_BOOT_ROM_ADDRESS) + ").").c_str());
	else if (address >= VRAM_DMA_START_ADDRESS && address <= VRAM_DMA_END_ADDRESS)
		log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at VRAM_DMA (" + getHexWord(VRAM_DMA_START_ADDRESS) + "-" + getHexWord(VRAM_DMA_END_ADDRESS) + ")").c_str());
	else if (address >= BG_OBJ_PALETTES_START_ADDRESS && address <= BG_OBJ_PALETTES_END_ADDRESS)
		log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at BJ_OBJ_PALLETES (" + getHexWord(BG_OBJ_PALETTES_START_ADDRESS) + "-" + getHexWord(BG_OBJ_PALETTES_END_ADDRESS) + ")").c_str());
	else if (address == WRAM_BANK_SELECT_ADDRESS)
		log(LogType::INFO, ("Reading from " + getHexWord(address) + ": at WRAM_BANK_SELECT (" + getHexWord(WRAM_BANK_SELECT_ADDRESS) + ")").c_str());

	return mem_[address];
}

void Memory::writeAt(const word address, const byte b)
{
	// On DMG, during this time (DMA), the CPU can access only HRAM(memory at $FF80 - $FFFE)
	if (display_.dmaTransferInProgress())
	{
		if (!(address >= HRAM_START_ADDRESS && address <= HRAM_END_ADDRESS))
		{
			log(LogType::WARNING, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + ". DMA is in progress and writes to anywhere but HRAM are ignored").c_str());
			
			if (display_.respectsIllegalReadWrites()) return;
		}
	}

	assert(!(address >= ECHO_WRAM_START_ADDRESS && address <= ECHO_WRAM_END_ADDRESS)); // echo ram writing is prohibited
	
	if (address >= UNUSABLE_START_ADDRESS && address <= UNUSABLE_END_ADDRESS)   // should not write to unusable memory block
	{
		log(LogType::WARNING, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + "  at unusuable address space").c_str());
	}
	
	if (address <= ROM_BANK_1_N_START_ADDRESS)
	{
		cartridge_.writeByteAt(address, b);
		return;
	}
	else if (address >= VRAM_START_ADDRESS && address <= VRAM_END_ADDRESS)
	{
		display_.writeByteAt(address, b);
		return;
	}
	else if (address >= OAM_START_ADDRESS && address <= OAM_END_ADDRESS)
	{
		display_.writeByteAt(address, b);
		return;
	}
	else if (address == JOYPAD_ADDRESS)
	{
		joypad_.writeByteAt(address, b);
		return;
	}
	else if (address >= SERIAL_TRANSFER_START_ADDRESS && address <= SERIAL_TRANSFER_END_ADDRESS) {}
		//log(LogType::INFO, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + "  SERIAL (" + getHexWord(SERIAL_TRANSFER_START_ADDRESS) + "-" + getHexWord(SERIAL_TRANSFER_END_ADDRESS) + ")").c_str());
	else if (address >= TIMER_START_ADDRESS && address <= TIMER_END_ADDRESS)
	{
		timer_.writeByteAt(address, b);
		return;
	}
	else if (address >= SOUND_START_ADDRESS && address <= SOUND_END_ADDRESS) {}
		//log(LogType::INFO, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + "  SOUND (" + getHexWord(SOUND_START_ADDRESS) + "-" + getHexWord(SOUND_END_ADDRESS) + ")").c_str());
	else if (address >= WAVE_START_ADDRESS && address <= WAVE_END_ADDRESS) {}
		//log(LogType::INFO, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + "  WAVE (" + getHexWord(WAVE_START_ADDRESS) + "-" + getHexWord(WAVE_END_ADDRESS) + ")").c_str());
	else if (address >= LCD_START_ADDRESS && address <= LCD_END_ADDRESS)
	{
		display_.writeByteAt(address, b);
		return;
	}
	else if (address == VRAM_BANK_SELECT_ADDRESS)
		log(LogType::INFO, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + "  VRAM_BANK_SELECT (" + getHexWord(VRAM_BANK_SELECT_ADDRESS) + "). CGB Only.").c_str());
	else if (address == DISABLE_BOOT_ROM_ADDRESS)
	{
		log(LogType::INFO, ("Writing: " + getHexByte(b) + " at " + getHexWord(address) + "  DISABLE_BOOT_ROM_ADDRESS (" + getHexWord(DISABLE_BOOT_ROM_ADDRESS) + ").").c_str());
		inBios_ = !(b > 0x0);
	}
	else if (address >= VRAM_DMA_START_ADDRESS && address <= VRAM_DMA_END_ADDRESS)
		log(LogType::INFO, ("Writing: " + getHexByte(b) + " at  " + getHexWord(address) + " VRAM_DMA (" + getHexWord(VRAM_DMA_START_ADDRESS) + "-" + getHexWord(VRAM_DMA_END_ADDRESS) + "). CGB Only.").c_str());
	else if (address >= BG_OBJ_PALETTES_START_ADDRESS && address <= BG_OBJ_PALETTES_END_ADDRESS)
		log(LogType::INFO, ("Writing: " + getHexByte(b) + " at  " + getHexWord(address) + " BJ_OBJ_PALLETES (" + getHexWord(BG_OBJ_PALETTES_START_ADDRESS) + "-" + getHexWord(BG_OBJ_PALETTES_END_ADDRESS) + "). CGB Only.").c_str());
	else if (address == WRAM_BANK_SELECT_ADDRESS)
		log(LogType::INFO, ("Writing: " + getHexByte(b) + " at  " + getHexWord(address) + " WRAM_BANK_SELECT (" + getHexWord(WRAM_BANK_SELECT_ADDRESS) + "). CGB Only.").c_str());

	mem_[address] = b;
}