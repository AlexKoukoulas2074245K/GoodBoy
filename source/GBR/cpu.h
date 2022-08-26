#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "types.h"

#include <vector>

class Memory;
class CPU final
{
public:
	static constexpr byte VBLANK_INTERRUPT_BIT   = 0;
	static constexpr byte LCD_STAT_INTERRUPT_BIT = 1;
	static constexpr byte TIMER_INTERRUPT_BIT    = 2;
	static constexpr byte SERIAL_INTERRUPT_BIT   = 3;
	static constexpr byte JOYPAD_INTERRUPT_BIT   = 4;

public:
	CPU(Memory& mem);
	~CPU();

	unsigned int executeNextInstruction();
	unsigned int handleInterrupts();
	
	void triggerInterrupt(const byte interruptBit);

private:
	inline void setRegAByte(const byte val) { registersAF_ = (val << 8) | (registersAF_ & 0x00FF);  }
	inline void setRegByte(const byte regIndex, const byte val) { generalPurposeRegisters_[regIndex] = val; }
	inline void setRegWord(const byte regIndex, const word val) { generalPurposeRegisters_[regIndex] = (val & 0xFF00) >> 8; generalPurposeRegisters_[regIndex + 1] = val & 0x00FF; }
	
	inline byte getRegAByte() const { return registersAF_ >> 8; }
	inline byte getRegFByte() const { return registersAF_ & 0x00FF; }
	inline byte getRegByte(const byte regIndex) const { return generalPurposeRegisters_[regIndex]; }
	inline word getRegWord(const byte regIndex) const { return generalPurposeRegisters_[regIndex] << 8 | generalPurposeRegisters_[regIndex + 1]; }

	inline byte readByteAtPC() 
	{
		byte b = mem_.readByteAt(registersPC_++);
		currentInstructionOperands_.emplace_back(b);
		return b; 
	}
	inline sbyte readSByteAtPC()
	{
		sbyte b = mem_.readSByteAt(registersPC_++);
		currentInstructionOperands_.emplace_back(b);
		return b;
	}
	inline word readWordAtPc()
	{
		word w = mem_.readWordAt(registersPC_);
		registersPC_ += 2;
		currentInstructionOperands_.emplace_back(w & 0xFF);
		currentInstructionOperands_.emplace_back(w >> 8);
		return w;
	}

	void addhln(const word w);
	void testBit(const byte bit, const byte val);
	void setBit(const byte bit, byte& val);
	void resetBit(const byte bit, byte& val);
	void jrccn(const byte cond, const sbyte offset);
	void jpccn(const byte cond, const word address);
	void xoran(const byte n);
	void oran(const byte n);
	void andan(const byte n);
	void incn(const byte regIndex);
	void decn(const byte regIndex);
	void cpa(const byte val);
	void rln(const byte regIndex);
	void rrn(const byte regIndex);
	void sln(const byte regIndex);
	void srn(const byte regIndex);
	void srln(const byte regIndex);
	void addspn();
	void ldhlspn();
	void rla();
	void rra();
	void rlca();
	void rrca();
	void rlhl();
	void rrhl();
	void sla();
	void sra();
	void slhl();
	void srhl();
	void srla();
	void srlhl();
	void pushnn(const byte regIndex);
	void popnn(const byte regIndex);
	void addan(const byte val);
	void suban(const byte val);
	void adcan(const byte val);
	void sbcan(const byte val);
	void swapa();
	void swapn(const byte regIndex);
	void swaphl();
	void pushaf();
	void popaf();
	void inca();
	void deca();
	void inchl();
	void dechl();
	void daa();
	void cpl();
	void ldihla();
	void ldiahl();
	void lddhla();
	void lddahl();
	void ret();
	void callnn(word address);
	void cbrlca();
	void cbrrca();
	void cbrlcn(const byte regIndex);
	void cbrrcn(const byte regIndex);
	void cbrlchl();
	void cbrrchl();
	void printState() const;
private:
	static constexpr byte REG_B_INDEX = 0;
	static constexpr byte REG_C_INDEX = 1;
	static constexpr byte REG_D_INDEX = 2;
	static constexpr byte REG_E_INDEX = 3;
	static constexpr byte REG_H_INDEX = 4;
	static constexpr byte REG_L_INDEX = 5;
	static constexpr byte REG_S_INDEX = 6;
	static constexpr byte REG_P_INDEX = 7;
	
	static constexpr byte REG_BC_INDEX = 0;
	static constexpr byte REG_DE_INDEX = 2;
	static constexpr byte REG_HL_INDEX = 4;
	static constexpr byte REG_SP_INDEX = 6;

	Memory& mem_;
	word registersAF_;
	byte generalPurposeRegisters_[sizeof(word) * 4];  // BC, DE, HL, SP
	std::vector<byte> currentInstructionOperands_;
	word registersPC_;
	bool isHalted_;
	bool shouldDumpState_;
	bool ime_;
	bool eiTriggered_; // ei is delayed by one instruction so we can't allow interrupts for the entire system's step
};

#endif