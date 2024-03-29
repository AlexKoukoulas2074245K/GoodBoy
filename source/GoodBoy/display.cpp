#include "cpu.h"
#include "display.h"
#include "logging.h"
#include "memory.h"

#include <algorithm>
#include <cassert>

#define GET_DISPLAY_MODE() (lcdStatus_ & 0x03)
#define SET_DISPLAY_MODE(mode) lcdStatus_ = (lcdStatus_ & 0xFC) | mode
#define IS_BIT_SET(bit, reg) (((reg >> bit) & 0x1) == 0x1)
#define SET_BIT(bit, reg) reg |= (1 << bit)
#define RESET_BIT(bit, reg) reg &= (~(1 << bit))

static constexpr int SCANLINE_DOTS            = 456;
static constexpr int HBLANK_DOTS              = 204;
static constexpr int VBLANK_DOTS              = 4560;
static constexpr int SEARCHING_OAM_DOTS       = 80;
static constexpr int TRANSFERRING_TO_LCD_DOTS = 172;
static constexpr int TOTAL_PER_FRAME_DOTS     = 70224;

static constexpr byte DISPLAY_MODE_HBLANK              = 0x0;
static constexpr byte DISPLAY_MODE_VBLANK              = 0x1;
static constexpr byte DISPLAY_MODE_SEARCHING_OAM       = 0x2;
static constexpr byte DISPLAY_MODE_TRANSFERRING_TO_LCD = 0x3;


/*
	Bit           Name                      Usage
	7	LCD and PPU enable	            0=Off, 1=On
	6	Window tile map area	        0=9800-9BFF, 1=9C00-9FFF
	5	Window enable	                0=Off, 1=On
	4	BG and Window tile data area	0=8800-97FF, 1=8000-8FFF
	3	BG tile map area	            0=9800-9BFF, 1=9C00-9FFF
	2	OBJ size	                    0=8x8, 1=8x16
	1	OBJ enable	                    0=Off, 1=On
	0	BG and Window enable/priority	0=Off, 1=On
*/
static constexpr word LCD_CONTROL_ADDRESS = 0xFF40;


/*
	Bit           Name                            Usage
	6	LYC=LY STAT Interrupt source         (1=Enable) (R/W)
	5	Mode 2 OAM STAT Interrupt source     (1=Enable) (R/W)
	4	Mode 1 VBlank STAT Interrupt source  (1=Enable) (R/W)
	3	Mode 0 HBlank STAT Interrupt source  (1=Enable) (R/W)
	2	LYC=LY Flag 	                     (0=Different, 1=Equal) (R)
	1-0	Mode Flag                            (Mode 0-3, see below)  (R)
        0: HBlank
        1: VBlank
        2: Searching OAM
        3: Transferring Data to LCD Controller

*/
static constexpr word LCD_STATUS_ADDRESS = 0xFF41;


/*
	Those specify the top-left coordinates of the visible 160�144 pixel area within 
	the 256�256 pixels BG map. Values in the range 0�255 may be used (R/W).
*/
static constexpr word SCY_ADDRESS = 0xFF42;
static constexpr word SCX_ADDRESS = 0xFF43;


/*
	LY indicates the current horizontal line, which might be about to be drawn, being drawn, 
	or just been drawn. LY can hold any value from 0 to 153, 
	with values from 144 to 153 indicating the VBlank period. (R)
*/
static constexpr word LY_ADDRESS = 0xFF44;


/*
	The Game Boy permanently compares the value of the LYC and LY registers. When both values are identical, 
	the �LYC=LY� flag in the STAT register is set, and (if enabled) a STAT interrupt is requested. (R/W)
*/
static constexpr word LYC_ADDRESS = 0xFF45;


/*
	Source:      $XX00-$XX9F   ;XX = $00 to $DF
	Destination: $FE00-$FE9F (R/W)
*/
static constexpr word DMA_TRANSFER_ADDRESS = 0xFF46;
static constexpr int DMA_CLOCK_CYCLES = 640;

/*
	Bit 7-6 - Color for index 3       Gameboy Native Color Indices:  0 = WHITE
	Bit 5-4 - Color for index 2                                      1 = LIGHT_GRAY   
	Bit 3-2 - Color for index 1                                      2 = DARK_GRAY
	Bit 1-0 - Color for index 0                                      3 = BLACK
	(R/W)
*/
static constexpr word BG_PALETTE_DATA_ADDRESS = 0xFF47;


/*
	Bit 7-6 - Color for index 3       Gameboy Native Color Indices:  0 = WHITE
	Bit 5-4 - Color for index 2                                      1 = LIGHT_GRAY
	Bit 3-2 - Color for index 1                                      2 = DARK_GRAY
	Bit 1-0 - Ignored. All OBJs have transparent color index 0
	(R/W)
*/
static constexpr word OBJ_PALETTE_0_DATA_ADDRESS = 0xFF48;
static constexpr word OBJ_PALETTE_1_DATA_ADDRESS = 0xFF49;


/*
	Specify the top-left coordinates of the Window. (The Window is an alternate background area which can be
	displayed above of the normal background. OBJs (sprites) may be still displayed above or behind the Window, just as for normal BG.)

	The Window is visible (if enabled) when both coordinates are in the ranges WX=0..166, WY=0..143 respectively. 
	Values WX=7, WY=0 place the Window at the top left of the screen, completely covering the background. (R/W)
*/
static constexpr word WIN_Y_ADDRESS = 0xFF4A;
static constexpr word WIN_X_ADDRESS = 0xFF4B;


/*
	This register can be written to change VRAM banks. Only bit 0 matters, all other bits are ignored.

	VRAM bank 1 is split like VRAM bank 0 ; 8000-97FF also stores tiles (just like in bank 0), 
	which can be accessed the same way as (and at the same time as) bank 0 tiles. 9800-9FFF contains the attributes for the corresponding Tile Maps.

	Reading from this register will return the number of the currently loaded VRAM bank in bit 0, and all other bits will be set to 1.
*/
static constexpr word VRAM_BANK_ADDRESS = 0xFF4F;


/*
	These two registers specify the address at which the transfer will read data from. Normally, this should be either in ROM, SRAM or WRAM,
	thus either in range 0000-7FF0 or A000-DFF0. Trying to specify a source address in VRAM will cause garbage to be copied.

	The four lower bits of this address will be ignored and treated as 0. (W)
*/
static constexpr word HDMA_SOURCE_START_HIGH_ADDRESS = 0xFF51;
static constexpr word HDMA_SOURCE_START_LOW_ADDRESS  = 0xFF52;


/*
	These two registers specify the address within 8000-9FF0 to which the data will be copied. Only bits 12-4 are respected; others are ignored. 
	The four lower bits of this address will be ignored and treated as 0.
*/
static constexpr word HDMA_DESTINATION_START_HIGH_ADDRESS = 0xFF53;
static constexpr word HDMA_DESTINATION_START_LOW_ADDRESS  = 0xFF54;


