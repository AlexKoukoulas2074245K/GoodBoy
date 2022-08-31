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
		MBC1_RAM_BATTERY = 0x3,
		MBC3_RAM_BATTERY = 0x13,
		MBC5_RAM_BATTERY = 0x1b,
		UNSUPPORTED = 0x1c,
	};

public:
	Cartridge();
	~Cartridge();

	std::string loadCartridge(const char* filepath);
	void unloadCartridge();
	
	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);

private:
	void setSaveFilename(const char* filepath);
	void flushExternalRamToFile();

private:
	byte* cartridgeRom_;
	byte* cartridgeExternalRam_;
	std::string cartridgeName_;
	std::string saveFileName_;	
	CartridgeType cartridgeType_;
	int cartridgeROMSizeInKB_;
	int cartridgeExternalRAMSizeInKB_;
	byte romBankNumberRegister_;
	byte ramBankNumberRegister_;
	byte secondaryBankNumberRegister_;
	byte bankingMode_;
	bool hasRTC_;
	bool externalRamEnabled_;
};

#endif