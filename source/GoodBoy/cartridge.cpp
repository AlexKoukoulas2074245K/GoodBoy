#define _CRT_SECURE_NO_WARNINGS

#include "cartridge.h"
#include "logging.h"

#include <cassert>
#include <stdio.h>

static constexpr word CARTRIDGE_TITLE_LENGTH  = 0x10;
static constexpr word CARTRIDGE_TITLE_ADDRESS = 0x0134;
static constexpr word CARTRIDGE_CGB_ADDRESS = 0x0143;
static constexpr word CARTRIDGE_TYPE_ADDRESS  = 0x0147;
static constexpr word CARTRIDGE_ROM_SIZE_ADDRESS = 0x0148;
static constexpr word CARTRIDGE_RAM_SIZE_ADDRESS = 0x0149;

static const std::string CARTRIDGE_TYPE_NAMES[] =
{
	"(ROM ONLY)", "(MBC1)", "(MBC1 + RAM)", "(MBC1+RAM+BATTERY)", "", "(MBC2)",
	"(MBC2+BATTERY)", "", "(ROM+RAM)", "(ROM+RAM+BATTERY)", "", "(MMM01)", "(MMM01+RAM)", "(MMM01+RAM+BATTERY)",
	"", "(MBC3+TIMER+BATTERY)", "(MBC3+TIMER+RAM+BATTERY)", "(MBC3)", "(MBC3+RAM)", "(MBC3+RAM+BATTERY)", "",
	"", "", "", "", "(MBC5)", "(MBC5+RAM)", "(MBC5+RAM+BATTERY)", "(MBC5+RUMBLE)", "(MBC5+RUMBLE+RAM)", "(MBC5+RUMBLE+RAM+BATTERY)"
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
	, cartridgeExternalRAMSizeInKB_(0)
	, romBankNumberRegister_(0x1)
	, ramBankNumberRegister_(0x0)
	, secondaryBankNumberRegister_(0x0)
	, bankingMode_(0x0)
	, cgbType_(CgbType::DMG)
	, externalRamEnabled_(false)
{
}

Cartridge::~Cartridge()
{
	unloadCartridge();
}

std::string Cartridge::loadCartridge(const char* filepath)
{
	log(LogType::INFO, "Loading %s", filepath);
	
	readCartridgeRom(filepath);
	setSaveFilename(filepath);
	setCartridgeAttributes();
	setCartridgeExternalRam();
	
	return cartridgeName_ + " " + CARTRIDGE_TYPE_NAMES[static_cast<int>(cartridgeType_)];
}

void Cartridge::unloadCartridge()
{
	flushExternalRamToFile();
	delete cartridgeRom_;
	delete cartridgeExternalRam_;
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
		case CartridgeType::MBC1_RAM:
		case CartridgeType::MBC1_RAM_BATTERY:
		{
			if (address <= 0x3FFF)
			{
				return cartridgeRom_[address];
			}
			else if (address <= 0x7FFF)
			{
				byte romBankTarget = romBankNumberRegister_;
				if (bankingMode_ == 0)
					romBankTarget |= (ramBankNumberRegister_ << 4);

				return cartridgeRom_[(address - 0x4000) + (romBankTarget * 0x4000)];
			}
			else if (address >= 0xA000 && address <= 0xBFFF)
			{
				if (externalRamEnabled_)
				{
					if (bankingMode_ == 0)
						return cartridgeExternalRam_[(address - 0xA000)];
					else
						return cartridgeExternalRam_[(address - 0xA000) + ramBankNumberRegister_ * 0x2000];
				}
				else
				{
					log(LogType::WARNING, "Reading from external RAM but not enabled %s byte. Returning garbage", getHexWord(address).c_str());
					return 0xFF;
				}
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom read address %s", getHexWord(address).c_str());
			}
		} break;

		case CartridgeType::MBC3_RAM_BATTERY:
		case CartridgeType::MBC3_TIMER_RAM_BATTERY:
		{
			if (address <= 0x3FFF)
			{
				return cartridgeRom_[address];
			}
			else if (address <= 0x7FFF)
			{
				return cartridgeRom_[(address - 0x4000) + (romBankNumberRegister_ * 0x4000)];
			}
			else if (address >= 0xA000 && address <= 0xBFFF)
			{
				if (externalRamEnabled_)
				{
					return cartridgeExternalRam_[(address - 0xA000) + ramBankNumberRegister_ * 0x2000];
				}
				else
				{
					log(LogType::WARNING, "Reading from external RAM but not enabled %s byte. Returning garbage", getHexWord(address).c_str());
					return 0xFF;
				}
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom read address %s", getHexWord(address).c_str());
			}
		} break;

		case CartridgeType::MBC5_RAM_BATTERY:
		case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
		{
			if (address <= 0x3FFF)
			{
				return cartridgeRom_[address];
			}
			else if (address <= 0x7FFF)
			{
				if (secondaryBankNumberRegister_ == 0x1)
					return cartridgeRom_[(address - 0x4000) + (0x100 | romBankNumberRegister_) * 0x4000];
				else
					return cartridgeRom_[(address - 0x4000) + romBankNumberRegister_ * 0x4000];
			}
			else if (address >= 0xA000 && address <= 0xBFFF)
			{
				if (externalRamEnabled_)
				{
					return cartridgeExternalRam_[(address - 0xA000) + ramBankNumberRegister_ * 0x2000];
				}
				else
				{
					log(LogType::WARNING, "Reading from external RAM but not enabled %s byte. Returning garbage", getHexWord(address).c_str());
					return 0xFF;
				}
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom read address %s", getHexWord(address).c_str());
			}
		} break;
            
        default: break;
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
		case CartridgeType::MBC1_RAM:
		case CartridgeType::MBC1_RAM_BATTERY:
		{
			if (address <= 0x1FFF)
			{
				externalRamEnabled_ = (b & 0x0A) == 0x0A;
			}
			else if (address <= 0x3FFF)
			{
				// This 5 - bit register (range $01 - $1F) selects the ROM bank number for the 4000 - 7FFF region.
				// Higher bits are discarded.
				romBankNumberRegister_ = b & 0x1F;
				
				assert(romBankNumberRegister_ * 16 < cartridgeROMSizeInKB_);

				// If this register is set to 0x00, it behaves as if it is set to 0x01
				if (romBankNumberRegister_ == 0x0) romBankNumberRegister_ = 0x1;
			}
			else if (address <= 0x5FFF)
			{
				if (b <= 0x03)
				{
					ramBankNumberRegister_ = b;					
				}
				else
				{
					log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
				}
			}
			else if (address <= 0x7FFF)
			{
				bankingMode_ = b & 0x1;
			}
			else if (address >= 0xA000 && address <= 0xBFFF)
			{
				if (externalRamEnabled_)
				{		
					if (bankingMode_ == 0)
						cartridgeExternalRam_[(address - 0xA000)] = b;
					else
						cartridgeExternalRam_[(address - 0xA000) + ramBankNumberRegister_ * 0x2000] = b;
				}				
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
			}
		} break;

		case CartridgeType::MBC3_RAM_BATTERY:
		case CartridgeType::MBC3_TIMER_RAM_BATTERY:
		{
			if (address <= 0x1FFF)
			{
				externalRamEnabled_ = (b & 0x0A) == 0x0A;										
			}
			else if (address <= 0x3FFF)
			{
				// This 7 - bit register (range $01 - $7F) selects the ROM bank number for the 4000 - 7FFF region.
				// Higher bits are discarded.
				romBankNumberRegister_ = b & 0x7F;

				assert(romBankNumberRegister_ * 16 < cartridgeROMSizeInKB_);

				// If this register is set to 0x00, it behaves as if it is set to 0x01
				if (b  == 0x0) romBankNumberRegister_ = 0x1;
			}
			else if (address <= 0x5FFF)
			{
				if (b <= 0x03)
				{
					ramBankNumberRegister_ = b;				
				}
				else
				{
					log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
				}
			}
			else if (address >= 0xA000 && address <= 0xBFFF)
			{
				if (externalRamEnabled_)
				{					
					cartridgeExternalRam_[(address - 0xA000) + ramBankNumberRegister_ * 0x2000] = b;										
				}
				else
				{
					log(LogType::WARNING, "Writing to external ram address %s byte %s but ERAM not enabled. Ignoring write", getHexWord(address).c_str(), getHexByte(b).c_str());
				}
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
			}
		} break;

		case CartridgeType::MBC5_RAM_BATTERY:
		case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
		{
			if (address <= 0x1FFF)
			{
				externalRamEnabled_ = (b & 0x0A) == 0x0A;
			}
			else if (address <= 0x2FFF)
			{								
				romBankNumberRegister_ = b;				

				// If this register is set to 0x00, it behaves as if it is set to 0x01
				if (b == 0x0) romBankNumberRegister_ = 0x1;
			}
			else if (address <= 0x3FFF)
			{
				// This 1 - bit register (range $01 - $7F) selects the upper bit ROM bank number.
				secondaryBankNumberRegister_ = b & 0x1;
			}
			else if (address <= 0x5FFF)
			{
				if (b <= 0x0F)
				{
					ramBankNumberRegister_ = b;
				}
				else
				{
					log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
				}
			}
			else if (address >= 0xA000 && address <= 0xBFFF)
			{
				if (externalRamEnabled_)
				{
					cartridgeExternalRam_[(address - 0xA000) + ramBankNumberRegister_ * 0x2000] = b;
				}
				else
				{
					log(LogType::WARNING, "Writing to external ram address %s byte %s but ERAM not enabled. Ignoring write", getHexWord(address).c_str(), getHexByte(b).c_str());
				}
			}
			else
			{
				log(LogType::WARNING, "Unhandled rom write address %s byte %s", getHexWord(address).c_str(), getHexByte(b).c_str());
			}
		} break;
            
        default: break;
	}
}