/*
	When using this transfer method, all data is transferred at once. The execution of the program is halted until the transfer has completed. 
	Note that the General Purpose DMA blindly attempts to copy the data, even if the LCD controller is currently accessing VRAM. 
	So General Purpose DMA should be used only if the Display is disabled, or during VBlank, or (for rather short blocks) during HBlank. 
	The execution of the program continues when the transfer has been completed, and FF55 then contains a value of FFh.

	Bit 7 = 1 - HBlank DMA
	The HBlank DMA transfers 10h bytes of data during each HBlank, that is, at LY=0-143, no data is transferred during VBlank (LY=144-153), but the transfer will then continue at LY=00. 
	The execution of the program is halted during the separate transfers, but the program execution continues during the �spaces� between each data block. 
	Note that the program should not change the Destination VRAM bank (FF4F), or the Source ROM/RAM bank (in case data is transferred from bankable memory) until the transfer has completed! 
	(The transfer should be paused as described below while the banks are switched)

	Reading from Register FF55 returns the remaining length (divided by 10h, minus 1), a value of 0FFh indicates that the transfer has completed. 
	It is also possible to terminate an active HBlank transfer by writing zero to Bit 7 of FF55. In that case reading from FF55 will return how many 
	$10 �blocks� remained (minus 1) in the lower 7 bits, but Bit 7 will be read as �1�. Stopping the transfer doesn�t set HDMA1-4 to $FF.

	WARNING
	HBlank DMA should not be started (write to FF55) during a HBlank period (STAT mode 0).

	If the transfer�s destination address overflows, the transfer stops prematurely. 
	The status of the registers if this happens still needs to be investigated.

	Confirming if the DMA Transfer is Active
	Reading Bit 7 of FF55 can be used to confirm if the DMA transfer is active (1=Not Active, 0=Active). 
	This works under any circumstances - after completion of General Purpose, or HBlank Transfer, and after manually terminating a HBlank Transfer.

	Transfer Timings
	In both Normal Speed and Double Speed Mode it takes about 8 micros to transfer a block of $10 bytes. 
	That is, 8 M-cycles in Normal Speed Mode [1], and 16 "fast" MACHINE-cycles in Double Speed Mode [2]. 
	Older MBC controllers (like MBC1-3) and slower ROMs are not guaranteed to support General Purpose or HBlank DMA, 
	that's because there are always 2 bytes transferred per microsecond (even if the itself program runs it Normal Speed Mode).
*/
static constexpr word HDMA_TRIGGER_ADDRESS = 0xFF55;
static constexpr unsigned int HDMA_16_BYTE_TRANSFER_IN_CLOCK_CYLES = 32;


/*
	This register is used to address a byte in the CGB�s background palette RAM. Since there are 8 palettes, 8 palettes � 4 colors/palette � 2 bytes/color = 64 bytes can be addressed.

	First comes BGP0 color number 0, then BGP0 color number 1, BGP0 color number 2, BGP0 color number 3, BGP1 color number 0, and so on. 
	Thus, address $03 allows accessing the second (upper) byte of BGP0 color #1 via BCPD, which contains the color�s blue and upper green bits.
*/
static constexpr word CGB_BACKGROUND_PALETTE_INDEX_ADDRESS = 0xFF68;
static constexpr word CGB_OBJ_PALETTE_INDEX_ADDRESS        = 0xFF6A;


/*
	This register allows to read/write data to the CGBs background palette memory, addressed through BCPS/BGPI. Each color is stored as little-endian RGB555:

	Bit 0-4   Red Intensity   ($00-1F)
	Bit 5-9   Green Intensity ($00-1F)
	Bit 10-14 Blue Intensity  ($00-1F)
	Much like VRAM, data in palette memory cannot be read or written during the time when the PPU is reading from it, that is, Mode 3.
*/
static constexpr word CGB_BACKGROUND_PALETTE_DATA_ADDRESS = 0xFF69;
static constexpr word CGB_OBJ_PALETTE_DATA_ADRESS         = 0xFF6B;

/*
	System Core Colors
*/
static const byte GAMEBOY_NATIVE_COLORS[4][4] =
{
    //A   //B   //G   //R
    {0xFF, 0xD0, 0xF8, 0xE0},
    {0xFF, 0x70, 0xC0, 0x88},
    {0xFF, 0x56, 0x68, 0x34},
    {0xFF, 0x20, 0x18, 0x08}
};

Display::Display()
	: mainMemoryBlock_(nullptr)
	, clock_(VBLANK_DOTS)
	, totalFrameClock_(0)
	, dmaClockCyclesRemaining_(0)
	, cgbHdmaClockCyclesRemaining_(0)
	, dmaSourceAddressStart_(0)
	, cgbHdmaSourceAddress_(0)
	, cgbHdmaDestinationAddress_(0)
	, cgbHdmaTransferLength_(0)
	, lcdStatus_(0)
	, lcdControl_(0)
	, scy_(0), scx_(0)
	, ly_(0)
	, winLy_(0)
	, lyc_(0)
	, bgPalette_(0)
	, obj0Palette_(0)
	, obj1Palette_(0)
	, winx_(0), winy_(0)
	, cgbVramBank_(0xFE)
	, cgbBackgroundPaletteIndex_(0)
    , cgbOBJPaletteIndex_(0)
    , cgbHdmaTrigger_(0)
	, cgbHdmaTransferMode_(0)
	, cgbType_(Cartridge::CgbType::DMG)
	, respectIllegalReadsWrites_(true)
{
	memset(finalSDLPixels_, 0xFF, sizeof(finalSDLPixels_));
	memset(cgbVram_, 0xFF, sizeof(cgbVram_));
	SET_DISPLAY_MODE(DISPLAY_MODE_VBLANK);
}

