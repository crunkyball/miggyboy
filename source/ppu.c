#include <assert.h>

#include "ppu.h"
#include "system.h"

int CycleCounter = 0;

#define NUM_SCANLINES 154	//0-143 for resolution, 144-153 for vblank
#define CYCLES_PER_FRAME 70224

static const cycles CYCLES_PER_SCANLINE = CYCLES_PER_FRAME / NUM_SCANLINES;

enum Colour ScreenBuffer[SCREEN_RES_X * SCREEN_RES_Y];

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

enum LCDC_Flags
{
    LCDC_BackgroundEnabled = 1 << 0,
    LCDC_SpritesEnabled = 1 << 1,
    LCDC_SpritesSize = 1 << 2,
    LCDC_BackgroundTileMapArea = 1 << 3,
    LCDC_BackgroundTileDataArea = 1 << 4,
    LCDC_WindowEnabled = 1 << 5,
    LCDC_WindowTileMapArea = 1 << 6,
    LCDC_Enabled = 1 << 7
};

static bool BackgroundEnabled() { return (*Register_LCDC & LCDC_BackgroundEnabled) != 0; }
static bool SpritesEnabled() { return (*Register_LCDC & LCDC_SpritesEnabled) != 0; }
static bool LargeSprites() { return (*Register_LCDC & LCDC_SpritesSize) != 0; }
static uint16_t BackgroundTileMapArea() { return (*Register_LCDC & LCDC_BackgroundTileMapArea) != 0 ? VRAM_TILE_MAP_ADDR_1 : VRAM_TILE_MAP_ADDR_0; }
static uint16_t BackgroundTileDataArea() { return (*Register_LCDC & LCDC_BackgroundTileDataArea) != 0 ? VRAM_TILE_DATA_ADDR_0 : VRAM_TILE_DATA_ADDR_1; }
static bool WindowEnabled() { return (*Register_LCDC & LCDC_WindowEnabled) != 0; }
static uint16_t WindowTileMapArea() { return (*Register_LCDC & LCDC_WindowTileMapArea) != 0 ? VRAM_TILE_MAP_ADDR_1 : VRAM_TILE_MAP_ADDR_0; }
static bool LCDEnabled() { return (*Register_LCDC & LCDC_Enabled) != 0; }

struct SpriteAttr
{
    byte YPos;
    byte XPos;
    byte TileId;

    union
    {
        struct  
        {
            byte Unused : 4;    //Used for CGB, if I ever plan to support that.
            byte PaletteNum : 1;
            byte XFlip : 1;
            byte YFlip : 1;
            byte BackgroundPriority : 1;
        };

        byte Flags;
    };
};

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

const enum Colour* PPUGetScreenBuffer()
{
    return ScreenBuffer;
}

static void RenderBackground(byte renderLine)
{
    uint16_t tileMapAddr = BackgroundTileMapArea();
    uint16_t tileDataAddr = BackgroundTileDataArea();

    byte* pTileLayout = AccessMem(tileMapAddr);
    byte* pTileData = AccessMem(tileDataAddr);

    byte y = renderLine;
    enum Colour* pScreenBufferLine = &ScreenBuffer[y * SCREEN_RES_X];

    for (byte x = 0; x < SCREEN_RES_X; ++x)
    {
        byte backgroundX = x + *Register_SCX;
        byte backgroundY = y + *Register_SCY;

        byte tileX = backgroundX / TILE_WIDTH;
        byte tileY = backgroundY / TILE_HEIGHT;
        int tileIdx = (tileY * BACKGROUND_TILES_PER_LINE) + tileX;

        byte tileId = pTileLayout[tileIdx];

        if (tileDataAddr == VRAM_TILE_DATA_ADDR_1)
        {
            //In this case the tiles are addressed signed.
            tileId = ((int8_t)tileId) + 128;
        }

        byte tilePixX = backgroundX % TILE_WIDTH;
        byte tilePixY = backgroundY % TILE_HEIGHT;

        byte* pThisTileData = &pTileData[tileId * BYTES_PER_TILE];

        pScreenBufferLine[x] = PPUGetTilePixColour(pThisTileData, tilePixX, tilePixY);
    }
}

static void RenderWindow(byte renderLine)
{

}

static void RenderSprites(byte renderLine)
{
    byte* pTileData = AccessMem(VRAM_TILE_DATA_ADDR_0);

    if (LargeSprites())
    {
        assert(0);  //TODO
    }

    for (uint16_t addr = VRAM_SPRITE_TABLE_ADDR; addr < VRAM_SPRITE_TABLE_ADDR + VRAM_SPRITE_TABLE_SIZE; addr += sizeof(struct SpriteAttr))
    {
        struct SpriteAttr* pSpriteAttr = (struct SpriteAttr*)AccessMem(addr);

        //Y position is offset by 16, X position is offset by 8.
        static const byte kSpriteXOffset = 8;
        static const byte kSpriteYOffset = 16;

        if (pSpriteAttr->YPos >= kSpriteYOffset && pSpriteAttr->YPos < SCREEN_RES_Y + kSpriteYOffset)
        {
            byte yPos = pSpriteAttr->YPos - kSpriteYOffset;
            byte xPos = pSpriteAttr->XPos - kSpriteXOffset;

            if (renderLine >= yPos && renderLine < yPos + kSpriteXOffset)
            {
                byte* pThisTileData = &pTileData[pSpriteAttr->TileId * BYTES_PER_TILE];
                enum Colour* pScreenBufferLine = &ScreenBuffer[renderLine * SCREEN_RES_X];

                byte spriteY = renderLine - yPos;
                byte pixY = pSpriteAttr->YFlip ? TILE_HEIGHT - (spriteY + 1) : spriteY;

                for (byte spriteX = 0; spriteX < TILE_WIDTH; ++spriteX)
                {
                    byte pixX = pSpriteAttr->XFlip ? TILE_WIDTH - (spriteX + 1) : spriteX;
                    pScreenBufferLine[xPos + spriteX] = PPUGetTilePixColour(pThisTileData, pixX, pixY);
                }
            }
        }
    }
}

static void RenderScanline(byte renderLine)
{
    if (!LCDEnabled())
        return;

    if (BackgroundEnabled())
    {
        RenderBackground(renderLine);
    }

    if (WindowEnabled())
    {
        RenderWindow(renderLine);
    }

    if (SpritesEnabled())
    {
        RenderSprites(renderLine);
    }
}

bool PPUInit()
{
    return true;
}

void PPUTick(cycles numCycles)
{
    byte currentLine = *Register_LY;
    
    if ((CycleCounter += numCycles) >= CYCLES_PER_SCANLINE)
    {
        CycleCounter -= CYCLES_PER_SCANLINE;

        if (*Register_LY >= NUM_SCANLINES)
        {
            *Register_LY = 0;
        }
        else
        {
            (*Register_LY)++;
        }
    }

    enum Mode mode = Mode_HBlank;

    if (*Register_LY > 143)
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

    //Render the line if we've changed.
    if (mode != Mode_VBlank && currentLine != *Register_LY)
    {
        RenderScanline(currentLine);
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
