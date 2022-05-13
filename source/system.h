#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

#define CLOCK_CYCLES 4194304

#define MEM_SIZE 64*1024

#define ROM_ADDR 0
#define ROM_SIZE 32*1024

#define VRAM_ADDR 0x8000
#define VRAM_SIZE 8*1024

#define RAM_ADDR 0xC000
#define RAM_SIZE 8*1024

//Addressable Memory
extern byte Mem[];

//I/O Registers
#define REGISTER_LCDC_ADDR 0xFF40
extern byte* Register_LCDC;

#define REGISTER_SCY_ADDR 0xFF42
extern byte* Register_SCY;

#define REGISTER_SCX_ADDR 0xFF43
extern byte* Register_SCX;

#define REGISTER_LY_ADDR 0xFF44
extern byte* Register_LY;

#define REGISTER_WY_ADDR 0xFF4A
extern byte* Register_WY;

#define REGISTER_WX_ADDR 0xFF4B
extern byte* Register_WX;

//Graphics Stuff
#define BACKGROUND_RES_X 256
#define BACKGROUND_RES_Y 256

#define SCREEN_RES_X 160
#define SCREEN_RES_Y 144

#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define BYTES_PER_TILE 16

static const int BACKGROUND_TILES_PER_LINE = BACKGROUND_RES_X / TILE_WIDTH;

bool SystemInit();
void SystemTick(uint32_t dt);

bool IsRegisterBitSet(const byte* pR, int bit);
void SetRegisterBit(byte* pR, int bit);
void UnsetRegisterBit(byte* pR, int bit);

#endif
