#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "types.h"

#include <string>

class Cartridge final
{
public:
	Cartridge();
	~Cartridge();

	std::string loadCartridge(const char* filename);
	void unloadCartridge();
	
	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);

private:
	byte* cartridgeRom_;
	std::string cartridgeName_;
	byte cartridgeType_;
};

#endif