void Cartridge::readCartridgeRom(const char* filepath)
{
	FILE* file = fopen(filepath, "rb");

	// get file size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// copy cartridge over
	cartridgeRom_ = new byte[size];
	fread(cartridgeRom_, sizeof(byte), size, file);
	fclose(file);
}

void Cartridge::setSaveFilename(const char* filepath)
{
	saveFileName_.clear();
	std::string path(filepath);
	assert(path.size() >= 5 && "Filepath too small");

	for (int i = static_cast<int>(path.size()) - 4; i >= 0; --i)
	{		
		saveFileName_ = path[i] + saveFileName_;
	}

	saveFileName_ += "sav";
}

void Cartridge::setCartridgeAttributes()
{
	std::string cartridgeName(&cartridgeRom_[CARTRIDGE_TITLE_ADDRESS], &cartridgeRom_[CARTRIDGE_TITLE_ADDRESS] + CARTRIDGE_TITLE_LENGTH);
	for (unsigned int i = 0U; i < cartridgeName.length(); ++i)
	{
		if (cartridgeName[i] == '\0') break;
		cartridgeName_ += cartridgeName[i];
	}

	// Cartridge Type
	cartridgeType_ = static_cast<CartridgeType>(cartridgeRom_[CARTRIDGE_TYPE_ADDRESS]);

	// Cartridge Rom Size
	cartridgeROMSizeInKB_ = 32 * (1 << cartridgeRom_[CARTRIDGE_ROM_SIZE_ADDRESS]);

	// Cartridge Ram size
	cartridgeExternalRAMSizeInKB_ = CARTRIDGE_RAM_SIZES[cartridgeRom_[CARTRIDGE_RAM_SIZE_ADDRESS]];

	// CGB Type
	switch (cartridgeRom_[CARTRIDGE_CGB_ADDRESS])
	{
		case 0x80: cgbType_ = CgbType::BACKWARDS_COMPATIBLE; break;
		case 0xC0: cgbType_ = CgbType::CGB_ONLY; break;
		default: cgbType_ = CgbType::DMG; break;
	}

}

