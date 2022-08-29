#ifndef TIMER_H
#define TIMER_H

#include "types.h"

class CPU;
class Timer final
{
	friend class System;
public:
	Timer();

	void setMemory(byte* mem) { mem_ = mem; }
	void setCPU(CPU* cpu) { cpu_ = cpu; }

	void update(const unsigned int spentCpuCycles);

	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);

private:
	int divRegisterCycleCounter_;
	int timerAccumRegisterCycleCounter_;
	CPU* cpu_;
	byte* mem_;
	byte divRegister_;
	byte timerAccumRegister_;
	byte timerModRegister_;
	byte timerControlRegister_;
	byte nextTimerModValue_;
	bool timerModUdpatedThisCycle_;
	bool timerAccumulatorEnabled_;
};

#endif