void Display::update(const unsigned int spentCpuCycles)
{
	if (dmaClockCyclesRemaining_ > 0)
	{
		dmaClockCyclesRemaining_ -= spentCpuCycles;

		if (dmaClockCyclesRemaining_ <= 0)
		{
			// DMA finished, copy over the contents to OAM ram
			for (int i = 0x00; i <= 0x9F; ++i)
			{
				mainMemoryBlock_[Memory::OAM_START_ADDRESS + i] = memory_->readByteAt(dmaSourceAddressStart_ + i);
			}
		}
		return;
	}

	if (cgbHdmaClockCyclesRemaining_ > 0)
	{
		cgbHdmaClockCyclesRemaining_ -= spentCpuCycles;

		if (cgbHdmaClockCyclesRemaining_ <= 0 && cgbHdmaTransferMode_ == 0)			
		{
			// HDMA finished, copy over the contents to destination
			for (int i = 0x00; i < cgbHdmaTransferLength_; ++i)
			{				
				cgbVram_[(cgbHdmaDestinationAddress_ + i - Memory::VRAM_START_ADDRESS) + (cgbVramBank_ & 0x1) * 0x2000] = memory_->readByteAt(cgbHdmaSourceAddress_ + i);
			}

			cgbHdmaTrigger_ = 0xFF;
			return;
		}
	}

	// Display/PPU is off. Return early
	if (!IS_BIT_SET(7, lcdControl_))
	{
		return;
	}

	clock_ += spentCpuCycles;
	totalFrameClock_ += spentCpuCycles;

	switch (GET_DISPLAY_MODE())
	{
		case DISPLAY_MODE_HBLANK:
		{
			if (clock_ >= HBLANK_DOTS)
			{
				clock_ -= HBLANK_DOTS;
				
				if (++ly_ == 144)
				{				
					SET_DISPLAY_MODE(DISPLAY_MODE_VBLANK);
					cpu_->triggerInterrupt(CPU::VBLANK_INTERRUPT_BIT);

					if (IS_BIT_SET(4, lcdStatus_))
					{
						cpu_->triggerInterrupt(CPU::LCD_STAT_INTERRUPT_BIT);
					}
				}
				else
				{					
					SET_DISPLAY_MODE(DISPLAY_MODE_SEARCHING_OAM);

					if (IS_BIT_SET(5, lcdStatus_))
					{
						cpu_->triggerInterrupt(CPU::LCD_STAT_INTERRUPT_BIT);
					}
				}

				compareLYtoLYC();
			}
		} break;

		case DISPLAY_MODE_VBLANK:
		{
			if (clock_ >= SCANLINE_DOTS)
			{
				clock_ -= SCANLINE_DOTS;				
				ly_++;

				compareLYtoLYC();
			}

			if (ly_ == 154)
			{
				ly_ = 0;
				winLy_ = 0;

				compareLYtoLYC();

				cb_(finalSDLPixels_);
				
				memset(bgAndWindowColorIndices, 0, sizeof(bgAndWindowColorIndices));
				memset(cgbBgTopLevelPriorityPixels, false, sizeof(cgbBgTopLevelPriorityPixels));
				memset(spriteColorIndices, 0, sizeof(spriteColorIndices));

				SET_DISPLAY_MODE(DISPLAY_MODE_SEARCHING_OAM);

				if (IS_BIT_SET(5, lcdStatus_))
				{
					cpu_->triggerInterrupt(CPU::LCD_STAT_INTERRUPT_BIT);
				}

				if (totalFrameClock_ != TOTAL_PER_FRAME_DOTS)
				{
					//log(LogType::WARNING, "Display frame time not %d. Instead it was %d", TOTAL_PER_FRAME_DOTS, totalFrameClock_);
				}
				//log(LogType::INFO, "====================================================================================");
				totalFrameClock_ = 0;
					
			}
		} break;

		case DISPLAY_MODE_SEARCHING_OAM:
		{
			if (clock_ >= SEARCHING_OAM_DOTS)
			{				
				searchOBJSInCurrentScanline();
				clock_ -= SEARCHING_OAM_DOTS;
				SET_DISPLAY_MODE(DISPLAY_MODE_TRANSFERRING_TO_LCD);
			}
		} break;

		case DISPLAY_MODE_TRANSFERRING_TO_LCD:
		{
			if (clock_ >= TRANSFERRING_TO_LCD_DOTS)
			{				
				clock_ -= TRANSFERRING_TO_LCD_DOTS;
				SET_DISPLAY_MODE(DISPLAY_MODE_HBLANK);
				
				renderScanline();
				
				if (cgbHdmaClockCyclesRemaining_ > 0)
				{
					for (int i = 0; i < 0x10; ++i)
					{
						cgbVram_[(cgbHdmaDestinationAddress_ + cgbHdmaHblankTransferCurrentIndex_ + i - Memory::VRAM_START_ADDRESS) + 
							     (cgbVramBank_ & 0x1) * 0x2000] = memory_->readByteAt(cgbHdmaSourceAddress_ + cgbHdmaHblankTransferCurrentIndex_ + i);
					}
					cgbHdmaHblankTransferCurrentIndex_ += 0x10;
					if (cgbHdmaHblankTransferCurrentIndex_ == cgbHdmaTransferLength_)
					{
						assert(false);
					}
				}

				if (IS_BIT_SET(3, lcdStatus_))
				{
					cpu_->triggerInterrupt(CPU::LCD_STAT_INTERRUPT_BIT);
				}
			}
		} break;
	}
}

byte Display::readByteAt(const word address) const
{
	if (address >= Memory::VRAM_START_ADDRESS && address <= Memory::VRAM_END_ADDRESS)
	{
		if (GET_DISPLAY_MODE() == DISPLAY_MODE_TRANSFERRING_TO_LCD)
		{
			log(LogType::WARNING, "Attempt to read from VRAM during LCD transfer. Returning garbage."); 
			if (respectIllegalReadsWrites_) return 0xFF;
		}

		if (cgbType_ == Cartridge::CgbType::DMG)
			return mainMemoryBlock_[address];
		else
			return cgbVram_[(address - Memory::VRAM_START_ADDRESS) + (cgbVramBank_ & 0x1) * 0x2000];
	}
	if (address >= Memory::OAM_START_ADDRESS && address <= Memory::OAM_END_ADDRESS)
	{
		if (GET_DISPLAY_MODE() == DISPLAY_MODE_SEARCHING_OAM || GET_DISPLAY_MODE() == DISPLAY_MODE_TRANSFERRING_TO_LCD)
		{
			log(LogType::WARNING, "Attempt to read from OAM during LCD transfer or searching phase. Returning garbage.");
			if (respectIllegalReadsWrites_) return 0xFF;
		}
		return mainMemoryBlock_[address];
	}

	switch (address)
	{
		case LCD_CONTROL_ADDRESS: return lcdControl_;
		case LCD_STATUS_ADDRESS: return lcdStatus_;
		case SCY_ADDRESS: return scy_;
		case SCX_ADDRESS: return scx_;
		case LY_ADDRESS: return ly_;
		case LYC_ADDRESS: return lyc_;
		case DMA_TRANSFER_ADDRESS: log(LogType::WARNING, "Attempt to read from DMA Address"); return 0xFF;
		case BG_PALETTE_DATA_ADDRESS: return bgPalette_;
		case OBJ_PALETTE_0_DATA_ADDRESS: return obj0Palette_;
		case OBJ_PALETTE_1_DATA_ADDRESS: return obj1Palette_;
		case WIN_X_ADDRESS: return winx_;
		case WIN_Y_ADDRESS: return winy_;
		case VRAM_BANK_ADDRESS: return cgbVramBank_;
		case HDMA_SOURCE_START_HIGH_ADDRESS: log(LogType::WARNING, "Attempt to read from high byte source HDMA Address"); return 0xFF;
		case HDMA_SOURCE_START_LOW_ADDRESS: log(LogType::WARNING, "Attempt to read from low byte source HDMA Address"); return 0xFF;
		case HDMA_DESTINATION_START_HIGH_ADDRESS: log(LogType::WARNING, "Attempt to read from high byte destination HDMA Address"); return 0xFF;
		case HDMA_DESTINATION_START_LOW_ADDRESS: log(LogType::WARNING, "Attempt to read from low byte destination HDMA Address"); return 0xFF;
		case HDMA_TRIGGER_ADDRESS: return cgbHdmaTrigger_;
		case CGB_BACKGROUND_PALETTE_INDEX_ADDRESS: return cgbBackgroundPaletteIndex_;
		case CGB_BACKGROUND_PALETTE_DATA_ADDRESS: return cgbBackgroundPaletteRam_[cgbBackgroundPaletteIndex_ & (0x3F)];
		case CGB_OBJ_PALETTE_INDEX_ADDRESS: return cgbOBJPaletteIndex_;
		case CGB_OBJ_PALETTE_DATA_ADRESS: return cgbOBJPaletteRam_[cgbOBJPaletteIndex_ & (0x3F)];
		default: log(LogType::WARNING, "Display::readByteAt Unknown read at %s", getHexWord(address).c_str());
	}
	return 0xFF;
}

