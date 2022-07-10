#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"
#include "system_types.h"

#define MEM_SIZE 64*1024

#define ROM_ADDR 0
#define ROM_SIZE 32*1024

#define VRAM_ADDR 0x8000
#define VRAM_SIZE 8*1024
#define VRAM_TILE_DATA_ADDR_0 0x8000
#define VRAM_TILE_DATA_ADDR_1 0x8800
#define VRAM_TILE_DATA_SIZE 4*1024
#define VRAM_TILE_MAP_ADDR_0 0x9800
#define VRAM_TILE_MAP_ADDR_1 0x9C00
#define VRAM_TILE_MAP_SIZE 1*1024
#define VRAM_SPRITE_TABLE_ADDR 0xFE00
#define VRAM_SPRITE_TABLE_SIZE 160

#define RAM_ADDR 0xC000
#define RAM_SIZE 8*1024

//Hardware Registers
#define REGISTER_P1_ADDR 0xFF00
extern byte* Register_P1;

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

#define REGISTER_DMA_ADDR 0xFF46
extern byte* Register_DMA;

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

//Graphics Stuff
#define BACKGROUND_RES_X 256
#define BACKGROUND_RES_Y 256

#define SCREEN_RES_X 160
#define SCREEN_RES_Y 144

#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define BYTES_PER_TILE 16

static const int BACKGROUND_TILES_PER_LINE = BACKGROUND_RES_X / TILE_WIDTH;

byte* AccessMem(uint16_t addr);

byte ReadMem(uint16_t addr);
void WriteMem(uint16_t addr, byte val);

uint16_t ReadMem16(uint16_t addr);

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
