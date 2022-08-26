#define _CRT_SECURE_NO_WARNINGS

#include "cartridge.h"
#include "logging.h"

#include <stdio.h>

static constexpr word CARTRIDGE_TITLE_LENGTH  = 0x10;
static constexpr word CARTRIDGE_TITLE_ADDRESS = 0x0134;
static constexpr word CARTRIDGE_TYPE_ADDRESS  = 0x0147;

Cartridge::Cartridge()
	: cartridgeRom_(nullptr)
{
}

Cartridge::~Cartridge()
{
	unloadCartridge();
}

std::string Cartridge::loadCartridge(const char* filename)
{
	log(LogType::INFO, "Loading %s", filename);

	FILE* file = fopen(filename, "rb");

	// get file size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// copy cartridge over
	cartridgeRom_ = new byte[size];
	fread(cartridgeRom_, sizeof(byte), size, file);
	
	std::string cartridgeName(&cartridgeRom_[CARTRIDGE_TITLE_ADDRESS], &cartridgeRom_[CARTRIDGE_TITLE_ADDRESS] + CARTRIDGE_TITLE_LENGTH);
	for (int i = 0; i < cartridgeName.length(); ++i)
	{
		if (cartridgeName[i] == '\0') break;
		cartridgeName_ += cartridgeName[i];
	}
	cartridgeType_ = cartridgeRom_[CARTRIDGE_TYPE_ADDRESS];

	switch (cartridgeType_)
	{
		case 0x00: cartridgeName_ += " (ROM ONLY)"; break;
		case 0x01: cartridgeName_ += " (MBC1)"; break;
		case 0x02: cartridgeName_ += " (MBC1+RAM)"; break;
		case 0x03: cartridgeName_ += " (MBC1+RAM+BATTERY)"; break;
		case 0x05: cartridgeName_ += " (MBC2)"; break;
		case 0x06: cartridgeName_ += " (MBC2+BATTERY)"; break;
		case 0x08: cartridgeName_ += " (ROM+RAM)"; break;
		case 0x09: cartridgeName_ += " (ROM+RAM+BATTERY)"; break;
		case 0x0B: cartridgeName_ += " (MMM01)"; break;
		case 0x0C: cartridgeName_ += " (MMM01+RAM)"; break;
		case 0x0D: cartridgeName_ += " (MMM01+RAM+BATTERY)"; break;
		case 0x0F: cartridgeName_ += " (MBC3+TIMER+BATTERY)"; break;
		case 0x10: cartridgeName_ += " (MBC3+TIMER+RAM+BATTERY)"; break;
		case 0x11: cartridgeName_ += " (MBC3)"; break;
		case 0x12: cartridgeName_ += " (MBC3+RAM)"; break;
		case 0x13: cartridgeName_ += " (MBC3+RAM+BATTERY)"; break;
		case 0x19: cartridgeName_ += " (MBC5)"; break;
		case 0x1A: cartridgeName_ += " (MBC5+RAM)"; break;
		case 0x1B: cartridgeName_ += " (MBC5+RAM+BATTERY)"; break;
		case 0x1C: cartridgeName_ += " (MBC5+RUMBLE)"; break;
		case 0x1D: cartridgeName_ += " (MBC5+RUMBLE+RAM)"; break;
		case 0x1E: cartridgeName_ += " (MBC5+RUMBLE+RAM+BATTERY)"; break;
		case 0x20: cartridgeName_ += " (MBC6)"; break;
		case 0x22: cartridgeName_ += " (MBC7+SENSOR+RUMBLE+RAM+BATTERY)"; break;
		case 0xFC: cartridgeName_ += " (POCKET CAMERA)"; break;
		case 0xFD: cartridgeName_ += " (BANDAI TAMA5)"; break;
		case 0xFE: cartridgeName_ += " (HuC3)"; break;
		case 0xFF: cartridgeName_ += " (HuC1+RAM+BATTERY)"; break;
		default:
			log(LogType::WARNING, "Unknown cartridge type");
	}

	return cartridgeName_;
}

void Cartridge::unloadCartridge()
{
	delete cartridgeRom_;
}

byte Cartridge::readByteAt(const word address) const
{
	return cartridgeRom_[address];
}

void Cartridge::writeByteAt(const word address, const byte b)
{
	log(LogType::WARNING, "Writing at rom address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
	cartridgeRom_[address] = b;
}