void Display::writeByteAt(const word address, const byte b)
{
	if (address >= Memory::VRAM_START_ADDRESS && address <= Memory::VRAM_END_ADDRESS)
	{
		if (GET_DISPLAY_MODE() == DISPLAY_MODE_TRANSFERRING_TO_LCD)
		{
			log(LogType::WARNING, "Attempt to write to VRAM during LCD transfer. Ignoring write.");			
			if (respectIllegalReadsWrites_) return;
		}

		if (cgbType_ == Cartridge::CgbType::DMG)
			mainMemoryBlock_[address] = b;
		else
			cgbVram_[(address - Memory::VRAM_START_ADDRESS) + (cgbVramBank_ & 0x1) * 0x2000] = b;
		return;
	}
	if (address >= Memory::OAM_START_ADDRESS && address <= Memory::OAM_END_ADDRESS)
	{
		if (GET_DISPLAY_MODE() == DISPLAY_MODE_SEARCHING_OAM || GET_DISPLAY_MODE() == DISPLAY_MODE_TRANSFERRING_TO_LCD)
		{
			log(LogType::WARNING, "Attempt to write to OAM during LCD transfer or searching phase. Ignoring write.");			
			if (respectIllegalReadsWrites_) return;
		}
		mainMemoryBlock_[address] = b;
		return;
	}

	switch (address)
	{
		case LCD_CONTROL_ADDRESS: 
		{
			bool oldLCDVal = IS_BIT_SET(7, lcdControl_);

			lcdControl_ = b;
			
			if (IS_BIT_SET(7, lcdControl_) && !oldLCDVal)
			{
				compareLYtoLYC();
			}
			
			if (!IS_BIT_SET(7, lcdControl_))
			{
				ly_ = 0;
				clock_ = 0;
				winLy_ = 0;
				SET_DISPLAY_MODE(DISPLAY_MODE_HBLANK);
			}
		} break;
		case LCD_STATUS_ADDRESS: lcdStatus_ = (b & 0xF8) | (lcdStatus_ & 0x07); break;  // Last 3 bits not writeable
		case SCY_ADDRESS: scy_ = b; break;
		case SCX_ADDRESS: scx_ = b; break;
		case LY_ADDRESS: log(LogType::WARNING, "Attempted to write %s at LY (%s). It is read only", getHexByte(b).c_str(), getHexWord(LY_ADDRESS).c_str()); break;
		case LYC_ADDRESS: lyc_ = b; break;
		case DMA_TRANSFER_ADDRESS: performDMATransfer(b); break;
		case BG_PALETTE_DATA_ADDRESS: bgPalette_ = b; break;
		case OBJ_PALETTE_0_DATA_ADDRESS: obj0Palette_ = b; break; // Bottom 2 bits are discarded since color index 0 for OBJs is always transparent
		case OBJ_PALETTE_1_DATA_ADDRESS: obj1Palette_ = b; break; // Bottom 2 bits are discarded since color index 0 for OBJs is always transparent
		case WIN_X_ADDRESS: winx_ = b; break;
		case WIN_Y_ADDRESS: winy_ = b; break;
		case VRAM_BANK_ADDRESS: cgbVramBank_ = ((b & 0x1) == 0x1) ? 0xFF : 0xFE; break; // Only set bottom bit for selected vram bank
		case HDMA_SOURCE_START_HIGH_ADDRESS: cgbHdmaSourceAddress_ = (b << 8) | (cgbHdmaSourceAddress_ & 0x00FF);  break;
		case HDMA_SOURCE_START_LOW_ADDRESS: cgbHdmaSourceAddress_ = (cgbHdmaSourceAddress_ & 0xFF00) | (b & 0xF0); break; // 4 bottom bits ignored
		case HDMA_DESTINATION_START_HIGH_ADDRESS: cgbHdmaDestinationAddress_ = ((b << 8) | (cgbHdmaDestinationAddress_ & 0x00FF)); break;
		case HDMA_DESTINATION_START_LOW_ADDRESS: cgbHdmaDestinationAddress_ = (cgbHdmaDestinationAddress_ & 0xFF00) | (b & 0xF0); break; // 4 bottom bits ignored 
		case HDMA_TRIGGER_ADDRESS: performCgbHDMATransfer(b);  break;
		case CGB_BACKGROUND_PALETTE_INDEX_ADDRESS: cgbBackgroundPaletteIndex_ = b; break;
		case CGB_OBJ_PALETTE_INDEX_ADDRESS: cgbOBJPaletteIndex_ = b; break;
		case CGB_BACKGROUND_PALETTE_DATA_ADDRESS: 
		{
			cgbBackgroundPaletteRam_[cgbBackgroundPaletteIndex_ & (0x3F)] = b;

			// Auto icrement index if bit 7 is set
			if (IS_BIT_SET(7, cgbBackgroundPaletteIndex_))
			{
				cgbBackgroundPaletteIndex_++;

				// If the auto increment exceeded max index (3F), then just unset bit 6
				if ((cgbBackgroundPaletteIndex_ & (0x3F)) == 0x00)
				{
					RESET_BIT(6, cgbBackgroundPaletteIndex_);
				}
			}
		} break;
		case CGB_OBJ_PALETTE_DATA_ADRESS: 
		{
			cgbOBJPaletteRam_[cgbOBJPaletteIndex_ & (0x3F)] = b;

			// Auto icrement index if bit 7 is set
			if (IS_BIT_SET(7, cgbOBJPaletteIndex_))
			{
				cgbOBJPaletteIndex_++;

				// If the auto increment exceeded max index (3F), then just unset bit 6
				if ((cgbOBJPaletteIndex_ & (0x3F)) == 0x00)
				{
					RESET_BIT(6, cgbOBJPaletteIndex_);
				}
			}			
		} break;
		default: log(LogType::WARNING, "Display::writeByteAt Unknown write %s at %s", getHexByte(b).c_str(), getHexWord(address).c_str());
	}
}

