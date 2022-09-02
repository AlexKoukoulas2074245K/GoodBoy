#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"
#include "cartridge.h"

#include <functional>
#include <vector>

class CPU;
class Memory;
class Display final
{
public:
	Display();

	using VBlankCallback = std::function<void(byte*)>;
	void setVBlankCallback(VBlankCallback cb) { cb_ = cb; }
	void setMainMemoryBlock(byte* mem) { mainMemoryBlock_ = mem; }
	void setCPU(CPU* cpu) { cpu_ = cpu; }
	void setMemory(Memory* mem) { memory_ = mem; }
	void setCartridgeCgbType(Cartridge::CgbType cgbType) { cgbType_ = cgbType; }

	void update(const unsigned int spentCpuCycles);

	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);
	bool dmaTransferInProgress() const { return dmaClockCyclesRemaining_ > 0; }
	bool cgbHdmaTransferInProgress() const { return cgbHdmaClockCyclesRemaining_ > 0; }
	bool respectsIllegalReadWrites() const { return respectIllegalReadsWrites_;  }
	
private:
	void performDMATransfer(const byte b);
	void performCgbHDMATransfer(const byte b);
	void renderScanline();
	void renderBackgroundScanline();
	void renderWindowScanline();
	void renderOBJsScanline();
	void searchOBJSInCurrentScanline();
	void compareLYtoLYC();

private:
	byte cgbVram_[0x4000];
	byte cgbBackgroundPaletteRam_[0x40];
	byte cgbOBJPaletteRam_[0x40];
	byte finalSDLPixels_[160 * 144 * 4];
	byte bgAndWindowColorIndices[160 * 144];
	bool cgbBgTopLevelPriorityPixels[160 * 144];
	byte spriteColorIndices[160 * 144];
	std::vector<word> selectedOBJAddressesForCurrentScanline_;
	CPU* cpu_;
	Memory* memory_;
	byte* mainMemoryBlock_;
	VBlankCallback cb_;
	int clock_;
	int totalFrameClock_;
	int dmaClockCyclesRemaining_;
	int cgbHdmaClockCyclesRemaining_;
	word dmaSourceAddressStart_;
	word cgbHdmaSourceAddress_;
	word cgbHdmaDestinationAddress_;	
	word cgbHdmaTransferLength_;
	word cgbHdmaHblankTransferCurrentIndex_;
	byte lcdStatus_;
	byte lcdControl_;
	byte scy_, scx_;
	byte ly_;
	byte winLy_;
	byte lyc_;
	byte bgPalette_;
	byte obj0Palette_;
	byte obj1Palette_;
	byte winx_, winy_;
	byte cgbVramBank_;
	byte cgbBackgroundPaletteIndex_;
	byte cgbOBJPaletteIndex_;
	byte cgbHdmaTrigger_;
	byte cgbHdmaTransferMode_;
	Cartridge::CgbType cgbType_;
	bool respectIllegalReadsWrites_;
};

#endif