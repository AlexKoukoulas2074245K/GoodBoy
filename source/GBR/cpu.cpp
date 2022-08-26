#define _CRT_SECURE_NO_WARNINGS

#include "cpu.h"
#include "logging.h"

#include <iomanip>
#include <sstream>
#include <stdio.h>

static byte coreInstructionClockCycles[256] = 
{   /*          0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
	/* 0x00 */  4,  12, 8,  8,  4,  4,  8,  4,  20, 8,  8,  8,  4,  4,  8,  4,
	/* 0x10 */  0,  12, 8,  8,  4,  4,  8,  4,  12, 8,  8,  8,  4,  4,  8,  4,
	/* 0x20 */  12, 12, 8,  8,  4,  4,  8,  4,  12, 8,  8,  8,  4,  4,  8,  4,
	/* 0x30 */  12, 12, 8,  8, 12, 12, 12,  4,  12, 8,  8,  8,  4,  4,  8,  4,
	/* 0x40 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0x50 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0x60 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0x70 */  8,  8,  8,  8,  8,  8,  0,  8,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0x80 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0x90 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0xA0 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0xB0 */  4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
	/* 0xC0 */  20, 12, 16, 16, 24, 16, 8,  16, 20, 16, 16, 0,  24, 24, 8,  16,
	/* 0xD0 */  20, 12, 16, 0,  24, 16, 8,  16, 20, 16, 16, 0,  24, 0,  8,  16,
	/* 0xE0 */  12, 12, 8,  0,  0,  16, 8,  16, 16, 4,  16, 0,  0,  0,  8,  16,
	/* 0xF0 */  12, 12, 8,  4,  0,  16, 8,  16, 12, 8,  16, 4,  0,  0,  8,  16
};

static byte cbInstructionClockCycles[256] =
{   /*          0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
	/* 0x00 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0x10 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0x20 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0x30 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0x40 */  8,  8,  8,  8,  8,  8,  12, 8,  8,  8,  8,  8,  8,  8,  12, 8,
	/* 0x50 */  8,  8,  8,  8,  8,  8,  12, 8,  8,  8,  8,  8,  8,  8,  12, 8,
	/* 0x60 */  8,  8,  8,  8,  8,  8,  12, 8,  8,  8,  8,  8,  8,  8,  12, 8,
	/* 0x70 */  8,  8,  8,  8,  8,  8,  12, 8,  8,  8,  8,  8,  8,  8,  12, 8,
	/* 0x80 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0x90 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0xA0 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0xB0 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0xC0 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0xD0 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0xE0 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,
	/* 0xF0 */  8,  8,  8,  8,  8,  8,  16, 8,  8,  8,  8,  8,  8,  8,  16, 8,

};

FILE* f;
std::stringstream s;

#define SET_Z_FLAG() registersAF_ |= 0x80
#define SET_N_FLAG() registersAF_ |= 0x40
#define SET_H_FLAG() registersAF_ |= 0x20
#define SET_C_FLAG() registersAF_ |= 0x10

#define RESET_Z_FLAG() registersAF_ &= ~0x80
#define RESET_N_FLAG() registersAF_ &= ~0x40
#define RESET_H_FLAG() registersAF_ &= ~0x20
#define RESET_C_FLAG() registersAF_ &= ~0x10

#define IS_Z_FLAG_SET() ((registersAF_ & 0x80) == 0x80)
#define IS_N_FLAG_SET() ((registersAF_ & 0x40) == 0x40)
#define IS_H_FLAG_SET() ((registersAF_ & 0x20) == 0x20)
#define IS_C_FLAG_SET() ((registersAF_ & 0x10) == 0x10)

static constexpr byte VBLANK_INTERRUPT_NEGATED_MASK   = ~(0x01);
static constexpr byte LCD_STAT_INTERRUPT_NEGATED_MASK = ~(0x02);
static constexpr byte TIMER_INTERRUPT_NEGATED_MASK    = ~(0x04);
static constexpr byte SERIAL_INTERRUPT_NEGATED_MASK   = ~(0x08);
static constexpr byte JOYPAD_INTERRUPT_NEGATED_MASK   = ~(0x10);

static constexpr word VBLANK_INTERRUPT_HANDLER_ADDRESS   = 0x40;
static constexpr word LCD_STAT_INTERRUPT_HANDLER_ADDRESS = 0x48;
static constexpr word TIMER_INTERRUPT_HANDLER_ADDRESS    = 0x50;
static constexpr word SERIAL_INTERRUPT_HANDLER_ADDRESS   = 0x58;
static constexpr word JOYPAD_INTERRUPT_HANDLER_ADDRESS   = 0x60;

static constexpr byte ISR_EXECUTION_CLOCK_CYCLES = 0;

CPU::CPU(Memory& mem)
	: mem_(mem)
	, registersAF_(0)
	, registersPC_(0)
	, isHalted_(false)
	, shouldDumpState_(false)
	, ime_(false)
	, eiTriggered_(false)
{
	memset(generalPurposeRegisters_, 0, sizeof(generalPurposeRegisters_));	
}

CPU::~CPU()
{
	/*f = fopen("out.txt", "w");
	fprintf(f, s.str().c_str());*/
}