void Display::performDMATransfer(const byte b)
{
	dmaClockCyclesRemaining_ = DMA_CLOCK_CYCLES;
	dmaSourceAddressStart_ = b << 8;
}

void Display::performCgbHDMATransfer(const byte b)
{
	assert(cgbHdmaDestinationAddress_ >= 0x8000 && cgbHdmaDestinationAddress_ <= 0x9FF0);
	cgbHdmaTransferLength_ = (((b & 0x7F) + 1) * 0x10);	
	cgbHdmaClockCyclesRemaining_ = (cgbHdmaTransferLength_ / 0x10) * HDMA_16_BYTE_TRANSFER_IN_CLOCK_CYLES;
	cgbHdmaTransferMode_ = IS_BIT_SET(7, b) ? 1 : 0;
	assert(cgbHdmaTransferMode_ == 0);
	cgbHdmaTrigger_ = 0;
	cgbHdmaHblankTransferCurrentIndex_ = 0;
}

void Display::renderScanline()
{
	// Display/Window disabled. Clear all pixels to white
	if (!IS_BIT_SET(0, lcdControl_))
	{
		for (int i = 0; i < 160; i += 4)
		{
			finalSDLPixels_[(ly_ * 4 * 160) + (i * 4) + 0] = GAMEBOY_NATIVE_COLORS[0][0];
			finalSDLPixels_[(ly_ * 4 * 160) + (i * 4) + 1] = GAMEBOY_NATIVE_COLORS[0][1];
			finalSDLPixels_[(ly_ * 4 * 160) + (i * 4) + 2] = GAMEBOY_NATIVE_COLORS[0][2];
			finalSDLPixels_[(ly_ * 4 * 160) + (i * 4) + 3] = GAMEBOY_NATIVE_COLORS[0][3];
		}
	}
	else
	{
		renderBackgroundScanline();

		// Only draw window if it's specifically enabled by bit 5
		if (IS_BIT_SET(5, lcdControl_))
		{
			renderWindowScanline();
			
		}
	}

	// Only draw OBJs if they are specifically enabled by bit 1
	if (IS_BIT_SET(1, lcdControl_))
	{
		renderOBJsScanline();		
	}	
}

void Display::renderBackgroundScanline()
{
	// Render BG
	word startingBgMapAddress = !IS_BIT_SET(3, lcdControl_) ? 0x9800 : 0x9C00;
	word startingBgAndWindowTileDataAddress = !IS_BIT_SET(4, lcdControl_) ? 0x9000 : 0x8000;
	bool signedTileAddressing = !IS_BIT_SET(4, lcdControl_);

	// Assemble current palette. i.e. assign gray shades to the color indexes for bg and window tiles
	byte palette[4] =
	{
		static_cast<byte>((bgPalette_ & 0x03) >> 0),
		static_cast<byte>((bgPalette_ & 0x0C) >> 2),
		static_cast<byte>((bgPalette_ & 0x30) >> 4),
		static_cast<byte>((bgPalette_ & 0xC0) >> 6)
	};

	for (int i = 0; i < 160; ++i)
	{
		word wrappedPixelXCoord = ((scx_ + i) % 0x100);   // Makes sure X coord is in the [0-255] range after accounting for scrolling
		word wrappedPixelYCoord = ((scy_ + ly_) % 0x100); // Makes sure Y coord is in the [0-255] range after accounting for scrolling

		word bgXCoord = wrappedPixelXCoord >> 3;          // Transforms X coord into BG map coord (dividing by 8 as the bg map is a 32x32 grid)
		word bgYCoord = wrappedPixelYCoord >> 3;          // Transforms Y coord into BG map coord (dividing by 8 as the bg map is a 32x32 grid)

		if (cgbType_ != Cartridge::CgbType::DMG)
		{
			// Bit 7    BG-to-OAM Priority         (0=Use OAM Priority bit, 1=BG Priority)
			// Bit 6    Vertical Flip(0 = Normal, 1 = Mirror vertically)
			// Bit 5    Horizontal Flip(0 = Normal, 1 = Mirror horizontally)
			// Bit 4    Not used
			// Bit 3    Tile VRAM Bank number(0 = Bank 0, 1 = Bank 1)
			// Bit 2 - 0  Background Palette number(BGP0 - 7)
			byte tileAttributes = cgbVram_[0x3800 + startingBgMapAddress + ((bgYCoord * 0x20) + bgXCoord) - 0x9800];
	
			bool bgToOAMPriority          = IS_BIT_SET(7, tileAttributes);
			bool verticalFlip             = IS_BIT_SET(6, tileAttributes);
			bool horizontalFlip           = IS_BIT_SET(5, tileAttributes);
			bool useTileDataFromVRAMBank0 = !IS_BIT_SET(3, tileAttributes);
			byte cgbPaletteNumber         = tileAttributes & 0x7;

			word tileId = cgbVram_[0x1800 + startingBgMapAddress + ((bgYCoord * 0x20) + bgXCoord) - 0x9800]; // Find tile id based on the above coords
			
			word tileAddress = startingBgAndWindowTileDataAddress + 16 * tileId;           // Find tile address based on Tile ID. Each tile data occupies 16 bytes

			if (signedTileAddressing)
				tileAddress = startingBgAndWindowTileDataAddress + (16 * static_cast<sbyte>(tileId));

			if (!useTileDataFromVRAMBank0)
				tileAddress = tileAddress - 0x8000 + 0x2000; // Remove 0x800 offset for tile data, but add vram bank 1's offset as well
			else
				tileAddress = tileAddress - 0x8000;

			byte tileCol = horizontalFlip ? (7 - (wrappedPixelXCoord % 8)) : wrappedPixelXCoord % 8; // Get tile data col to draw
			byte tileRow = verticalFlip ? (7 - (wrappedPixelYCoord % 8)) : wrappedPixelYCoord % 8; // Get tile data row to draw	

			word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
			byte targetTileDataFirstByte = cgbVram_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
			byte targetTileDataSecondByte = cgbVram_[targetTileRowAddress + 1];     // Get second byte of tile data row (most significant bits for the final color index)
			
			byte colorIndexLSB = (targetTileDataFirstByte >> (7 - tileCol)) & 0x01;  // Get target bit from first byte
			byte colorIndexMSB = (targetTileDataSecondByte >> (7 - tileCol)) & 0x01; // Get target bit from second byte
		
			byte colorIndex = colorIndexMSB << 1 | colorIndexLSB; // Assemble final color index

			bgAndWindowColorIndices[ly_ * 160 + i] = colorIndex;
			cgbBgTopLevelPriorityPixels[ly_ * 160 + i] = bgToOAMPriority;				

			// Palette Ram is 64 bytes. Each palette 0..7 takes up 8 bytes.
			// PaletteN 8 bytes distribution:
			// byte 0 - Color 0 high byte (blue & upper green data) where -> bits 6,5,4,3,2 (blue) bits 1,0 (high green)
			// byte 1 - Color 0 low byte  (lower green data & red)  where -> bits 7,6,5 (low green) bits 4,3,2,1,0 (red)
			// byte 2 - Color 1 high byte (blue & upper green data) where -> bits 6,5,4,3,2 (blue) bits 1,0 (high green)
			// .....
			// byte 7 - Color 3 low byte (lower green data & red)   where -> bits 7,6,5 (low green) bits 4,3,2,1,0 (red)
			byte colorHighByte = cgbBackgroundPaletteRam_[cgbPaletteNumber * 8 + (colorIndex * 2) + 1]; // Little endian which is why the high byte is on position n + 1
			byte colorLowByte  = cgbBackgroundPaletteRam_[cgbPaletteNumber * 8 + (colorIndex * 2) + 0];

			float fracBlueIntensity  = ((colorHighByte & 0x7C) >> 2) / static_cast<float>(0x1F);
			float fracGreenIntensity = ((((colorHighByte & 0x3) << 3) | ((colorLowByte & 0xE0) >> 5))) / static_cast<float>(0x1F);
			float fracRedIntensity   = ((colorLowByte & 0x1F) >> 0) / static_cast<float>(0x1F);

			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 0] = 0xFF;                                         // A
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 1] = static_cast<byte>(fracBlueIntensity * 0xFF);  // B
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 2] = static_cast<byte>(fracGreenIntensity * 0xFF); // G
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 3] = static_cast<byte>(fracRedIntensity * 0xFF);   // R
		}
		else // DMG
		{
			word tileId = mainMemoryBlock_[startingBgMapAddress + ((bgYCoord * 0x20) + bgXCoord)]; // Find tile id based on the above coords
			word tileAddress = startingBgAndWindowTileDataAddress + 16 * tileId;       // Find tile address based on Tile ID. Each tile data occupies 16 bytes

			if (signedTileAddressing)
				tileAddress = startingBgAndWindowTileDataAddress + (16 * static_cast<sbyte>(tileId));

			byte tileCol = wrappedPixelXCoord % 8; // Get tile data col to draw
			byte tileRow = wrappedPixelYCoord % 8; // Get tile data row to draw		

			word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
			byte targetTileDataFirstByte = mainMemoryBlock_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
			byte targetTileDataSecondByte = mainMemoryBlock_[targetTileRowAddress + 1];     // Get second byte of tile data row (most significant bits for the final color index)

			byte colorIndexLSB = (targetTileDataFirstByte >> (7 - tileCol)) & 0x01;  // Get target bit from first byte
			byte colorIndexMSB = (targetTileDataSecondByte >> (7 - tileCol)) & 0x01; // Get target bit from second byte

			byte colorIndex = colorIndexMSB << 1 | colorIndexLSB; // Assemble final color index

			bgAndWindowColorIndices[ly_ * 160 + i] = palette[colorIndex];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 0] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][0];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 1] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][1];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 2] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][2];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 3] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][3];
		}
	}
}

