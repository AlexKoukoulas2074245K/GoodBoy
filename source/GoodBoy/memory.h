#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"
#include "cartridge.h"

class APU;
class Display;
class Joypad;
class Timer;
class Memory final
{
public:
	static constexpr word ROM_BANK_0_START_ADDRESS   = 0x0000;
	static constexpr word ROM_BANK_0_END_ADDRESS     = 0x3FFF;
	static constexpr word ROM_BANK_1_N_START_ADDRESS = 0x4000;
	static constexpr word ROM_BANK_1_N_END_ADDRESS   = 0x7FFF;
	static constexpr word VRAM_START_ADDRESS         = 0x8000;
	static constexpr word VRAM_END_ADDRESS           = 0x9FFF;
	static constexpr word EXTERNAL_RAM_START_ADDRESS = 0xA000;
	static constexpr word EXTERNAL_RAM_END_ADDRESS   = 0xBFFF;
	static constexpr word WRAM_0_START_ADDRESS       = 0xC000;
	static constexpr word WRAM_0_END_ADDRESS         = 0xCFFF;
	static constexpr word WRAM_1_START_ADDRESS       = 0xD000;
	static constexpr word WRAM_1_END_ADDRESS         = 0xDFFF;
	static constexpr word ECHO_WRAM_START_ADDRESS    = 0xE000;
	static constexpr word ECHO_WRAM_END_ADDRESS      = 0xFDFF;
	static constexpr word OAM_START_ADDRESS          = 0xFE00;
	static constexpr word OAM_END_ADDRESS            = 0xFE9F;
	static constexpr word UNUSABLE_START_ADDRESS     = 0xFEA0;
	static constexpr word UNUSABLE_END_ADDRESS       = 0xFEFF;
	static constexpr word HRAM_START_ADDRESS         = 0xFF80;
	static constexpr word HRAM_END_ADDRESS           = 0xFFFE;

	static constexpr word CARTRIDGE_HEADER_START_ADDRESS = 0x0100;
	static constexpr word CARTRIDGE_HEADER_END_ADDRESS   = 0x014F;
	static constexpr word JOYPAD_ADDRESS                 = 0xFF00;
	static constexpr word SERIAL_TRANSFER_START_ADDRESS  = 0xFF01;
	static constexpr word SERIAL_TRANSFER_END_ADDRESS    = 0xFF02;
	static constexpr word TIMER_START_ADDRESS            = 0xFF04;
	static constexpr word TIMER_END_ADDRESS              = 0xFF07;
	static constexpr word IF_ADDRESS                     = 0xFF0F;
	static constexpr word SOUND_START_ADDRESS            = 0xFF10;
	static constexpr word SOUND_END_ADDRESS              = 0xFF3F;
	static constexpr word LCD_START_ADDRESS              = 0xFF40;
	static constexpr word LCD_END_ADDRESS                = 0xFF4B;
	static constexpr word CGB_SPEED_SWITCH_ADDRESS       = 0xFF4D; // CGB+
	static constexpr word VRAM_BANK_SELECT_ADDRESS       = 0xFF4F; // CGB+
	static constexpr word DISABLE_BOOT_ROM_ADDRESS       = 0xFF50;
	static constexpr word VRAM_DMA_START_ADDRESS         = 0xFF51; // CGB+
	static constexpr word VRAM_DMA_END_ADDRESS           = 0xFF55; // CGB+
	static constexpr word BG_OBJ_PALETTES_START_ADDRESS  = 0xFF68; // CGB+
	static constexpr word BG_OBJ_PALETTES_END_ADDRESS    = 0xFF6B; // CGB+
	static constexpr word OBJECT_PRIORITY_ADDRESS        = 0xFF6C; // CGB+
	static constexpr word WRAM_BANK_SELECT_ADDRESS       = 0xFF70; // CGB+
	static constexpr word IE_ADDRESS                     = 0xFFFF;

public:
	friend class System;
	Memory(Display&, Cartridge&, Joypad&, Timer&, APU&);

	void setCartridgeCgbType(Cartridge::CgbType cgbType) { cgbType_ = cgbType; }

	sbyte readSByteAt(const word address) const;

	word readWordAt(const word address) const;
	byte readByteAt(const word address) const;

	void writeWordAt(const word address, const word w);
	void writeByteAt(const word address, const byte b);

private:
	byte readAt(const word address) const;
	void writeAt(const word address, const byte b);

	byte mem_[0x10000];
	byte cgbWram_[0x8000];
	APU& apu_;
	Display& display_;
	Cartridge& cartridge_;
	Joypad& joypad_;
	Timer& timer_;
	byte cgbWramBank_;
	Cartridge::CgbType cgbType_;
	bool inBios_;
};

#endif /* MEMORY_H */

