#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

#include <functional>
#include <vector>

class CPU;
class Display final
{
public:
	Display();

	using VBlankCallback = std::function<void(byte*)>;
	void setVBlankCallback(VBlankCallback cb) { cb_ = cb; }
	void setMemory(byte* mem) { mem_ = mem; }
	void setCPU(CPU* cpu) { cpu_ = cpu; }

	void update(const unsigned int spentCpuCycles);

	byte readByteAt(const word address) const;
	void writeByteAt(const word address, const byte b);
	bool dmaTransferInProgress() const { return dmaClockCyclesRemaining_ > 0; }
	bool respectsIllegalReadWrites() const { return respectIllegalReadsWrites_;  }
	
private:
	void performDMATransfer(const byte b);
	void renderScanline();
	void renderBackgroundScanline();
	void renderWindowScanline();
	void renderOBJsScanline();
	void searchOBJSInCurrentScanline();

private:
	byte finalSDLPixels_[160 * 144 * 4];
	byte bgAndWindowColorIndices[160 * 144];
	byte spriteColorIndices[160 * 144];
	std::vector<word> selectedOBJAddressesForCurrentScanline_;
	CPU* cpu_;
	byte* mem_;
	VBlankCallback cb_;
	int clock_;
	int totalFrameClock_;
	int dmaClockCyclesRemaining_;
	word dmaSourceAddressStart_;
	byte lcdStatus_;
	byte lcdControl_;
	byte scy_, scx_;
	byte ly_;
	byte lyc_;
	byte bgPalette_;
	byte obj0Palette_;
	byte obj1Palette_;
	byte winx_, winy_;
	bool respectIllegalReadsWrites_;
};

#endif