void Display::renderWindowScanline()
{
	word startingWindowMapAddress = !IS_BIT_SET(6, lcdControl_) ? 0x9800 : 0x9C00;
	word startingBgAndWindowTileDataAddress = !IS_BIT_SET(4, lcdControl_) ? 0x9000 : 0x8000;
	bool signedTileAddressing = !IS_BIT_SET(4, lcdControl_);

	// Assemble current palette. i.e. assign gray shades to the color indexes for bg and window tiles
	byte palette[4] =
	{
		static_cast<byte>((bgPalette_ & 0x03) >> 0),
		static_cast<byte>((bgPalette_ & 0x0C) >> 2),
		static_cast<byte>((bgPalette_ & 0x30) >> 4),
		static_cast<byte>((bgPalette_ & 0xC0) >> 6)
	};

	bool windowScanlineRendered = false;
	for (int i = 0; i < 160; ++i)
	{
		if (ly_ < winy_) return;
		if (i < (winx_ - 7)) continue;
		if (static_cast<int>(winx_) - 7 >= 160) continue;
		if (winy_ >= 144) return;

		word wrappedPixelXCoord = ((i - (static_cast<int>(winx_) - 7)) % 0x100);   // Makes sure X coord is in the [0-255] range after accounting for scrolling
		word wrappedPixelYCoord = (winLy_) % 0x100; // Makes sure Y coord is in the [0-255] range after accounting for scrolling

		word bgXCoord = wrappedPixelXCoord >> 3;                                   // Transforms X coord into BG map coord (dividing by 8 as the bg map is a 32x32 grid)
		word bgYCoord = wrappedPixelYCoord >> 3;                                   // Transforms Y coord into BG map coord (dividing by 8 as the bg map is a 32x32 grid)

		if (cgbType_ != Cartridge::CgbType::DMG)
		{
			// Bit 7    BG-to-OAM Priority         (0=Use OAM Priority bit, 1=BG Priority)
			// Bit 6    Vertical Flip(0 = Normal, 1 = Mirror vertically)
			// Bit 5    Horizontal Flip(0 = Normal, 1 = Mirror horizontally)
			// Bit 4    Not used
			// Bit 3    Tile VRAM Bank number(0 = Bank 0, 1 = Bank 1)
			// Bit 2 - 0  Background Palette number(BGP0 - 7)
			byte tileAttributes = cgbVram_[0x3800 + startingWindowMapAddress + ((bgYCoord * 0x20) + bgXCoord) - 0x9800];

			bool bgToOAMPriority = IS_BIT_SET(7, tileAttributes);
			bool verticalFlip = IS_BIT_SET(6, tileAttributes);
			bool horizontalFlip = IS_BIT_SET(5, tileAttributes);
			bool useTileDataFromVRAMBank0 = !IS_BIT_SET(3, tileAttributes);
			byte cgbPaletteNumber = tileAttributes & 0x7;

			word tileId = cgbVram_[0x1800 + startingWindowMapAddress + ((bgYCoord * 0x20) + bgXCoord) - 0x9800]; // Find tile id based on the above coords

			word tileAddress = startingBgAndWindowTileDataAddress + 16 * tileId;           // Find tile address based on Tile ID. Each tile data occupies 16 bytes

			if (signedTileAddressing)
				tileAddress = startingBgAndWindowTileDataAddress + (16 * static_cast<sbyte>(tileId));

			if (!useTileDataFromVRAMBank0)
				tileAddress = tileAddress - 0x8000 + 0x2000; // Remove 0x800 offset for tile data, but add vram bank 1's offset as well
			else
				tileAddress = tileAddress - 0x8000;

			byte tileCol = horizontalFlip ? (7 - (wrappedPixelXCoord % 8)) : wrappedPixelXCoord % 8; // Get tile data col to draw
			byte tileRow = verticalFlip ? (7 - (wrappedPixelYCoord % 8)) : wrappedPixelYCoord % 8; // Get tile data row to draw	

			word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
			byte targetTileDataFirstByte = cgbVram_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
			byte targetTileDataSecondByte = cgbVram_[targetTileRowAddress + 1];     // Get second byte of tile data row (most significant bits for the final color index)

			byte colorIndexLSB = (targetTileDataFirstByte >> (7 - tileCol)) & 0x01;  // Get target bit from first byte
			byte colorIndexMSB = (targetTileDataSecondByte >> (7 - tileCol)) & 0x01; // Get target bit from second byte

			byte colorIndex = colorIndexMSB << 1 | colorIndexLSB; // Assemble final color index

			bgAndWindowColorIndices[ly_ * 160 + i] = colorIndex;
			cgbBgTopLevelPriorityPixels[ly_ * 160 + i] = bgToOAMPriority;

			// Palette Ram is 64 bytes. Each palette 0..7 takes up 8 bytes.
			// PaletteN 8 bytes distribution:
			// byte 0 - Color 0 high byte (blue & upper green data) where -> bits 6,5,4,3,2 (blue) bits 1,0 (high green)
			// byte 1 - Color 0 low byte  (lower green data & red)  where -> bits 7,6,5 (low green) bits 4,3,2,1,0 (red)
			// byte 2 - Color 1 high byte (blue & upper green data) where -> bits 6,5,4,3,2 (blue) bits 1,0 (high green)
			// .....
			// byte 7 - Color 3 low byte (lower green data & red)   where -> bits 7,6,5 (low green) bits 4,3,2,1,0 (red)
			byte colorHighByte = cgbBackgroundPaletteRam_[cgbPaletteNumber * 8 + (colorIndex * 2) + 1];
			byte colorLowByte = cgbBackgroundPaletteRam_[cgbPaletteNumber * 8 + (colorIndex * 2) + 0];

			float fracBlueIntensity = ((colorHighByte & 0x7C) >> 2) / static_cast<float>(0x1F);
			float fracGreenIntensity = ((((colorHighByte & 0x3) << 3) | ((colorLowByte & 0xE0) >> 5))) / static_cast<float>(0x1F);
			float fracRedIntensity = ((colorLowByte & 0x1F) >> 0) / static_cast<float>(0x1F);

			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 0] = 0xFF;                                         // A
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 1] = static_cast<byte>(fracBlueIntensity * 0xFF);  // B
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 2] = static_cast<byte>(fracGreenIntensity * 0xFF); // G
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 3] = static_cast<byte>(fracRedIntensity * 0xFF);   // R
		}
		else // DMG
		{
			word tileId = mainMemoryBlock_[startingWindowMapAddress + ((bgYCoord * 0x20) + bgXCoord)]; // Find tile id based on the above coords
			word tileAddress = startingBgAndWindowTileDataAddress + 16 * tileId;       // Find tile address based on Tile ID. Each tile data occupies 16 bytes

			if (signedTileAddressing)
				tileAddress = startingBgAndWindowTileDataAddress + (16 * static_cast<sbyte>(tileId));

			byte tileCol = wrappedPixelXCoord % 8; // Get tile data col to draw
			byte tileRow = wrappedPixelYCoord % 8; // Get tile data row to draw

			word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
			byte targetTileDataFirstByte = mainMemoryBlock_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
			byte targetTileDataSecondByte = mainMemoryBlock_[targetTileRowAddress + 1];     // Get second byte of tile data row (most significant bits for the final color index)

			byte colorIndexLSB = (targetTileDataFirstByte >> (7 - tileCol)) & 0x01;  // Get target bit from first byte
			byte colorIndexMSB = (targetTileDataSecondByte >> (7 - tileCol)) & 0x01; // Get target bit from second byte

			byte colorIndex = colorIndexMSB << 1 | colorIndexLSB; // Assemble final color index

			bgAndWindowColorIndices[ly_ * 160 + i] = palette[colorIndex];

			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 0] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][0];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 1] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][1];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 2] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][2];
			finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 3] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][3];

		}

		windowScanlineRendered = true;
	}

	if (windowScanlineRendered)
		winLy_++;
}