unsigned int CPU::executeNextInstruction()
{
	if (registersPC_ == 0x213) 
	{
		auto b = false;
	}

	if (isHalted_)
	{
		return coreInstructionClockCycles[0];
	}

	currentInstructionOperands_.clear();
	byte opcode = readByteAtPC();
	unsigned int clockCycles = coreInstructionClockCycles[opcode];

	switch (opcode)
	{
		// noop
		case 0x00: break;

		// 8-bit Loads
		case 0x06: setRegByte(REG_B_INDEX, readByteAtPC()); break;		
		case 0x0E: setRegByte(REG_C_INDEX, readByteAtPC()); break;
		case 0x16: setRegByte(REG_D_INDEX, readByteAtPC()); break;
		case 0x1E: setRegByte(REG_E_INDEX, readByteAtPC()); break;
		case 0x26: setRegByte(REG_H_INDEX, readByteAtPC()); break;
		case 0x2E: setRegByte(REG_L_INDEX, readByteAtPC()); break;
		case 0x7F: setRegAByte(getRegAByte()); break;
		case 0x78: setRegAByte(getRegByte(REG_B_INDEX)); break;
		case 0x79: setRegAByte(getRegByte(REG_C_INDEX)); break;
		case 0x7A: setRegAByte(getRegByte(REG_D_INDEX)); break;
		case 0x7B: setRegAByte(getRegByte(REG_E_INDEX)); break;
		case 0x7C: setRegAByte(getRegByte(REG_H_INDEX)); break;
		case 0x7D: setRegAByte(getRegByte(REG_L_INDEX)); break;
		case 0x0A: setRegAByte(mem_.readByteAt(getRegWord(REG_BC_INDEX))); break;
		case 0x1A: setRegAByte(mem_.readByteAt(getRegWord(REG_DE_INDEX))); break;
		case 0x7E: setRegAByte(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xFA: setRegAByte(mem_.readByteAt(readWordAtPc())); break;
		case 0x3E: setRegAByte(readByteAtPC()); break;
		case 0xF2: setRegAByte(mem_.readByteAt(0xFF00 + getRegByte(REG_C_INDEX))); break;
		case 0xE2: mem_.writeByteAt(0xFF00 + getRegByte(REG_C_INDEX), getRegAByte()); break;
		case 0x47: setRegByte(REG_B_INDEX, getRegAByte()); break;
		case 0x4F: setRegByte(REG_C_INDEX, getRegAByte()); break;
		case 0x57: setRegByte(REG_D_INDEX, getRegAByte()); break;
		case 0x5F: setRegByte(REG_E_INDEX, getRegAByte()); break;
		case 0x67: setRegByte(REG_H_INDEX, getRegAByte()); break;
		case 0x6F: setRegByte(REG_L_INDEX, getRegAByte()); break;
		case 0x02: mem_.writeByteAt(getRegWord(REG_BC_INDEX), getRegAByte()); break;
		case 0x12: mem_.writeByteAt(getRegWord(REG_DE_INDEX), getRegAByte()); break;
		case 0x77: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegAByte()); break;
		case 0xEA: mem_.writeByteAt(readWordAtPc(), getRegAByte()); break;
		case 0xE0: mem_.writeByteAt(0xFF00 + readByteAtPC(), getRegAByte()); break;
		case 0xF0: setRegAByte(mem_.readByteAt(0xFF00 + readByteAtPC())); break;
		
		case 0x40: setRegByte(REG_B_INDEX, getRegByte(REG_B_INDEX)); break;
		case 0x41: setRegByte(REG_B_INDEX, getRegByte(REG_C_INDEX)); break;
		case 0x42: setRegByte(REG_B_INDEX, getRegByte(REG_D_INDEX)); break;
		case 0x43: setRegByte(REG_B_INDEX, getRegByte(REG_E_INDEX)); break;
		case 0x44: setRegByte(REG_B_INDEX, getRegByte(REG_H_INDEX)); break;
		case 0x45: setRegByte(REG_B_INDEX, getRegByte(REG_L_INDEX)); break;
		case 0x46: setRegByte(REG_B_INDEX, mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;

		case 0x48: setRegByte(REG_C_INDEX, getRegByte(REG_B_INDEX)); break;
		case 0x49: setRegByte(REG_C_INDEX, getRegByte(REG_C_INDEX)); break;
		case 0x4A: setRegByte(REG_C_INDEX, getRegByte(REG_D_INDEX)); break;
		case 0x4B: setRegByte(REG_C_INDEX, getRegByte(REG_E_INDEX)); break;
		case 0x4C: setRegByte(REG_C_INDEX, getRegByte(REG_H_INDEX)); break;
		case 0x4D: setRegByte(REG_C_INDEX, getRegByte(REG_L_INDEX)); break;
		case 0x4E: setRegByte(REG_C_INDEX, mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;

		case 0x50: setRegByte(REG_D_INDEX, getRegByte(REG_B_INDEX)); break;
		case 0x51: setRegByte(REG_D_INDEX, getRegByte(REG_C_INDEX)); break;
		case 0x52: setRegByte(REG_D_INDEX, getRegByte(REG_D_INDEX)); break;
		case 0x53: setRegByte(REG_D_INDEX, getRegByte(REG_E_INDEX)); break;
		case 0x54: setRegByte(REG_D_INDEX, getRegByte(REG_H_INDEX)); break;
		case 0x55: setRegByte(REG_D_INDEX, getRegByte(REG_L_INDEX)); break;
		case 0x56: setRegByte(REG_D_INDEX, mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;

		case 0x58: setRegByte(REG_E_INDEX, getRegByte(REG_B_INDEX)); break;
		case 0x59: setRegByte(REG_E_INDEX, getRegByte(REG_C_INDEX)); break;
		case 0x5A: setRegByte(REG_E_INDEX, getRegByte(REG_D_INDEX)); break;
		case 0x5B: setRegByte(REG_E_INDEX, getRegByte(REG_E_INDEX)); break;
		case 0x5C: setRegByte(REG_E_INDEX, getRegByte(REG_H_INDEX)); break;
		case 0x5D: setRegByte(REG_E_INDEX, getRegByte(REG_L_INDEX)); break;
		case 0x5E: setRegByte(REG_E_INDEX, mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;

		case 0x60: setRegByte(REG_H_INDEX, getRegByte(REG_B_INDEX)); break;
		case 0x61: setRegByte(REG_H_INDEX, getRegByte(REG_C_INDEX)); break;
		case 0x62: setRegByte(REG_H_INDEX, getRegByte(REG_D_INDEX)); break;
		case 0x63: setRegByte(REG_H_INDEX, getRegByte(REG_E_INDEX)); break;
		case 0x64: setRegByte(REG_H_INDEX, getRegByte(REG_H_INDEX)); break;
		case 0x65: setRegByte(REG_H_INDEX, getRegByte(REG_L_INDEX)); break;
		case 0x66: setRegByte(REG_H_INDEX, mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;

		case 0x68: setRegByte(REG_L_INDEX, getRegByte(REG_B_INDEX)); break;
		case 0x69: setRegByte(REG_L_INDEX, getRegByte(REG_C_INDEX)); break;
		case 0x6A: setRegByte(REG_L_INDEX, getRegByte(REG_D_INDEX)); break;
		case 0x6B: setRegByte(REG_L_INDEX, getRegByte(REG_E_INDEX)); break;
		case 0x6C: setRegByte(REG_L_INDEX, getRegByte(REG_H_INDEX)); break;
		case 0x6D: setRegByte(REG_L_INDEX, getRegByte(REG_L_INDEX)); break;
		case 0x6E: setRegByte(REG_L_INDEX, mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;

		case 0x70: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegByte(REG_B_INDEX)); break;
		case 0x71: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegByte(REG_C_INDEX)); break;
		case 0x72: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegByte(REG_D_INDEX)); break;
		case 0x73: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegByte(REG_E_INDEX)); break;
		case 0x74: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegByte(REG_H_INDEX)); break;
		case 0x75: mem_.writeByteAt(getRegWord(REG_HL_INDEX), getRegByte(REG_L_INDEX)); break;
		case 0x36: mem_.writeByteAt(getRegWord(REG_HL_INDEX), readByteAtPC()); break;

		case 0x22: ldihla(); break;
		case 0x2A: ldiahl(); break;
		case 0x32: lddhla(); break;
		case 0x3A: lddahl(); break;

		// 16-bit Loads
		case 0x01:
		case 0x11:
		case 0x21:
		case 0x31: setRegWord((opcode >> 4) * sizeof(word), readWordAtPc()); break;

		// LD SP,HL
		case 0xF9: setRegWord(REG_SP_INDEX, getRegWord(REG_HL_INDEX)); break;

		// LD nn, SP
		case 0x08: mem_.writeWordAt(readWordAtPc(), getRegWord(REG_SP_INDEX)); break;

		// XOR A n
		case 0xAF: xoran((registersAF_ & 0xFF00) >> 8); break;
		case 0xA8: xoran(getRegByte(REG_B_INDEX)); break;
		case 0xA9: xoran(getRegByte(REG_C_INDEX)); break;
		case 0xAA: xoran(getRegByte(REG_D_INDEX)); break;
		case 0xAB: xoran(getRegByte(REG_E_INDEX)); break;
		case 0xAC: xoran(getRegByte(REG_H_INDEX)); break;
		case 0xAD: xoran(getRegByte(REG_L_INDEX)); break;
		case 0xAE: xoran(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xEE: xoran(readByteAtPC()); break;

		// OR A n
		case 0xB7: oran((registersAF_ & 0xFF00) >> 8); break;
		case 0xB0: oran(getRegByte(REG_B_INDEX)); break;
		case 0xB1: oran(getRegByte(REG_C_INDEX)); break;
		case 0xB2: oran(getRegByte(REG_D_INDEX)); break;
		case 0xB3: oran(getRegByte(REG_E_INDEX)); break;
		case 0xB4: oran(getRegByte(REG_H_INDEX)); break;
		case 0xB5: oran(getRegByte(REG_L_INDEX)); break;
		case 0xB6: oran(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xF6: oran(readByteAtPC()); break;

		// AND A n
		case 0xA7: andan((registersAF_ & 0xFF00) >> 8); break;
		case 0xA0: andan(getRegByte(REG_B_INDEX)); break;
		case 0xA1: andan(getRegByte(REG_C_INDEX)); break;
		case 0xA2: andan(getRegByte(REG_D_INDEX)); break;
		case 0xA3: andan(getRegByte(REG_E_INDEX)); break;
		case 0xA4: andan(getRegByte(REG_H_INDEX)); break;
		case 0xA5: andan(getRegByte(REG_L_INDEX)); break;
		case 0xA6: andan(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xE6: andan(readByteAtPC()); break;

		// JR cc,n
		case 0x20: jrccn(!IS_Z_FLAG_SET(), readSByteAtPC()); break;
		case 0x28: jrccn(IS_Z_FLAG_SET(), readSByteAtPC()); break;
		case 0x30: jrccn(!IS_C_FLAG_SET(), readSByteAtPC()); break;
		case 0x38: jrccn(IS_C_FLAG_SET(), readSByteAtPC()); break;
		
		// RET CC
		case 0xC0: if (!IS_Z_FLAG_SET()) ret();  break;
		case 0xC8: if (IS_Z_FLAG_SET()) ret();  break;
		case 0xD0: if (!IS_C_FLAG_SET()) ret(); break;
		case 0xD8: if (IS_C_FLAG_SET()) ret(); break;
			
		// INC n
		case 0x3C: inca(); break;
		case 0x04: incn(REG_B_INDEX); break;
		case 0x0C: incn(REG_C_INDEX); break;
		case 0x14: incn(REG_D_INDEX); break;
		case 0x1C: incn(REG_E_INDEX); break;
		case 0x24: incn(REG_H_INDEX); break;
		case 0x2C: incn(REG_L_INDEX); break;
		case 0x34: inchl(); break;

		// DEC n
		case 0x3D: deca(); break;
		case 0x05: decn(REG_B_INDEX); break;
		case 0x0D: decn(REG_C_INDEX); break;
		case 0x15: decn(REG_D_INDEX); break;
		case 0x1D: decn(REG_E_INDEX); break;
		case 0x25: decn(REG_H_INDEX); break;
		case 0x2D: decn(REG_L_INDEX); break;
		case 0x35: dechl(); break;

		// CP A
		case 0xBF: cpa(getRegAByte()); break;
		case 0xB8: cpa(getRegByte(REG_B_INDEX)); break;
		case 0xB9: cpa(getRegByte(REG_C_INDEX)); break;
		case 0xBA: cpa(getRegByte(REG_D_INDEX)); break;
		case 0xBB: cpa(getRegByte(REG_E_INDEX)); break;
		case 0xBC: cpa(getRegByte(REG_H_INDEX)); break;
		case 0xBD: cpa(getRegByte(REG_L_INDEX)); break;
		case 0xBE: cpa(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xFE: cpa(readByteAtPC()); break;

		// ADD A n
		case 0x87: addan(getRegAByte()); break;
		case 0x80: addan(getRegByte(REG_B_INDEX)); break;
		case 0x81: addan(getRegByte(REG_C_INDEX)); break;
		case 0x82: addan(getRegByte(REG_D_INDEX)); break;
		case 0x83: addan(getRegByte(REG_E_INDEX)); break;
		case 0x84: addan(getRegByte(REG_H_INDEX)); break;
		case 0x85: addan(getRegByte(REG_L_INDEX)); break;
		case 0x86: addan(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xC6: addan(readByteAtPC()); break;

		// SUB A n
		case 0x97: suban(getRegAByte()); break;
		case 0x90: suban(getRegByte(REG_B_INDEX)); break;
		case 0x91: suban(getRegByte(REG_C_INDEX)); break;
		case 0x92: suban(getRegByte(REG_D_INDEX)); break;
		case 0x93: suban(getRegByte(REG_E_INDEX)); break;
		case 0x94: suban(getRegByte(REG_H_INDEX)); break;
		case 0x95: suban(getRegByte(REG_L_INDEX)); break;
		case 0x96: suban(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xD6: suban(readByteAtPC()); break;

		// ADD HL, n
		case 0x09:
		case 0x19:
		case 0x29:
		case 0x39: addhln(getRegWord(((opcode & 0xF0) >> 4) * sizeof(word))); break;

		// INC nn
		case 0x03: setRegWord(REG_BC_INDEX, getRegWord(REG_BC_INDEX) + 1); break;
		case 0x13: setRegWord(REG_DE_INDEX, getRegWord(REG_DE_INDEX) + 1); break;
		case 0x23: setRegWord(REG_HL_INDEX, getRegWord(REG_HL_INDEX) + 1); break;
		case 0x33: setRegWord(REG_SP_INDEX, getRegWord(REG_SP_INDEX) + 1); break;

		// dec nn
		case 0x0B:
		case 0x1B:
		case 0x2B:
		case 0x3B:
		{
			byte regIndex = ((opcode & 0xF0) >> 4) * 2;
			word regWord = getRegWord(regIndex);
			setRegWord(regIndex, regWord - 1);
		} break;

		// SCF
		case 0x37:
		{
			RESET_H_FLAG();
			RESET_N_FLAG();
			SET_C_FLAG();
		} break;

		// CCF
		case 0x3F: 
		{
			RESET_H_FLAG();
			RESET_N_FLAG();
			if (IS_C_FLAG_SET())
				RESET_C_FLAG();
			else 
				SET_C_FLAG();
		} break;


		// ADD SP, n
		case 0xE8: addspn();  break;

		// LDHL SP, n
		case 0xF8: ldhlspn(); break;

		// RLA  (Z cleared)
		case 0x07: rlca(); break;

		// RLCA (Z cleared)
		case 0x17: rla(); RESET_Z_FLAG();  break;

		// RRCA (Z cleared)
		case 0x0F: rrca(); break;

		//  RRA (Z cleared)
		case 0x1F: rra(); RESET_Z_FLAG(); break;

		// DAA
		case 0x27: daa(); break;

		// CPL 
		case 0x2F: cpl(); break;

		// DI
		case 0xF3: ime_ = false;  break;

		// EI
		case 0xFB: eiTriggered_ = true; break;

		// CALL nn
		case 0xCD:
		{
			word newAddress = readWordAtPc();
			callnn(newAddress);
		} break;

		// CALL cc, nn
		case 0xC4: { word newAddress = readWordAtPc(); if (!IS_Z_FLAG_SET()) callnn(newAddress); } break;
		case 0xCC: { word newAddress = readWordAtPc(); if (IS_Z_FLAG_SET()) callnn(newAddress); } break;
		case 0xD4: { word newAddress = readWordAtPc(); if (!IS_C_FLAG_SET()) callnn(newAddress); } break;
		case 0xDC: { word newAddress = readWordAtPc(); if (IS_C_FLAG_SET()) callnn(newAddress); } break;

		// PUSH nn
		case 0xF5: pushaf(); break;
		case 0xC5: pushnn(REG_BC_INDEX); break;
		case 0xD5: pushnn(REG_DE_INDEX); break;
		case 0xE5: pushnn(REG_HL_INDEX); break;
		
		// POP nn
		case 0xF1: popaf(); break;
		case 0xC1: popnn(REG_BC_INDEX); break;
		case 0xD1: popnn(REG_DE_INDEX); break;
		case 0xE1: popnn(REG_HL_INDEX); break;

		// RET
		case 0xC9: ret(); break;
		
		// RETI
		case 0xD9: ret(); eiTriggered_ = true; break;

		// JR N
		case 0x18: registersPC_ += readSByteAtPC(); break;
		
		// JP nn
		case 0xC3: registersPC_ = readWordAtPc(); break;

		// JP (hl)
		case 0xE9: registersPC_ = getRegWord(REG_HL_INDEX); break;

		// JP cc, nn
		case 0xC2: jpccn(!IS_Z_FLAG_SET(), readWordAtPc()); break;
		case 0xCA: jpccn(IS_Z_FLAG_SET(), readWordAtPc()); break;
		case 0xD2: jpccn(!IS_C_FLAG_SET(), readWordAtPc()); break;
		case 0xDA: jpccn(IS_C_FLAG_SET(), readWordAtPc()); break;

		// RST n
		case 0xC7: callnn(0x00);  break;
		case 0xCF: callnn(0x08);  break;
		case 0xD7: callnn(0x10);  break;
		case 0xDF: callnn(0x18);  break;
		case 0xE7: callnn(0x20);  break;
		case 0xEF: callnn(0x28);  break;
		case 0xF7: callnn(0x30);  break;
		case 0xFF: callnn(0x38);  break;

		// ADC A n
		case 0x8F: adcan(getRegAByte()); break;
		case 0x88: 
		case 0x89: 
		case 0x8A: 
		case 0x8B: 
		case 0x8C: 
		case 0x8D: adcan(getRegByte((opcode & 0x0F) - 0x08)); break;
		case 0x8E: adcan(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;
		case 0xCE: adcan(readByteAtPC()); break;

		// SBC A n
		case 0x9F: sbcan(getRegAByte()); break;
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D: sbcan(getRegByte((opcode & 0x0F) - 0x08)); break;
		case 0x9E: sbcan(mem_.readByteAt(getRegWord(REG_HL_INDEX))); break;		
		case 0xDE: sbcan(readByteAtPC()); break;

		// HALT
		case 0x76: isHalted_ = true; break;

		// CB
		case 0xCB:
		{
			byte cbOpcode = readByteAtPC();
			clockCycles = cbInstructionClockCycles[cbOpcode];

			switch (cbOpcode)
			{
				// RLC n
				case 0x07: cbrlca(); break;
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x04:
				case 0x05: cbrlcn(cbOpcode & 0x0F); break;
				case 0x06: cbrlchl(); break;

				// RRC n
				case 0x0F: cbrrca(); break;
				case 0x08: 
				case 0x09:
				case 0x0A:
				case 0x0B:
				case 0x0C:
				case 0x0D: cbrrcn((cbOpcode & 0x0F) - 0x08); break;
				case 0x0E: cbrrchl(); break;

				// RL n
				case 0x17: rla(); break;
				case 0x10:
				case 0x11:
				case 0x12:
				case 0x13:
				case 0x14:
				case 0x15: rln(cbOpcode & 0x0F); break;
				case 0x16: rlhl(); break;

				// RR n
				case 0x1F: rra(); break;
				case 0x18:
				case 0x19:
				case 0x1A:
				case 0x1B:
				case 0x1C:
				case 0x1D: rrn((cbOpcode & 0x0F) - 0x08); break;
				case 0x1E: rrhl(); break;
				
				// SL n
				case 0x27: sla(); break;
				case 0x20: 
				case 0x21: 
				case 0x22: 
				case 0x23: 
				case 0x24: 
				case 0x25: sln(cbOpcode & 0x0F); break;
				case 0x26: slhl(); break;
				
				// SR n
				case 0x2F: sra(); break;
				case 0x28:
				case 0x29:
				case 0x2A:
				case 0x2B:
				case 0x2C:
				case 0x2D: srn((cbOpcode & 0x0F) - 0x08); break;
				case 0x2E: srhl(); break;

				// SRL n
				case 0x3F: srla(); break;
				case 0x38:
				case 0x39:
				case 0x3A:
				case 0x3B:
				case 0x3C:
				case 0x3D: srln((cbOpcode & 0x0F) - 0x8); break;
				case 0x3E: srlhl(); break;

				// SWAP n
				case 0x37: swapa(); break;
				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: swapn(cbOpcode & 0x0F); break;
				case 0x36: swaphl(); break;

				// BIT b,r
				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
				case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
				{
					byte bit = (cbOpcode >> 3) & 0x07;
					byte reg = cbOpcode % 0x8;

					if (reg == 0x6)
						testBit(bit, mem_.readByteAt(getRegWord(REG_HL_INDEX)));
					else if (reg == 0x7)
						testBit(bit, getRegAByte());
					else
						testBit(bit, getRegByte(reg));
				} break;

				
				// RES b,r
				case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
				case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
				case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
				case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
				{
					byte bit = (cbOpcode >> 3) & 0x07;
					byte reg = cbOpcode % 0x8;

					if (reg == 0x6)
					{
						byte val = mem_.readByteAt(getRegWord(REG_HL_INDEX));
						resetBit(bit, val);
						mem_.writeByteAt(getRegWord(REG_HL_INDEX), val);
					}
					else if (reg == 0x7)
					{
						byte val = getRegAByte();
						resetBit(bit, val);
						setRegAByte(val);
					}
					else
					{
						byte val = getRegByte(reg);
						resetBit(bit, val);
						setRegByte(reg, val);
					}
				} break;

				// SET b,r
				case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7: case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
				case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7: case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
				case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7: case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF:
				case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF7: case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:
				{
					byte bit = (cbOpcode >> 3) & 0x07;					
					byte reg = cbOpcode % 0x8;

					if (reg == 0x6)
					{
						byte val = mem_.readByteAt(getRegWord(REG_HL_INDEX));
						setBit(bit, val);
						mem_.writeByteAt(getRegWord(REG_HL_INDEX), val);
					}
					else if (reg == 0x7)
					{
						byte val = getRegAByte();
						setBit(bit, val);
						setRegAByte(val);
					}
					else
					{
						byte val = getRegByte(reg);
						setBit(bit, val);
						setRegByte(reg, val);
					}
				} break;

				default:
					std::stringstream unhandledOpcode;
					unhandledOpcode << "Unhandled opcode: " << getHexByte(opcode) << " " << getHexByte(cbOpcode) << "\n";
					log(LogType::ERROR, unhandledOpcode.str().c_str());
			}
		} break;

		default:
			std::stringstream unhandledOpcode; 
			unhandledOpcode << "Unhandled opcode: " << getHexByte(opcode) << "\n";
			log(LogType::ERROR, unhandledOpcode.str().c_str());
	}

	if (shouldDumpState_)
		printState();
	
	/*
	s << getHexByte(opcode);
	s << ": ";
	s << getHexWord(registersPC_) << " ";
	s << getHexWord(registersAF_) << " ";
	s << getHexWord(getRegWord(REG_BC_INDEX)) << " ";
	s << getHexWord(getRegWord(REG_DE_INDEX)) << " ";
	s << getHexWord(getRegWord(REG_HL_INDEX)) << " ";
	s << getHexWord(getRegWord(REG_SP_INDEX)) << "\n";
	*/
	return clockCycles;
}

void CPU::addhln(const word w)
{
	word hlW = getRegWord(REG_HL_INDEX);
	word bot12hl = hlW & 0xFFF;
	word bot12w  = w & 0xFFF;

	unsigned int halfCarryTest = bot12hl + bot12w;
	if (halfCarryTest > 0xFFF)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	unsigned int fullCarryTest = hlW + w;
	if (fullCarryTest > 0xFFFF)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();

	setRegWord(REG_HL_INDEX,fullCarryTest & 0xFFFF);
}

void CPU::testBit(const byte bit, const byte val)
{
	if (((val >> bit) & 0x1) != 0x1) 
		SET_Z_FLAG();
	else                 
		RESET_Z_FLAG();
	
	RESET_N_FLAG();
	SET_H_FLAG();
}

void CPU::setBit(const byte bit, byte& val)
{
	val |= (1 << bit);
}

void CPU::resetBit(const byte bit, byte& val)
{
	val &= (~(1 << bit));
}

void CPU::jrccn(const byte cond, const sbyte offset)
{
	if (cond) registersPC_ += offset;
}

void CPU::jpccn(const byte cond, const word address)
{
	if (cond) registersPC_ = address;
}

void CPU::xoran(const byte n)
{
	byte val = getRegAByte();
	byte res = val ^ n;
	
	RESET_Z_FLAG();
	RESET_N_FLAG();
	RESET_H_FLAG();
	RESET_C_FLAG();

	if (res == 0) SET_Z_FLAG();
	setRegAByte(res);
}

void CPU::oran(const byte n)
{
	byte val = getRegAByte();
	byte res = val | n;

	RESET_Z_FLAG();
	RESET_N_FLAG();
	RESET_H_FLAG();
	RESET_C_FLAG();

	if (res == 0) SET_Z_FLAG();
	setRegAByte(res);
}

void CPU::andan(const byte n)
{
	byte val = getRegAByte();
	byte res = val & n;

	RESET_Z_FLAG();
	RESET_N_FLAG();
	SET_H_FLAG();
	RESET_C_FLAG();

	if (res == 0) SET_Z_FLAG();
	setRegAByte(res);
}

void CPU::incn(const byte regIndex)
{
	byte regVal = getRegByte(regIndex);
	bool isBit3SetBefore = (regVal & 0x08) == 0x08;
	regVal++;
	bool isBit3SetAfter = (regVal & 0x08) == 0x08;

	if (regVal == 0x0)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();
	
	RESET_N_FLAG();

	if (isBit3SetBefore && !isBit3SetAfter)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	setRegByte(regIndex, regVal);
}

void CPU::decn(const byte regIndex)
{
	byte val = getRegByte(regIndex);
	byte calc = (val -1);

	SET_N_FLAG();
	
	if (calc == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if (((calc ^ 0x01 ^ val) & 0x10) == 0x10)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	setRegByte(regIndex, calc);
}

void CPU::cpa(const byte val)
{
	byte aVal = getRegAByte();
	byte calc = (aVal - val);

	SET_N_FLAG();

	if (calc == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if ((aVal & 0x0F) < (val & 0x0F))
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	if (aVal < val)
		SET_C_FLAG();
	else
		RESET_C_FLAG();
}

void CPU::rln(const byte regIndex)
{
	byte val = getRegByte(regIndex);
	
	bool oldCFlag = IS_C_FLAG_SET();

	if (val >> 7 == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();
	
	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;

	if (oldCFlag) val |= 0x1;

	if (val == 0x0) SET_Z_FLAG();
	else RESET_Z_FLAG();

	setRegByte(regIndex, val);
}

void CPU::rrn(const byte regIndex)
{
	byte val = getRegByte(regIndex);

	bool oldCFlag = IS_C_FLAG_SET();

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;

	if (oldCFlag) val |= 0x80;

	if (val == 0x0) SET_Z_FLAG();
	else RESET_Z_FLAG();

	setRegByte(regIndex, val);
}

void CPU::sln(const byte regIndex)
{
	byte val = getRegByte(regIndex);

	if ((val >> 7) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegByte(regIndex, val);
}


void CPU::srn(const byte regIndex)
{
	byte val = getRegByte(regIndex);

	if ((val & 0x1) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val = (val >> 1) | ( val & 0x80);
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegByte(regIndex, val);
}

void CPU::srln(const byte regIndex)
{
	byte val = getRegByte(regIndex);

	if ((val & 0x1) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegByte(regIndex, val);
}

void CPU::addspn()
{
	word spVal = getRegWord(REG_SP_INDEX);
	sbyte val = readSByteAtPC();
	word result = (spVal + val);

	RESET_Z_FLAG();
	RESET_N_FLAG();

	if ((result & 0xF) < (spVal & 0xF)) SET_H_FLAG();
	else RESET_H_FLAG();

	if ((result & 0xFF) < (spVal & 0xFF)) SET_C_FLAG();
	else RESET_C_FLAG();

	setRegWord(REG_SP_INDEX, result);
}

void CPU::ldhlspn()
{
	word spVal = getRegWord(REG_SP_INDEX);
	sbyte val = readSByteAtPC();
	word result = spVal + val;
	word check = spVal ^ val ^ ((spVal + val) & 0xFFFF);

	if ((check & 0x100) == 0x100) SET_C_FLAG();
	else RESET_C_FLAG();

	if ((check & 0x10) == 0x10) SET_H_FLAG();
	else RESET_H_FLAG();
		
	RESET_Z_FLAG();
	RESET_N_FLAG();


	setRegWord(REG_HL_INDEX, result);
}

void CPU::rla()
{
	byte val = getRegAByte();

	bool oldCFlag = IS_C_FLAG_SET();

	if (val >> 7 == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;

	if (oldCFlag) val |= 0x1;

	if (val == 0x0) SET_Z_FLAG();
	else RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::rlca()
{
	byte val = getRegAByte();

	if (val >> 7 == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_Z_FLAG();
	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;

	if (IS_C_FLAG_SET()) val |= 0x1;

	setRegAByte(val);
}

void CPU::rra()
{
	byte val = getRegAByte();

	bool oldCFlag = IS_C_FLAG_SET();

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;

	if (oldCFlag) val |= 0x80;

	if (val == 0x0) SET_Z_FLAG();
	else RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::rrca()
{
	byte val = getRegAByte();

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_Z_FLAG();
	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;

	if (IS_C_FLAG_SET()) val |= 0x80;

	setRegAByte(val);
}



void CPU::rlhl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);

	bool oldCFlag = IS_C_FLAG_SET();

	if ((val >> 7) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;

	if (oldCFlag) val |= 0x1;

	if (val == 0x0) SET_Z_FLAG();
	else RESET_Z_FLAG();

	mem_.writeByteAt(hlAddress, val);
}

void CPU::rrhl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);

	bool oldCFlag = IS_C_FLAG_SET();

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;

	if (oldCFlag) val |= 0x80;

	if (val == 0x0) SET_Z_FLAG();
	else RESET_Z_FLAG();

	mem_.writeByteAt(hlAddress, val);
}

void CPU::sla()
{
	byte val = getRegAByte();

	if ((val >> 7) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::sra()
{
	byte val = getRegAByte();

	if ((val & 0x1) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val = (val >> 1) | (val & 0x80);
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::slhl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);

	if ((val >> 7) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	val <<= 1;
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	mem_.writeByteAt(hlAddress, val);
}

void CPU::srhl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);

	if ((val & 0x1) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val = (val >> 1) | (val & 0x80);
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	mem_.writeByteAt(hlAddress, val);
}

void CPU::srla()
{
	byte val = getRegAByte();

	if ((val & 0x1) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::srlhl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);

	if ((val & 0x1) == 0x1)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;
	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	mem_.writeByteAt(hlAddress, val);
}

void CPU::pushnn(const byte regIndex)
{
	word spAddress = getRegWord(REG_SP_INDEX);
	setRegWord(REG_SP_INDEX, spAddress - 2);
	mem_.writeWordAt(spAddress - 2, getRegWord(regIndex));
}

void CPU::popnn(const byte regIndex)
{
	word spAddress = getRegWord(REG_SP_INDEX);
	setRegWord(regIndex, mem_.readWordAt(spAddress));
	setRegWord(REG_SP_INDEX, spAddress + 2);
}

void CPU::addan(const byte val)
{
	byte aVal = getRegAByte();
	byte calc = aVal + val;

	RESET_N_FLAG();
	if (calc == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if (((calc ^ aVal ^ val) & 0x10) == 0x10)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	if (calc < aVal)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	setRegAByte(static_cast<byte>(calc));
}

void CPU::suban(const byte val)
{
	byte aVal = getRegAByte();
	byte calc = (aVal - val);

	SET_N_FLAG();

	if (calc == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if ((aVal & 0x0F) < (val & 0x0F))
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	if ((aVal & 0xFF) < (val & 0xFF))
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	setRegAByte(calc);
}

void CPU::adcan(const byte val)
{
	byte regAVal = getRegAByte();
	byte c = IS_C_FLAG_SET() ? 0x01 : 0x00;

	RESET_N_FLAG();

	if (((int)(regAVal & 0x0F) + (int)(val & 0x0F) + (int)c) > 0x0F)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	if (((int)(regAVal & 0xFF) + (int)(val & 0xFF) + (int)c) > 0xFF)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	byte res = regAVal + val + c;
	setRegAByte(res);
	if (res == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();
}

void CPU::sbcan(const byte val)
{
	int un = (int)val & 0xFF;
	int tmpA = (int)getRegAByte() & 0xFF;
	int ua = tmpA;

	ua -= un;

	if (IS_C_FLAG_SET()) ua -= 1;

	SET_N_FLAG();
	if (ua < 0)
		SET_C_FLAG();
	else
		RESET_C_FLAG();

	ua &= 0xFF;

	if (ua == 0)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if (((ua ^ un ^ tmpA) & 0x10) == 0x10)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	setRegAByte(static_cast<byte>(ua));
}

void CPU::swapa()
{
	byte val = getRegAByte();
	byte res = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);

	if (res == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();
	RESET_C_FLAG();

	setRegAByte(res);
}

void CPU::swapn(const byte regIndex)
{
	byte val = getRegByte(regIndex);
	byte res = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);

	if (res == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();
	RESET_C_FLAG();

	setRegByte(regIndex, res);
}

void CPU::swaphl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);
	byte res = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);

	if (res == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();
	RESET_C_FLAG();

	mem_.writeByteAt(hlAddress, res);
}

void CPU::pushaf()
{
	word spAddress = getRegWord(REG_SP_INDEX);
	setRegWord(REG_SP_INDEX, spAddress - 2);
	mem_.writeWordAt(spAddress - 2, getRegAByte() << 8 | getRegFByte());
}

void CPU::popaf()
{
	word spAddress = getRegWord(REG_SP_INDEX);
	word val = mem_.readWordAt(spAddress);
	registersAF_ = (val & 0xFFF0);
	setRegWord(REG_SP_INDEX, spAddress + 2);
}

void CPU::inca()
{
	byte regVal = getRegAByte();
	bool isBit3SetBefore = (regVal & 0x08) == 0x08;
	regVal++;
	bool isBit3SetAfter = (regVal & 0x08) == 0x08;

	if (regVal == 0x0)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	RESET_N_FLAG();

	if (isBit3SetBefore && !isBit3SetAfter)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	setRegAByte(regVal);
}

void CPU::deca()
{
	byte val = getRegAByte();
	byte calc = (val - 1);

	SET_N_FLAG();

	if (calc == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if (((calc ^ 0x01 ^ val) & 0x10) == 0x10)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	setRegAByte(calc);
}

void CPU::inchl()
{
	word adr = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(adr);

	bool isBit3SetBefore = (val & 0x08) == 0x08;
	val++;
	bool isBit3SetAfter = (val & 0x08) == 0x08;

	if (val == 0x0)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	RESET_N_FLAG();

	if (isBit3SetBefore && !isBit3SetAfter)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	mem_.writeByteAt(adr, val);
}

void CPU::dechl()
{
	word hlAddress = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(hlAddress);
	byte calc = (val - 1);

	SET_N_FLAG();

	if (calc == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	if (((calc ^ 0x01 ^ val) & 0x10) == 0x10)
		SET_H_FLAG();
	else
		RESET_H_FLAG();

	mem_.writeWordAt(hlAddress, calc);
}

void CPU::daa()
{
	int aVal = getRegAByte();

	if (!IS_N_FLAG_SET())
	{
		if (IS_H_FLAG_SET() || (aVal & 0xF) > 9)
		{
			aVal += 0x06;
		}

		if (IS_C_FLAG_SET() || (aVal > 0x9F))
		{
			aVal += 0x60;
		}
	}
	else
	{
		if (IS_H_FLAG_SET())
		{
			aVal = (aVal - 0x06) & 0xFF;
		}

		if (IS_C_FLAG_SET())
		{
			aVal -= 0x60;
		}
	}

	if ((aVal & 0x100) == 0x100)
		SET_C_FLAG();

	RESET_H_FLAG();

	aVal &= 0xFF;

	(aVal == 0x00) ? SET_Z_FLAG() : RESET_Z_FLAG();
	setRegAByte(static_cast<byte>(aVal));
}

void CPU::cpl()
{
	setRegAByte(getRegAByte() ^ 0xFF);
	SET_N_FLAG();
	SET_H_FLAG();
}

void CPU::ldihla()
{
	word hlValue = getRegWord(REG_HL_INDEX);
	mem_.writeByteAt(hlValue, getRegAByte());
	setRegWord(REG_HL_INDEX, hlValue + 1);
}

void CPU::ldiahl()
{
	word hlValue = getRegWord(REG_HL_INDEX);
	setRegAByte(mem_.readByteAt(hlValue));
	setRegWord(REG_HL_INDEX, hlValue + 1);
}

void CPU::lddhla()
{
	word hlValue = getRegWord(REG_HL_INDEX);
	mem_.writeByteAt(hlValue, getRegAByte());
	setRegWord(REG_HL_INDEX, hlValue - 1);
}

void CPU::lddahl()
{
	word hlValue = getRegWord(REG_HL_INDEX);
	setRegAByte(mem_.readByteAt(hlValue));
	setRegWord(REG_HL_INDEX, hlValue - 1);
}


void CPU::ret()
{
	word spAddress = getRegWord(REG_SP_INDEX);
	registersPC_ = mem_.readWordAt(spAddress);
	setRegWord(REG_SP_INDEX, spAddress + 2);
}

void CPU::callnn(word address)
{
	word spAddress = getRegWord(REG_SP_INDEX);
	setRegWord(REG_SP_INDEX, spAddress - 2);
	mem_.writeWordAt(spAddress - 2, registersPC_);
	registersPC_ = address;
}

void CPU::cbrlca()
{
	byte val = getRegAByte();

	if (val >> 7 == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;
	if (IS_C_FLAG_SET()) val |= 0x1;

	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::cbrrca()
{
	byte val = getRegAByte();

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;
	if (IS_C_FLAG_SET()) val |= 0x80;

	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegAByte(val);
}

void CPU::cbrlcn(const byte regIndex)
{
	byte val = getRegByte(regIndex);

	if (val >> 7 == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;
	if (IS_C_FLAG_SET()) val |= 0x1;

	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegByte(regIndex, val);
}

void CPU::cbrrcn(const byte regIndex)
{
	byte val = getRegByte(regIndex);

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;
	if (IS_C_FLAG_SET()) val |= 0x80;

	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	setRegByte(regIndex, val);
}

void CPU::cbrlchl()
{
	word address = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(address);

	if ((val >> 7) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val <<= 1;
	if (IS_C_FLAG_SET()) val |= 0x1;

	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	mem_.writeByteAt(address, val);
}

void CPU::cbrrchl()
{
	word address = getRegWord(REG_HL_INDEX);
	byte val = mem_.readByteAt(address);

	if ((val & 0x01) == 0x1) SET_C_FLAG();
	else RESET_C_FLAG();

	RESET_N_FLAG();
	RESET_H_FLAG();

	val >>= 1;
	if (IS_C_FLAG_SET()) val |= 0x80;

	if (val == 0x00)
		SET_Z_FLAG();
	else
		RESET_Z_FLAG();

	mem_.writeByteAt(address, val);
}

unsigned int CPU::handleInterrupts()
{
	if (ime_)
	{
		byte interruptFlagRegister = mem_.readByteAt(Memory::IF_ADDRESS);
		byte interruptEnabledRegister = mem_.readByteAt(Memory::IE_ADDRESS);
		byte maskedIFIE = (interruptFlagRegister & 0x1F) & (interruptEnabledRegister & 0x1F); // we only care about the 5 bottom bits
		if (maskedIFIE != 0x00)
		{
			isHalted_ = false;

			word spAddress = getRegWord(REG_SP_INDEX);
			setRegWord(REG_SP_INDEX, spAddress - 2); // Dec SP
			mem_.writeWordAt(spAddress - 2, registersPC_);    // Push PC to stack
			ime_ = false;                                       // Prevent cascading interrupts 

			if ((maskedIFIE >> VBLANK_INTERRUPT_BIT) == 0x1)
			{
				mem_.writeByteAt(Memory::IF_ADDRESS, interruptFlagRegister & VBLANK_INTERRUPT_NEGATED_MASK); // disable VBlank bit on IF
				registersPC_ = VBLANK_INTERRUPT_HANDLER_ADDRESS;
			}
			else if ((maskedIFIE >> LCD_STAT_INTERRUPT_BIT) == 0x1)
			{
				mem_.writeByteAt(Memory::IF_ADDRESS, interruptFlagRegister & LCD_STAT_INTERRUPT_NEGATED_MASK);
				registersPC_ = LCD_STAT_INTERRUPT_HANDLER_ADDRESS;
			}
			else if ((maskedIFIE >> TIMER_INTERRUPT_BIT) == 0x1)
			{
				mem_.writeByteAt(Memory::IF_ADDRESS, interruptFlagRegister & TIMER_INTERRUPT_NEGATED_MASK);
				registersPC_ = TIMER_INTERRUPT_HANDLER_ADDRESS;
			}
			else if ((maskedIFIE >> SERIAL_INTERRUPT_BIT) == 0x1)
			{
				mem_.writeByteAt(Memory::IF_ADDRESS, interruptFlagRegister & SERIAL_INTERRUPT_NEGATED_MASK);
				registersPC_ = SERIAL_INTERRUPT_HANDLER_ADDRESS;
			}
			else
			{
				mem_.writeByteAt(Memory::IF_ADDRESS, interruptFlagRegister & JOYPAD_INTERRUPT_NEGATED_MASK);
				registersPC_ = JOYPAD_INTERRUPT_HANDLER_ADDRESS;
			}
		}
		return ISR_EXECUTION_CLOCK_CYCLES;
	}

	// This needs to happen effectively at the end of the system's entire update 
	// (the interrupt check is the last function call on the update)
	// because ei is delayed by one instruction.
	if (eiTriggered_)
	{
		ime_ = true;
		eiTriggered_ = false;
	}

	return 0;
}

void CPU::triggerInterrupt(const byte interruptBit)
{
	byte ifRegister = mem_.readByteAt(Memory::IF_ADDRESS);
	mem_.writeByteAt(Memory::IF_ADDRESS, ifRegister | (0x1 << interruptBit));
	isHalted_ = false;
}

void CPU::printState() const
{
	std::stringstream state;
	state << "\n=============== state post:";
	
	for (const byte b: currentInstructionOperands_)
	{
		state << getHexByte(b) << " ";
	}

	state << " =============\n";
	const auto sizeofHeader = state.str().size();
	state << "af: " << getHexWord(registersAF_) << "\n";
	state << "bc: " << getHexWord(getRegWord(REG_BC_INDEX)) << "\n";
	state << "de: " << getHexWord(getRegWord(REG_DE_INDEX)) << "\n";
	state << "hl: " << getHexWord(getRegWord(REG_HL_INDEX)) << "\n";
	state << "sp: " << getHexWord(getRegWord(REG_SP_INDEX)) << "\n";
	state << "pc: " << getHexWord(registersPC_) << "\n";
	for (unsigned int i = 0U; i < sizeofHeader - 2; ++i) state << "=";
	state << "\n";
	log(LogType::INFO, state.str().c_str());
}