void Cartridge::setCartridgeExternalRam()
{
	cartridgeExternalRam_ = new byte[cartridgeExternalRAMSizeInKB_ * 1024];
	memset(cartridgeExternalRam_, 0xFF, cartridgeExternalRAMSizeInKB_ * 1024);
	FILE* eRamFile = fopen(saveFileName_.c_str(), "rb");
	if (eRamFile != nullptr)
	{
		// get file size
		fseek(eRamFile, 0, SEEK_END);
		fseek(eRamFile, 0, SEEK_SET);
		fread(cartridgeExternalRam_, sizeof(byte), cartridgeExternalRAMSizeInKB_ * 1024, eRamFile);
		fclose(eRamFile);
	}
}

void Cartridge::flushExternalRamToFile()
{
	switch (cartridgeType_)
	{
	case CartridgeType::MBC1_RAM_BATTERY:
	case CartridgeType::MBC1_RAM:
	case CartridgeType::MBC3_RAM_BATTERY:
	case CartridgeType::MBC3_TIMER_RAM_BATTERY:
	case CartridgeType::MBC5_RAM_BATTERY:
	case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
	{
		if (cartridgeExternalRAMSizeInKB_ > 0 && !externalRamEnabled_)
		{
			FILE* file = fopen(saveFileName_.c_str(), "wb");
			fwrite(cartridgeExternalRam_, sizeof(byte), cartridgeExternalRAMSizeInKB_ * 1024, file);
			fclose(file);
		}
	} break;
	default: break;
	}
}