void Display::renderOBJsScanline()
{
	word startingOBJTileDataAddress = 0x8000;
	bool xlSprites = IS_BIT_SET(2, lcdControl_);

	// Assemble current palette for OBJs. Bottom 2 bits are ignored because color index 0 is transparent for OBJs
	byte obj0palette[4] =
	{
		static_cast<byte>((obj0Palette_ & 0x03) >> 0),
		static_cast<byte>((obj0Palette_ & 0x0C) >> 2),
		static_cast<byte>((obj0Palette_ & 0x30) >> 4),
		static_cast<byte>((obj0Palette_ & 0xC0) >> 6)
	};

	// Assemble current palette for OBJs. Bottom 2 bits are ignored because color index 0 is transparent for OBJs
	byte obj1palette[4] =
	{
		static_cast<byte>((obj1Palette_ & 0x03) >> 0),
		static_cast<byte>((obj1Palette_ & 0x0C) >> 2),
		static_cast<byte>((obj1Palette_ & 0x30) >> 4),
		static_cast<byte>((obj1Palette_ & 0xC0) >> 6)
	};

	for (word objAddress : selectedOBJAddressesForCurrentScanline_)
	{
		byte objYPos      = mainMemoryBlock_[objAddress + 0] - 16;
		byte objXPos      = mainMemoryBlock_[objAddress + 1] - 8;
		byte objTileIndex = mainMemoryBlock_[objAddress + 2];
		byte objFlags     = mainMemoryBlock_[objAddress + 3];

		bool bgAndWindowOverObj = IS_BIT_SET(7, objFlags);
		bool isHorFlipped       = IS_BIT_SET(5, objFlags);
		bool isVerFlipped       = IS_BIT_SET(6, objFlags);
		bool useObjPalette0     = !IS_BIT_SET(4, objFlags);
		bool cgbUseVramBank0    = !IS_BIT_SET(3, objFlags);
		byte cgbPaletteNumber   = objFlags & 0x07;

		word tileAddress = startingOBJTileDataAddress + 16 * objTileIndex; // Find tile address based on Tile ID. Each tile data occupies 16 bytes

		if (xlSprites)
		{
			tileAddress = startingOBJTileDataAddress + 16 * (objTileIndex & 0xFE); // XL sprites ignore bottom bit of tile index
			if (ly_ >= objYPos + 8)
			{
				tileAddress += 16;
			}
		}

		byte tileRow = (ly_ - objYPos) % 8;
		if (isVerFlipped)
		{
			if (xlSprites)
			{
				if (ly_ >= objYPos + 8)
				{
					tileAddress -= 16;
				}
				else
				{
					tileAddress += 16;
				}
			}
			
			tileRow = 7 - ((ly_ - objYPos) % 8);
		}

		word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
		byte targetTileDataFirstByte = mainMemoryBlock_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
		byte targetTileDataSecondByte = mainMemoryBlock_[targetTileRowAddress + 1]; // Get second byte of tile data row (most significant bits for the final color index)

		if (cgbType_ != Cartridge::CgbType::DMG)
		{
			targetTileDataFirstByte = cgbUseVramBank0 ? cgbVram_[targetTileRowAddress - 0x8000] : cgbVram_[(targetTileRowAddress - 0x8000) + 0x2000];
			targetTileDataSecondByte = cgbUseVramBank0 ? cgbVram_[targetTileRowAddress - 0x8000 + 1] : cgbVram_[(targetTileRowAddress - 0x8000) + 0x2000 + 1];
		}

		for (int j = 0; j < 8; ++j)
		{
			byte colorIndexLSB = (targetTileDataFirstByte >> (7 - j)) & 0x01;  // Get target bit from first byte
			byte colorIndexMSB = (targetTileDataSecondByte >> (7 - j)) & 0x01; // Get target bit from second byte

			if (isHorFlipped)
			{
				colorIndexLSB = (targetTileDataFirstByte >> j) & 0x01;  // Get target bit from first byte
				colorIndexMSB = (targetTileDataSecondByte >> j) & 0x01; // Get target bit from second byte
			}

			byte colorIndex = colorIndexMSB << 1 | colorIndexLSB; // Assemble final color index

			byte pixelCoordY = ly_;
			byte pixelCoordX = objXPos + j;

			if (pixelCoordX >= 160 || pixelCoordY >= 144)
			{
				continue;
			}

			// If BG and Window over obj and the current written pixel color by BG/window is not transparent
			// then this object is skipped
			if (bgAndWindowOverObj && bgAndWindowColorIndices[pixelCoordY * 160 + pixelCoordX] != 0)
			{
				continue;
			}

			// Ignore transparent pixels
			byte finalColorIndex = useObjPalette0 ? obj0palette[colorIndex] : obj1palette[colorIndex];
			if (colorIndex == 0)
			{
				continue;
			}

			if (cgbType_ == Cartridge::CgbType::DMG)
			{
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 0] = GAMEBOY_NATIVE_COLORS[finalColorIndex][0];
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 1] = GAMEBOY_NATIVE_COLORS[finalColorIndex][1];
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 2] = GAMEBOY_NATIVE_COLORS[finalColorIndex][2];
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 3] = GAMEBOY_NATIVE_COLORS[finalColorIndex][3];
			}
			else
			{
				// If a top level priority bg tile is at this pixel, skip
				if (cgbBgTopLevelPriorityPixels[pixelCoordY * 160 + pixelCoordX])
				{
					continue;
				}

				// Palette Ram is 64 bytes. Each palette 0..7 takes up 8 bytes.
				// PaletteN 8 bytes distribution:
				// byte 0 - Color 0 low byte (lower green data & red)  where -> bits 7,6,5 (low green) bits 4,3,2,1,0 (red)
				// byte 1 - Color 0 high byte (blue & upper green data) where -> bits 6,5,4,3,2 (blue) bits 1,0 (high green)
				// byte 2 - Color 1 low byte (lower green data & red)  where -> bits 7,6,5 (low green) bits 4,3,2,1,0 (red)
				// .....
				// byte 7 - Color 3 high byte (blue & upper green data) where -> bits 6,5,4,3,2 (blue) bits 1,0 (high green)
				byte colorHighByte = cgbOBJPaletteRam_[cgbPaletteNumber * 8 + (colorIndex * 2) + 1];
				byte colorLowByte  = cgbOBJPaletteRam_[cgbPaletteNumber * 8 + (colorIndex * 2)];

				float fracBlueIntensity  = ((colorHighByte & 0x7C) >> 2) / static_cast<float>(0x1F);
				float fracGreenIntensity = (((colorHighByte & 0x3) << 3) | ((colorLowByte & 0xE0) >> 5)) / static_cast<float>(0x1F);
				float fracRedIntensity   = ((colorLowByte  & 0x1F) >> 0) / static_cast<float>(0x1F);

				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 0] = 0xFF;                                         // A
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 1] = static_cast<byte>(fracBlueIntensity * 0xFF);  // B
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 2] = static_cast<byte>(fracGreenIntensity * 0xFF); // G
				finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 3] = static_cast<byte>(fracRedIntensity * 0xFF);   // R
			}
		}
	}
}

