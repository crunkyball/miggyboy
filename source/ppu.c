#include "ppu.h"
#include "system.h"

bool PPUInit()
{
	return true;
}

int ScanLine = 0;
int CycleCounter = 0;

void PPUTick(cycles numCycles)
{
	//Completely fake for now to get the boot rom working. Set the scanline based on 144/60, which isn't the right thing to do!
	const cycles CYCLES_PER_SCANLINE = 485;

	if ((CycleCounter += numCycles) >= CYCLES_PER_SCANLINE)
	{
		CycleCounter -= CYCLES_PER_SCANLINE;

		if (ScanLine >= 144)
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
