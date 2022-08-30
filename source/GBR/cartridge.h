#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "types.h"

#include <string>

class Cartridge final
{
public:
	enum class CartridgeType
	{
		ROM_ONLY = 0x0,
		MBC1 = 0x1,
		//MBC1_RAM = 0x2,
		//MBC1_RAM_BATTERY = 0x3,
		MBC3_RAM_BATTERY = 0x13,
		UNSUPPORTED = 0x14,
	};

public:
	Cartridge();
	~Cartridge();

	std::string loadCartridge(const char* filename);
	void unloadCartridge();
	
	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);

private:
	byte* cartridgeRom_;
	byte* cartridgeExternalRam_;
	std::string cartridgeName_;
	byte rtcRegisters_[5];
	CartridgeType cartridgeType_;
	int cartridgeROMSizeInKB_;
	int cartridgeExternalRAMSizeInKB_;
	byte romBankNumberRegister_;
	byte ramBankNumberRegister_;
	byte secondaryBankNumberRegister_;
	byte bankingMode_;
	bool externalRamEnabled_;
};

#endif