void Display::searchOBJSInCurrentScanline()
{
	selectedOBJAddressesForCurrentScanline_.clear();

	bool xlSprites = IS_BIT_SET(2, lcdControl_);

	// Scan all OAM memories 4-byte obj entries
	for (word i = Memory::OAM_START_ADDRESS; i < Memory::OAM_END_ADDRESS; i += 4)
	{
		byte objYPos = mainMemoryBlock_[i] - 16;

		if (xlSprites) // 8x16 case
		{
			if (ly_ >= objYPos && ly_ < objYPos + 16) 
				selectedOBJAddressesForCurrentScanline_.insert(selectedOBJAddressesForCurrentScanline_.begin(), i);
		}
		else // 8x8 case
		{
			if (ly_ >= objYPos && ly_ < objYPos + 8) 
				selectedOBJAddressesForCurrentScanline_.insert(selectedOBJAddressesForCurrentScanline_.begin(), i);
		}

		if (selectedOBJAddressesForCurrentScanline_.size() == 10) break;
	}
	
	// Bubble sort for x priority
	int count = static_cast<int>(selectedOBJAddressesForCurrentScanline_.size());
	for (int i = 0; i < count - 1; ++i)
	{
		for (int j = 0; j < count - i - 1; ++j)
		{
			byte lhsX = mainMemoryBlock_[selectedOBJAddressesForCurrentScanline_[j] + 1];
			byte rhsX = mainMemoryBlock_[selectedOBJAddressesForCurrentScanline_[j + 1] + 1];
			
			if (lhsX < rhsX)
			{
				word temp = selectedOBJAddressesForCurrentScanline_[j + 1];
				selectedOBJAddressesForCurrentScanline_[j + 1] = selectedOBJAddressesForCurrentScanline_[j];
				selectedOBJAddressesForCurrentScanline_[j] = temp;
			}
		}
	}
}

void Display::compareLYtoLYC()
{
	if (ly_ == lyc_)
	{
		SET_BIT(2, lcdStatus_);
		if (IS_BIT_SET(6, lcdStatus_))
		{
			cpu_->triggerInterrupt(CPU::LCD_STAT_INTERRUPT_BIT);
		}
	}
	else
	{
		RESET_BIT(2, lcdStatus_);
	}
}
