#include "system.h"

System::System()
	: display_()
	, cartridge_()
	, joypad_()
	, timer_()
	, mem_(display_, cartridge_, joypad_, timer_)
	, cpu_(mem_)
{
	display_.setMemory(mem_.mem_);
	joypad_.setMemory(mem_.mem_);
	timer_.setMemory(mem_.mem_);

	display_.setCPU(&cpu_);
	joypad_.setCPU(&cpu_);
	timer_.setCPU(&cpu_);
}

unsigned int System::emulateNextMachineStep()
{
	// Update CPU
	unsigned int cpuclockCycles = cpu_.executeNextInstruction();

	// Update display based on spent clock cycles
	display_.update(cpuclockCycles);
	
	// Update timer based on spent clock cycles
	timer_.update(cpuclockCycles);

	// Handle interrupts
	cpuclockCycles += cpu_.handleInterrupts();

	return cpuclockCycles;
}

std::string System::loadCartridge(const char* filename)
{
	return cartridge_.loadCartridge(filename);
}

void System::setInputState(const byte actionButtons, const byte directionButtons)
{
	joypad_.setJoypadState(actionButtons, directionButtons);
}

void System::setVBlankCallback(Display::VBlankCallback cb)
{
	display_.setVBlankCallback(cb);
}