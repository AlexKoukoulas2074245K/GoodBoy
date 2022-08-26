#include "cpu.h"
#include "logging.h"
#include "memory.h"
#include "timer.h"

/*
	Incremented at a rate of Clock/256 rate. Writing anything to this resets it 0 (R/W)
*/
static constexpr word DIV_REGISTER_ADDRESS = 0xFF04;
static constexpr word DIV_REGISTER_CYCLE_FREQ = 256;

/*
	Incremented at the clock frequency specified by TAC. See below. When the value overflows,
	a timer interrupt is requested and the value is reset to the one specified by the Timer Mod Register see below.
*/
static constexpr word TIMER_ACCUM_ADDRESS = 0xFF05;

/*
	When the timer accumulator register overflows, its value is reset to this register and a timer interrupt is requested.
	If a write to this register is executed on the same cycle as the content of this register is transferred to the accumulator due 
	to an overflow, then the old value is transferred to the accumulator above, rather than the new one written just now.
*/
static constexpr word TIMER_MOD_ADDRESS = 0xFF06;


/*
	Bit  2        Timer Accumulator Enable
	Bits 1 - 0    Input CLock Select:
			00: Clock/1024
			01: Clock/16
			10: Clock/64
			11: Clock/256
*/
static constexpr word TIMER_CONTROL_ADDRESS = 0xFF07;

#define IS_BIT_SET(bit, reg) (((reg >> bit) & 0x1) == 0x1)

Timer::Timer()
	: divRegisterCycleCounter_(0)
	, timerAccumRegisterCycleCounter_(0)
	, mem_(nullptr)
	, divRegister_(0)
	, timerAccumRegister_(0)
	, timerModRegister_(0)
	, timerControlRegister_(0)
	, nextTimerModValue_(0)
	, timerModUdpatedThisCycle_(false)
{
}

void Timer::update(const unsigned int spentCpuCycles)
{
	divRegisterCycleCounter_ += spentCpuCycles;
	if (divRegisterCycleCounter_ >= DIV_REGISTER_CYCLE_FREQ)
	{
		divRegisterCycleCounter_ -= DIV_REGISTER_CYCLE_FREQ;
		divRegister_++;
	}

	if (IS_BIT_SET(2, timerControlRegister_))
	{
		timerAccumRegisterCycleCounter_ += spentCpuCycles;

		word timerAccumFreq = 1024;
		switch (timerControlRegister_ & 0x03)
		{
			case 0x00: timerAccumFreq = 1024; break;
			case 0x01: timerAccumFreq = 16; break;
			case 0x02: timerAccumFreq = 64; break;
			case 0x03: timerAccumFreq = 256; break;
		}

		if (timerAccumRegisterCycleCounter_ >= timerAccumFreq)
		{
			timerAccumRegisterCycleCounter_ -= timerAccumFreq;
			timerAccumRegister_++;
			if (timerAccumRegister_ == 0)
			{
				cpu_->triggerInterrupt(CPU::TIMER_INTERRUPT_BIT);
				timerAccumRegister_ = timerModRegister_;
			}
		}
	}

	if (timerModUdpatedThisCycle_)
	{
		timerModRegister_ = nextTimerModValue_;
		timerModUdpatedThisCycle_ = false;
	}
}

byte Timer::readByteAt(const word address) const
{
	switch (address)
	{
		case DIV_REGISTER_ADDRESS: return divRegister_;
		case TIMER_ACCUM_ADDRESS: return timerAccumRegister_;
		case TIMER_MOD_ADDRESS: return timerModRegister_;
		case TIMER_CONTROL_ADDRESS: return timerControlRegister_;
		default:
			log(LogType::WARNING, "Unknown TIMER read at %s", getHexWord(address));
	}
	return 0xFF;
}

void Timer::writeByteAt(const word address, const byte b)
{
	switch (address)
	{
		case DIV_REGISTER_ADDRESS: divRegister_ = 0x00; break; // Writing anything to the divider register resets it to 0
		case TIMER_ACCUM_ADDRESS: timerAccumRegister_ = b; break;
		case TIMER_MOD_ADDRESS: timerModUdpatedThisCycle_ = true; nextTimerModValue_ = b; break; // see same cycle mod update edge case above
		case TIMER_CONTROL_ADDRESS: timerControlRegister_ = (b & 0x07); timerAccumRegisterCycleCounter_ = 0; break; // Only 3 bottom bits are writeable
		default:
			log(LogType::WARNING, "Unknown TIMER write %s at %s", getHexByte(b), getHexWord(address));
	}
}