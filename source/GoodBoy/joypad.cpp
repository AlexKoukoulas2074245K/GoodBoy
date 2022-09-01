#include "cpu.h"
#include "joypad.h"

#define IS_BIT_SET(bit, reg) (((reg >> bit) & 0x1) == 0x1)

/*
	In joypad land, 1 means unpressed and 0 means pressed. So initial state of register 
	would be CF (1100 1111) meaning all ones except the writeable bits (bit 4 and bit 5)
*/
static constexpr byte JOYPAD_REGISTER_INITIAL_STATE = 0xCF; 

Joypad::Joypad()
	: mem_(nullptr)
	, joypadRegister_(JOYPAD_REGISTER_INITIAL_STATE)
	, lastActionButtonsState_(0)
	, lastDirectionButtonsState_(0)
{

}

void Joypad::setJoypadState(const byte actionButtons, const byte directionButtons)
{
	lastActionButtonsState_ = actionButtons;
	lastDirectionButtonsState_ = directionButtons;
}

byte Joypad::readByteAt(const word) const
{
	return joypadRegister_;
}

void Joypad::writeByteAt(const word, const byte b)
{
	joypadRegister_ &= (~0x30);
	joypadRegister_ |= (b & 0x30); // only bits 4 & 5 are r/w

	// If action buttons are selected by resetting bit 5
	if (!(IS_BIT_SET(5, joypadRegister_)))
	{
		// The bottom 4 bits of the register become the inverse of the last action state mask.
		// (The inverse because we send in a mask indicating 1 for presses, and the gameboy
		// reads 1 as unpressed).
		joypadRegister_ = (joypadRegister_ & 0xF0) | ((~lastActionButtonsState_) & 0x0F);
		
		// Also if in the final bottom 4-bits state there is at least one (enabled) button that is pressed at this point, 
		// (i.e. we have at least one 0 in the 4 bottom bits) Request a Joypad interrupt.
		if ((joypadRegister_ & 0x0F) != 0x0F)
		{
			cpu_->triggerInterrupt(CPU::JOYPAD_INTERRUPT_BIT);
		}
	}
	// If direction buttons are selected by resetting bit 4
	if (!(IS_BIT_SET(4, joypadRegister_)))
	{
		// The bottom 4 bits of the register become the inverse of the last direction state mask.
		// (The inverse because we send in a mask indicating 1 for presses, and the gameboy
		// reads 1 as unpressed).
		joypadRegister_ = (joypadRegister_ & 0xF0) | ((~lastDirectionButtonsState_) & 0x0F);

		// Also if in the final bottom 4-bits state there is at least one (enabled) button that is pressed at this point, 
		// (i.e. we have at least one 0 in the 4 bottom bits) Request a Joypad interrupt.
		if ((joypadRegister_ & 0x0F) != 0x0F)
		{
			cpu_->triggerInterrupt(CPU::JOYPAD_INTERRUPT_BIT);
		}
	}
}