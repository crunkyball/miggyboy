#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

#define MEM_SIZE 64*1024

#define ROM_ADDR 0
#define ROM_SIZE 32*1024

#define VRAM_ADDR 0x8000
#define VRAM_SIZE 8*1024
#define VRAM_TILE_DATA_ADDR 0x8000
#define VRAM_TILE_DATA_SIZE 6*1024  //6k for the whole tile data but it's split into two overlapping 4k sections for BG and Window below.
#define VRAM_BG_TILE_DATA_0_ADDR 0x8800
#define VRAM_BG_TILE_DATA_1_ADDR 0x8000
#define VRAM_BG_TILE_DATA_SIZE 4*1024
#define VRAM_TILE_MAP_0_ADDR 0x9800
#define VRAM_TILE_MAP_1_ADDR 0x9C00
#define VRAM_TILE_MAP_SIZE 1*1024

#define RAM_ADDR 0xC000
#define RAM_SIZE 8*1024

//Addressable Memory
extern byte Mem[];

//I/O Registers
#define REGISTER_DIV_ADDR 0xFF04
extern byte* Register_DIV;

#define REGISTER_TIMA_ADDR 0xFF05
extern byte* Register_TIMA;

#define REGISTER_TMA_ADDR 0xFF06
extern byte* REGISTER_TMA;

#define REGISTER_TAC_ADDR 0xFF07
extern byte* REGISTER_TAC;

#define REGISTER_IF_ADDR 0xFF0F
extern byte* Register_IF;

#define REGISTER_LCDC_ADDR 0xFF40
extern byte* Register_LCDC;

#define REGISTER_STAT_ADDR 0xFF41
extern byte* Register_STAT;

#define REGISTER_SCY_ADDR 0xFF42
extern byte* Register_SCY;

#define REGISTER_SCX_ADDR 0xFF43
extern byte* Register_SCX;

#define REGISTER_LY_ADDR 0xFF44
extern byte* Register_LY;

#define REGISTER_BGP_ADDR 0xFF47
extern byte* Register_BGP;

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
    Interrupt_VBlank = 0,
    Interrupt_STAT = 1,
    Interrupt_Timer = 2,
    //Interrupt_Serial = 3,
    Interrupt_Joypad = 4
};

enum InterruptOp
{
    InterruptOp_VBlank = 0x40,
    InterruptOp_STAT = 0x48,
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

void FireInterrupt(enum Interrupt interrupt);

bool SystemInit(const char* pRomFile);
void SystemTick(uint32_t dt);

#if DEBUG_ENABLED
void ToggleSingleStepMode();
void EnableSingleStepMode();
void RequestSingleStep();

void RegisterStepCallback(CallbackFunc callback);
void RegisterROMChangedCallback(CallbackFunc callback);
#endif

#endif
