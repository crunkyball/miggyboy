#include "ppu.h"
#include "system.h"

int ScanLine = 0;
int CycleCounter = 0;

#define NUM_SCANLINES 154	//0-143 for resolution, 144-153 for vblank
#define CYCLES_PER_FRAME 70224

static const cycles CYCLES_PER_SCANLINE = CYCLES_PER_FRAME / NUM_SCANLINES;

enum Mode
{
    Mode_HBlank = 0,
    Mode_VBlank = 1,
    Mode_SearchingOAM = 2,
    Mode_TransferringDataToLCD = 3
};

static enum Mode CurrentMode = Mode_HBlank;

#define SEARCHING_OAM_PERIOD 80
//This can take 168-291 cycles, apparently. Does it matter if it's not emulated properly?
#define TRANSFERRING_DATA_TO_LCD_PERIOD (SEARCHING_OAM_PERIOD + 170)

enum Colour PPUGetTilePixColour(byte* pTileData, int x, int y)
{
    byte* pLineData = &pTileData[y * 2];	//2 bytes per line.

    byte bit = 1 << (7 - x);
    byte bit1 = (*pLineData & bit) == 0 ? 0 : 1;
    byte bit2 = (*(pLineData + 1) & bit) == 0 ? 0 : 2;

    byte paletteIndex = bit1 | bit2;

    byte shiftAmount = paletteIndex * 2;
    byte col = (*Register_BGP & (0b11 << shiftAmount)) >> shiftAmount;

    return col;
}

bool PPUInit()
{
    return true;
}

void PPUTick(cycles numCycles)
{
    if ((CycleCounter += numCycles) >= CYCLES_PER_SCANLINE)
    {
        CycleCounter -= CYCLES_PER_SCANLINE;

        if (ScanLine >= NUM_SCANLINES)
        {
            ScanLine = 0;
        }
        else
        {
            ScanLine++;
        }

        *Register_LY = ScanLine;
    }

    enum Mode mode = Mode_HBlank;

    if (ScanLine > 143)
    {
        mode = Mode_VBlank;
    }
    else if (CycleCounter < SEARCHING_OAM_PERIOD)
    {
        mode = Mode_SearchingOAM;
    }
    else if (CycleCounter < TRANSFERRING_DATA_TO_LCD_PERIOD)
    {
        mode = Mode_TransferringDataToLCD;
    }

    if (mode != CurrentMode)
    {
        CurrentMode = mode;
        
        if (CurrentMode == Mode_VBlank)
        {
            FireInterrupt(Interrupt_VBlank);
        }

        //Also need to handle the STAT interrupt.
    }
}
