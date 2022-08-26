#ifndef JOYPAD_H
#define JOYPAD_H

#include "types.h"

class CPU;
class Joypad final
{
public:
	Joypad();

	void setMemory(byte* mem) { mem_ = mem; }
	void setCPU(CPU* cpu) { cpu_ = cpu; }
	void setJoypadState(const byte actionButtons, const byte directionButtons);
	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);

private:
	CPU* cpu_;
	byte* mem_;
	byte joypadRegister_;
	byte lastActionButtonsState_;
	byte lastDirectionButtonsState_;
};


#endif