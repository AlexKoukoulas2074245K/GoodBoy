#ifndef SYSTEM_H
#define SYSTEM_H

#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "display.h"
#include "joypad.h"
#include "memory.h"
#include "timer.h"

class System final
{
public:
	static constexpr byte ACTION_BUTTON_A_MASK = 0x1;
	static constexpr byte ACTION_BUTTON_B_MASK = 0x2;
	static constexpr byte ACTION_BUTTON_SELECT_MASK = 0x4;
	static constexpr byte ACTION_BUTTON_START_MASK = 0x8;

	static constexpr byte DIRECTION_BUTTON_RIGHT_MASK = 0x1;
	static constexpr byte DIRECTION_BUTTON_LEFT_MASK = 0x2;
	static constexpr byte DIRECTION_BUTTON_UP_MASK = 0x4;
	static constexpr byte DIRECTION_BUTTON_DOWN_MASK = 0x8;

public:
	System();
	
	unsigned int emulateNextMachineStep();

	std::string loadCartridge(const char* filename);

	void setInputState(const byte actionButtons, const byte directionButtons);
	void setVBlankCallback(Display::VBlankCallback cb);
    
    void toggleSoundDisabled();
    bool isSoundDisabled() const;
    
private:
	Display display_;
	Cartridge cartridge_;
	Joypad joypad_;
	Timer timer_;
	APU apu_;
	Memory mem_;
	CPU cpu_;
};

#endif
