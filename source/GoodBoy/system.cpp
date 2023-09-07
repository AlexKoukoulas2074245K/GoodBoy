#include "system.h"

System::System()
	: display_()
	, cartridge_()
	, joypad_()
	, timer_()
	, apu_()
	, mem_(display_, cartridge_, joypad_, timer_, apu_)
	, cpu_(mem_, display_)
{
	display_.setMemory(&mem_);
	display_.setMainMemoryBlock(mem_.mem_);
	joypad_.setMemory(mem_.mem_);
	timer_.setMemory(mem_.mem_);

	display_.setCPU(&cpu_);
	joypad_.setCPU(&cpu_);
	timer_.setCPU(&cpu_);
}

unsigned int System::emulateNextMachineStep()
{
	// Update CPU
	unsigned int cpuClockCycles = cpu_.executeNextInstruction();

	// Update display based on spent clock cycles
	display_.update(cpuClockCycles);
	
	// Update timer based on spent clock cycles
	timer_.update(cpuClockCycles);

	// Update apu based on spent clock cycles
	apu_.update(cpuClockCycles);

	// Handle interrupts
	cpuClockCycles += cpu_.handleInterrupts();

	return cpuClockCycles;
}

std::string System::loadCartridge(const char* filename)
{
	const auto& cartridgeName = cartridge_.loadCartridge(filename);
	mem_.setCartridgeCgbType(cartridge_.getCgbType());
	display_.setCartridgeCgbType(cartridge_.getCgbType());
	return cartridgeName;
}

void System::setInputState(const byte actionButtons, const byte directionButtons)
{
	joypad_.setJoypadState(actionButtons, directionButtons);
}

void System::setVBlankCallback(Display::VBlankCallback cb)
{
	display_.setVBlankCallback(cb);
}

void System::toggleSoundDisabled()
{
    apu_.setSoundDisabled(!apu_.isSoundDisabled());
}

bool System::isSoundDisabled() const
{
     return apu_.isSoundDisabled();
}

