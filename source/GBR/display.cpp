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

static constexpr unsigned int SCANLINE_DOTS            = 456;
static constexpr unsigned int HBLANK_DOTS              = 204; 
static constexpr unsigned int VBLANK_DOTS              = 4560;
static constexpr unsigned int SEARCHING_OAM_DOTS       = 80;
static constexpr unsigned int TRANSFERRING_TO_LCD_DOTS = 172;
static constexpr unsigned int TOTAL_PER_FRAME_DOTS     = 70224;

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
	Those specify the top-left coordinates of the visible 160×144 pixel area within 
	the 256×256 pixels BG map. Values in the range 0–255 may be used (R/W).
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
	the “LYC=LY” flag in the STAT register is set, and (if enabled) a STAT interrupt is requested. (R/W)
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
	System Core Colors
*/
static const byte GAMEBOY_NATIVE_COLORS[4][4] =
{
	//A   //B   //G   //R
	0xFF, 0xD0, 0xF8, 0xE0,
	0xFF, 0x70, 0xC0, 0x88,
	0xFF, 0x56, 0x68, 0x34,
	0xFF, 0x20, 0x18, 0x08
};

Display::Display()
	: mem_(nullptr)
	, clock_(VBLANK_DOTS)
	, totalFrameClock_(0)
	, dmaClockCyclesRemaining_(0)
	, dmaSourceAddressStart_(0)
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
				mem_[Memory::OAM_START_ADDRESS + i] = mem_[dmaSourceAddressStart_ + i];
			}
		}
		return;
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
			return mem_[address];
		else
			return cgbVram_[(address - Memory::VRAM_START_ADDRESS) + cgbVramBank_ * 0x2000];
	}
	if (address >= Memory::OAM_START_ADDRESS && address <= Memory::OAM_END_ADDRESS)
	{
		if (GET_DISPLAY_MODE() == DISPLAY_MODE_SEARCHING_OAM || GET_DISPLAY_MODE() == DISPLAY_MODE_TRANSFERRING_TO_LCD)
		{
			log(LogType::WARNING, "Attempt to read from OAM during LCD transfer or searching phase. Returning garbage.");
			if (respectIllegalReadsWrites_) return 0xFF;
		}
		return mem_[address];
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
			mem_[address] = b;
		else
			cgbVram_[(address - Memory::VRAM_START_ADDRESS) + cgbVramBank_ * 0x2000] = b;
		return;
	}
	if (address >= Memory::OAM_START_ADDRESS && address <= Memory::OAM_END_ADDRESS)
	{
		if (GET_DISPLAY_MODE() == DISPLAY_MODE_SEARCHING_OAM || GET_DISPLAY_MODE() == DISPLAY_MODE_TRANSFERRING_TO_LCD)
		{
			log(LogType::WARNING, "Attempt to write to OAM during LCD transfer or searching phase. Ignoring write.");			
			if (respectIllegalReadsWrites_) return;
		}
		mem_[address] = b;
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
		default: log(LogType::WARNING, "Display::writeByteAt Unknown write %s at %s", getHexByte(b).c_str(), getHexWord(address).c_str());
	}
}

void Display::performDMATransfer(const byte b)
{
	dmaClockCyclesRemaining_ = DMA_CLOCK_CYCLES;
	dmaSourceAddressStart_ = b << 8;
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

		word bgXCoord = wrappedPixelXCoord >> 3;                                   // Transforms X coord into BG map coord (dividing by 8 as the bg map is a 32x32 grid)
		word bgYCoord = wrappedPixelYCoord >> 3;                                   // Transforms Y coord into BG map coord (dividing by 8 as the bg map is a 32x32 grid)

		word tileId = mem_[startingBgMapAddress + ((bgYCoord * 0x20) + bgXCoord)]; // Find tile id based on the above coords
		word tileAddress = startingBgAndWindowTileDataAddress + 16 * tileId;       // Find tile address based on Tile ID. Each tile data occupies 16 bytes

		if (signedTileAddressing)
			tileAddress = startingBgAndWindowTileDataAddress + (16 * static_cast<sbyte>(tileId));

		byte tileCol = wrappedPixelXCoord % 8; // Get tile data col to draw
		byte tileRow = wrappedPixelYCoord % 8; // Get tile data row to draw

		word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
		byte targetTileDataFirstByte = mem_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
		byte targetTileDataSecondByte = mem_[targetTileRowAddress + 1];     // Get second byte of tile data row (most significant bits for the final color index)

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

		word tileId = mem_[startingWindowMapAddress + ((bgYCoord * 0x20) + bgXCoord)]; // Find tile id based on the above coords
		word tileAddress = startingBgAndWindowTileDataAddress + 16 * tileId;       // Find tile address based on Tile ID. Each tile data occupies 16 bytes

		if (signedTileAddressing)
			tileAddress = startingBgAndWindowTileDataAddress + (16 * static_cast<sbyte>(tileId));

		byte tileCol = wrappedPixelXCoord % 8; // Get tile data col to draw
		byte tileRow = wrappedPixelYCoord % 8; // Get tile data row to draw

		word targetTileRowAddress = tileAddress + (tileRow * 2 /* 2 bytes per tile data row */);
		byte targetTileDataFirstByte = mem_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
		byte targetTileDataSecondByte = mem_[targetTileRowAddress + 1];     // Get second byte of tile data row (most significant bits for the final color index)

		byte colorIndexLSB = (targetTileDataFirstByte >> (7 - tileCol)) & 0x01;  // Get target bit from first byte
		byte colorIndexMSB = (targetTileDataSecondByte >> (7 - tileCol)) & 0x01; // Get target bit from second byte

		byte colorIndex = colorIndexMSB << 1 | colorIndexLSB; // Assemble final color index

		bgAndWindowColorIndices[ly_ * 160 + i] = palette[colorIndex];
		
		finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 0] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][0];
		finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 1] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][1];
		finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 2] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][2];
		finalSDLPixels_[(ly_ * 160 * 4) + (i * 4) + 3] = GAMEBOY_NATIVE_COLORS[palette[colorIndex]][3];

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
		byte objYPos      = mem_[objAddress + 0] - 16;
		byte objXPos      = mem_[objAddress + 1] - 8;
		byte objTileIndex = mem_[objAddress + 2];
		byte objFlags     = mem_[objAddress + 3];

		bool bgAndWindowOverObj = IS_BIT_SET(7, objFlags);
		bool isHorFlipped       = IS_BIT_SET(5, objFlags);
		bool isVerFlipped       = IS_BIT_SET(6, objFlags);
		bool useObjPalette0     = !IS_BIT_SET(4, objFlags);

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
		byte targetTileDataFirstByte = mem_[targetTileRowAddress];          // Get first byte of tile data row (least significant bits for the final color index)
		byte targetTileDataSecondByte = mem_[targetTileRowAddress + 1]; // Get second byte of tile data row (most significant bits for the final color index)

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

			finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 0] = GAMEBOY_NATIVE_COLORS[finalColorIndex][0];
			finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 1] = GAMEBOY_NATIVE_COLORS[finalColorIndex][1];
			finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 2] = GAMEBOY_NATIVE_COLORS[finalColorIndex][2];
			finalSDLPixels_[(ly_ * 160 * 4) + (pixelCoordX * 4) + 3] = GAMEBOY_NATIVE_COLORS[finalColorIndex][3];
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
		byte objYPos = mem_[i] - 16;

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
			byte lhsX = mem_[selectedOBJAddressesForCurrentScanline_[j] + 1];
			byte rhsX = mem_[selectedOBJAddressesForCurrentScanline_[j + 1] + 1];
			
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