#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

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
#define REGISTER_IF_ADDR 0xFF0F
extern byte* Register_IF;

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

#define REGISTER_IE_ADDR 0xFFFF
extern byte* Register_IE;

//Interrupts
#define NUM_INTERRUPTS 5

enum Interrupt
{
    Interrupt_VBlank = 1 << 0,
    Interrupt_LCD = 1 << 1,
    Interrupt_Timer = 1 << 2,
    //Interrupt_Serial = 1 << 3,
    Interrupt_Joypad = 1 << 3
};

enum InterruptOp
{
    InterruptOp_VBlank = 0x40,
    InterruptOp_LCD = 0x48,
    InterruptOp_Timer = 0x50,
    //InterruptOp_Serial = 0x58,
    InterruptOp_Joypad = 0x60
};

//Graphics Stuff
#define BACKGROUND_RES_X 256
#define BACKGROUND_RES_Y 256

#define SCREEN_RES_X 160
#define SCREEN_RES_Y 144

#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define BYTES_PER_TILE 16

static const int BACKGROUND_TILES_PER_LINE = BACKGROUND_RES_X / TILE_WIDTH;

bool SystemInit(const char* pRomFile);
void SystemTick(uint32_t dt);

#endif
