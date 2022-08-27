#define _CRT_SECURE_NO_WARNINGS

#include "cartridge.h"
#include "logging.h"

#include <cassert>
#include <stdio.h>

static constexpr word CARTRIDGE_TITLE_LENGTH  = 0x10;
static constexpr word CARTRIDGE_TITLE_ADDRESS = 0x0134;
static constexpr word CARTRIDGE_TYPE_ADDRESS  = 0x0147;
static constexpr word CARTRIDGE_ROM_SIZE_ADDRESS = 0x0148;
static constexpr word CARTRIDGE_RAM_SIZE_ADDRESS = 0x0149;

static const std::string CARTRIDGE_TYPE_NAMES[] =
{
	"(ROM ONLY)", "(MBC1)", "(MBC1 + RAM)", "(MBC1+RAM+BATTERY)"
};

static const word CARTRIDGE_RAM_SIZES[] =
{
	0, 0, 8, 32, 128, 64
};

Cartridge::Cartridge()
	: cartridgeRom_(nullptr)
	, cartridgeName_()
	, cartridgeType_(CartridgeType::UNSUPPORTED)
	, cartridgeROMSizeInKB_(0)
	, cartridgeRAMSizeInKB_(0)
	, romBankNumberRegister_(0x1)
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
	for (unsigned int i = 0U; i < cartridgeName.length(); ++i)
	{
		if (cartridgeName[i] == '\0') break;
		cartridgeName_ += cartridgeName[i];
	}

	byte type = cartridgeRom_[CARTRIDGE_TYPE_ADDRESS];
	if (type >= static_cast<byte>(CartridgeType::UNSUPPORTED))
	{
		return " UNSUPPORTED";
	}

	cartridgeType_ = static_cast<CartridgeType>(type);
	cartridgeName_ = CARTRIDGE_TYPE_NAMES[type];
	cartridgeROMSizeInKB_ = 32 * (1 << cartridgeRom_[CARTRIDGE_ROM_SIZE_ADDRESS]);
	cartridgeRAMSizeInKB_ = CARTRIDGE_RAM_SIZES[cartridgeRom_[CARTRIDGE_RAM_SIZE_ADDRESS]];

	return " " + cartridgeName_;
}

void Cartridge::unloadCartridge()
{
	delete cartridgeRom_;
}

byte Cartridge::readByteAt(const word address) const
{
	switch (cartridgeType_)
	{
		case CartridgeType::ROM_ONLY:
		{
			return cartridgeRom_[address];
		} break;

		case CartridgeType::MBC1:
		{
			if (address <= 0x3FFF)
			{
				return cartridgeRom_[address];
			}
			else
			{
				return cartridgeRom_[(address - 0x4000) + (romBankNumberRegister_ * 0x4000)];
			}
		}
	}
	return 0xFF;
}

void Cartridge::writeByteAt(const word address, const byte b)
{
	switch (cartridgeType_)
	{
		case CartridgeType::ROM_ONLY:
		{
			log(LogType::WARNING, "Writing at rom address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
		} break;

		case CartridgeType::MBC1:
		{
			if (address <= 0x1FFF)
			{
				log(LogType::WARNING, "RAM ENABLE written on an MBC1 with 0 RAM", getHexWord(address).c_str(), getHexByte(b).c_str());
			}
			else if (address <= 0x3FFF)
			{
				// This 5 - bit register (range $01 - $1F) selects the ROM bank number for the 4000 - 7FFF region.
				// Higher bits are discarded.
				romBankNumberRegister_ = b & 0x1F;
				
				assert(romBankNumberRegister_ * 16 < cartridgeROMSizeInKB_);

				// If this register is set to 0x00, it behaves as if it is set to 0x01
				if ((b & 0x1F)== 0x0) romBankNumberRegister_ = 0x1;
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
			}
		} break;
	}
}
