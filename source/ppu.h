#ifndef PPU_H
#define PPU_H

#include "types.h"

enum Colour
{
    ColourWhite = 0,
    ColourLightGrey = 1,
    ColourDarkGrey = 2,
    ColourBlack = 3
};

enum Colour PPUGetTilePixColour(byte* pTileData, int x, int y);

const enum Colour* PPUGetScreenBuffer();

bool PPUInit();
void PPUTick(cycles numCycles);

#endif
