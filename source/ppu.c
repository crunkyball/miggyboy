#include "ppu.h"
#include "system.h"

bool PPUInit()
{
	return true;
}

int ScanLine = 0;
int CycleCounter = 0;

#define NUM_SCANLINES 154	//0-143 for resolution, 144-153 for vblank
#define CYCLES_PER_FRAME 70224

static const cycles CYCLES_PER_SCANLINE = CYCLES_PER_FRAME / NUM_SCANLINES;

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
}
