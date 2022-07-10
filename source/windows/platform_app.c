#include <SDL.h>
#include <SDL_ttf.h>
#include <Windows.h>
#include <stdio.h>

#include "platform_app.h"
#include "cpu.h"
#include "ppu.h"
#include "utils.h"

#include "debug.h"
#include "platform_debug.h"

static DirectionInputCallbackFunc DirectionInputCallback = NULL;
static ButtonInputCallbackFunc ButtonInputCallback = NULL;

static LARGE_INTEGER startTime;
static LARGE_INTEGER frequency;

static SDL_Window* Window;
static SDL_Renderer* WindowRenderer;

#if DEBUG_ENABLED
static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;
#else
static const int WINDOW_WIDTH = SCREEN_RES_X;
static const int WINDOW_HEIGHT = SCREEN_RES_Y;
#endif

#if DEBUG_ENABLED
static TTF_Font* DebugFont;

static int SelectedDisassemblyIndex = 0;
static int DisassemblyOffset = 0;

static void DrawDebugText(int x, int y, uint32_t col, const char* pFmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, pFmt);
    vsprintf(buf, pFmt, args);
    va_end(args);

    SDL_Color textCol = {
        (col & 0xFF000000) >> 24,
        (col & 0x00FF0000) >> 16,
        (col & 0x0000FF00) >> 8,
        (col & 0x000000FF)
    };

    //This is not remotely efficient but it's only for debug.
    SDL_Surface* pTextSurface = TTF_RenderText_Blended(DebugFont, buf, textCol);
    SDL_Texture* pTextTexture = SDL_CreateTextureFromSurface(WindowRenderer, pTextSurface);

    int width, height;
    SDL_QueryTexture(pTextTexture, NULL, NULL, &width, &height);
    SDL_Rect rect = { x, y, width, height };

    SDL_RenderCopy(WindowRenderer, pTextTexture, NULL, &rect);

    SDL_DestroyTexture(pTextTexture);
    SDL_FreeSurface(pTextSurface);
}

static void DecreaseDisassemblyIndex()
{
    if (SelectedDisassemblyIndex == 0)
    {
        const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

        int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC) + DisassemblyOffset;

        if (disassemblyIdx > 0)
        {
            DisassemblyOffset--;
        }
    }
    else
    {
        SelectedDisassemblyIndex--;
    }
}

static void IncreaseDisassemblyIndex()
{
    if (SelectedDisassemblyIndex == (CALLSTACK_SIZE - 1))
    {
        const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

        int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC) + DisassemblyOffset;

        if ((disassemblyIdx + CALLSTACK_SIZE) < DisassembledROMSize)
        {
            DisassemblyOffset++;
        }
    }
    else
    {
        SelectedDisassemblyIndex++;
    }
}

static void ToggleDisassemblyBreakpoint()
{
    const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

    int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC) + DisassemblyOffset + SelectedDisassemblyIndex;
    int breakpointAddr = DisassembledROM[disassemblyIdx].ROMAddr;

    DebugToggleBreakpoint(breakpointAddr);
}

static void OnBreakpointHit()
{
    //Reset the offsets so the breakpoint is in focus.
    SelectedDisassemblyIndex = 0;
    DisassemblyOffset = 0;
}

static void DrawDebugInfo()
{
    static const int kTextGap = 16;

    const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

    //Program
    int textY = 5;
    int textX = 170;
    DrawDebugText(textX, textY, 0x000000FF, "Program");

    int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC) + DisassemblyOffset;

    for (int i = 0; i < CALLSTACK_SIZE; ++i)
    {
        const char* pOpStr = "???";
        uint16_t opAddr = 0;

        textY += kTextGap;

        if (disassemblyIdx >= 0 && disassemblyIdx < DisassembledROMSize)
        {
            pOpStr = DisassembledROM[disassemblyIdx].OpStr;
            opAddr = DisassembledROM[disassemblyIdx].ROMAddr;

            if (DebugHasBreakpoint(opAddr))
            {
                SDL_SetRenderDrawColor(WindowRenderer, 0xFF, 0x00, 0x00, 0xFF);
                SDL_Rect rect = { textX - 6, textY + 2, 12, 12 };
                SDL_RenderFillRect(WindowRenderer, &rect);

                if (opAddr == cpuRegisters->PC)
                {
                    SDL_SetRenderDrawColor(WindowRenderer, 0xFF, 0xFF, 0x4D, 0xFF);
                    SDL_Rect rect = { textX + 7, textY, 193, 16 };
                    SDL_RenderFillRect(WindowRenderer, &rect);
                }
            }

            disassemblyIdx++;

            DrawDebugText(textX, textY, 0x000000FF, "%s0x%.4X: %s", opAddr == cpuRegisters->PC ? ">" : " ", opAddr, pOpStr);
        }

        if (i == SelectedDisassemblyIndex)
        {
            SDL_SetRenderDrawColor(WindowRenderer, 0x00, 0x00, 0x00, 0xFF);
            SDL_Rect rect = { textX + 7, textY, 193, 16 };
            SDL_RenderDrawRect(WindowRenderer, &rect);
        }
    }

    //Callstack
    textY = 5;
    textX = 390;
    DrawDebugText(textX, textY, 0x000000FF, "Callstack");
    for (int i = 0; i < CALLSTACK_SIZE; ++i)
    {
        //int callstackIdx = (DebugCallstackHead + i) % CALLSTACK_SIZE;
        //I think it's better if the head of the callstack is at the top.
        int callstackIdx = ((DebugCallstackHead - (i + 1)) + CALLSTACK_SIZE) % CALLSTACK_SIZE;
        int disassemblyIdx = DebugCallstack[callstackIdx];

        const char* pOpStr = "???";
        uint16_t opAddr = 0;

        if (disassemblyIdx >= 0 && disassemblyIdx < DisassembledROMSize)
        {
            pOpStr = DisassembledROM[disassemblyIdx].OpStr;
            opAddr = DisassembledROM[disassemblyIdx].ROMAddr;
        }

        DrawDebugText(textX, textY += kTextGap, 0x000000FF, "%s0x%.4X: %s", i == 0 ? ">" : " ", opAddr, pOpStr);
    }

    //CPU Registers
    textY = 150;
    textX = 10;
    DrawDebugText(textX, textY, 0x000000FF, "CPU Registers");
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "A: 0x%.2X", cpuRegisters->A);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "B: 0x%.2X", cpuRegisters->B);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "C: 0x%.2X", cpuRegisters->C);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "D: 0x%.2X", cpuRegisters->D);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "E: 0x%.2X", cpuRegisters->E);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "F: 0x%.2X", cpuRegisters->F);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "H: 0x%.2X", cpuRegisters->H);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "L: 0x%.2X", cpuRegisters->L);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "SP: 0x%.4X", cpuRegisters->SP);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "PC: 0x%.4X", cpuRegisters->PC);

    //Hardware Registers
    textY = 320;
    textX = 10;
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "Hardware Registers");
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "P1: 0x%.2X", *Register_P1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "IF: 0x%.2X", *Register_IF);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "LCDC: 0x%.2X", *Register_LCDC);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "IE: 0x%.2X", *Register_IE);

    //Flags
    textY = 180;
    textX = 100;
    DrawDebugText(textX, textY, 0x000000FF, "Flags");
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "Z: %d", (cpuRegisters->F & (1 << 7)) == 0 ? 0 : 1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "S: %d", (cpuRegisters->F & (1 << 6)) == 0 ? 0 : 1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "H: %d", (cpuRegisters->F & (1 << 5)) == 0 ? 0 : 1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "C: %d", (cpuRegisters->F & (1 << 4)) == 0 ? 0 : 1);

    //Tiles
    textY = 160;
    textX = 170;
    DrawDebugText(textX, textY, 0x000000FF, "Tiles");

    const int TILE_POS_START_X = 170;
    const int TILE_POS_START_Y = 180;
    const int TILES_PER_LINE = 32;
    for (int tileAddr = VRAM_TILE_DATA_ADDR_0, tileIdx = 0; tileAddr < (VRAM_TILE_DATA_ADDR_1 + VRAM_TILE_DATA_SIZE); tileAddr += BYTES_PER_TILE, ++tileIdx)
    {
        int tilePosX = TILE_POS_START_X + ((tileIdx % TILES_PER_LINE) * (TILE_WIDTH + 1));
        int tilePosY = TILE_POS_START_Y + ((tileIdx / TILES_PER_LINE) * (TILE_HEIGHT + 1));
       
        for (int tileY = 0; tileY < TILE_HEIGHT; ++tileY)
        {
            for (int tileX = 0; tileX < TILE_WIDTH; ++tileX)
            {
                byte* pMem = AccessMem(tileAddr);
                enum Colour col = PPUGetTilePixColour(pMem, tileX, tileY);

                switch (col)
                {
                    case ColourWhite: SDL_SetRenderDrawColor(WindowRenderer, 0xFF, 0xFF, 0xFF, 0xFF); break;
                    case ColourLightGrey: SDL_SetRenderDrawColor(WindowRenderer, 0x54, 0x54, 0x54, 0xFF); break;
                    case ColourDarkGrey: SDL_SetRenderDrawColor(WindowRenderer, 0xA9, 0xA9, 0xA9, 0xFF); break;
                    case ColourBlack: SDL_SetRenderDrawColor(WindowRenderer, 0x00, 0x00, 0x00, 0xFF); break;
                }

                SDL_RenderDrawPoint(WindowRenderer, tilePosX + tileX, tilePosY + tileY);
            }
        }
    }

    //Instructions
    DrawDebugText(10, 450, 0x0000FF, "(P) Toggle Single Step, ([) Step CPU, (B) Breakpoint, (-/=) Browse Program");
}

#endif

static void RenderPPUScreenBuffer()
{
    const enum Colour* pScreenBuffer = PPUGetScreenBuffer();

    for (int y = 0; y < SCREEN_RES_Y; ++y)
    {
        for (int x = 0; x < SCREEN_RES_X; ++x)
        {
            enum Colour col = pScreenBuffer[(y * SCREEN_RES_X) + x];

            switch (col)
            {
                case ColourWhite: SDL_SetRenderDrawColor(WindowRenderer, 0x7F, 0x86, 0x0F, 0xFF); break;
                case ColourLightGrey: SDL_SetRenderDrawColor(WindowRenderer, 0x57, 0x7C, 0x44, 0xFF); break;
                case ColourDarkGrey: SDL_SetRenderDrawColor(WindowRenderer, 0x36, 0x5D, 0x48, 0xFF); break;
                case ColourBlack: SDL_SetRenderDrawColor(WindowRenderer, 0x2A, 0x45, 0x3B, 0xFF); break;
            }

            SDL_RenderDrawPoint(WindowRenderer, x, y);
        }
    }
}

void AppRegisterDirectionInputCallback(DirectionInputCallbackFunc callback)
{
    DirectionInputCallback = callback;
}

void AppRegisterButtonInputCallback(ButtonInputCallbackFunc callback)
{
    ButtonInputCallback = callback;
}

bool AppInit()
{
    QueryPerformanceCounter(&startTime);
    QueryPerformanceFrequency(&frequency);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return false;
    }

#if DEBUG_ENABLED
    if (TTF_Init() != 0)
    {
        return false;
    }

    DebugFont = TTF_OpenFont("VeraMono.ttf", 14);

    if (DebugFont == NULL)
    {
        return false;
    }

    RegisterBreakpointHitCallback(&OnBreakpointHit);
#endif

    Window = SDL_CreateWindow("MiggyBoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);

    if (Window == NULL)
    {
        return false;
    }

    WindowRenderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);

    if (WindowRenderer == NULL)
    {
        return false;
    }

    SDL_SetRenderDrawColor(WindowRenderer, 0x00, 0x00, 0x00, 0xFF);

    return true;
}

void AppDestroy()
{
    SDL_DestroyRenderer(WindowRenderer);
    SDL_DestroyWindow(Window);

#if DEBUG_ENABLED
    TTF_Quit();
#endif

    SDL_Quit();
}

bool AppTick()
{
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
        if (e.type == SDL_QUIT)
        {
            return false;
        }

        if (e.type == SDL_KEYDOWN)
        {
            switch (e.key.keysym.sym)
            {
#if DEBUG_ENABLED
                case SDLK_p: ToggleSingleStepMode(); break;
                case SDLK_LEFTBRACKET: RequestSingleStep(); break;
                case SDLK_MINUS: DecreaseDisassemblyIndex(); break;
                case SDLK_EQUALS: IncreaseDisassemblyIndex(); break;
                case SDLK_b: ToggleDisassemblyBreakpoint(); break;
                case SDLK_d: DebugDumpDisassembly(); break;
#endif
                case SDLK_UP: DirectionInputCallback(Input_Up, true); break;
                case SDLK_DOWN: DirectionInputCallback(Input_Down, true); break;
                case SDLK_LEFT: DirectionInputCallback(Input_Left, true); break;
                case SDLK_RIGHT: DirectionInputCallback(Input_Right, true); break;
                case SDLK_z: ButtonInputCallback(Input_A, true); break;
                case SDLK_x: ButtonInputCallback(Input_B, true); break;
                case SDLK_s: ButtonInputCallback(Input_Start, true); break;
                case SDLK_a: ButtonInputCallback(Input_Select, true); break;
            }
        }

        if (e.type == SDL_KEYUP)
        {
            switch (e.key.keysym.sym)
            {
                case SDLK_UP: DirectionInputCallback(Input_Up, false); break;
                case SDLK_DOWN: DirectionInputCallback(Input_Down, false); break;
                case SDLK_LEFT: DirectionInputCallback(Input_Left, false); break;
                case SDLK_RIGHT: DirectionInputCallback(Input_Right, false); break;
                case SDLK_z: ButtonInputCallback(Input_A, false); break;
                case SDLK_x: ButtonInputCallback(Input_B, false); break;
                case SDLK_s: ButtonInputCallback(Input_Start, false); break;
                case SDLK_a: ButtonInputCallback(Input_Select, false); break;
            }
        }
    }

    return true;
}

void AppPreRender()
{
    SDL_SetRenderDrawColor(WindowRenderer, 0xCC, 0xCC, 0xCC, 0xFF);
    SDL_RenderClear(WindowRenderer);
}

void AppRender()
{
    RenderPPUScreenBuffer();

#if DEBUG_ENABLED
    DrawDebugInfo();
#endif
}

void AppPostRender()
{
    SDL_RenderPresent(WindowRenderer);
}

uint32_t AppGetTimeMS()
{
    LARGE_INTEGER timeNow;
    QueryPerformanceCounter(&timeNow);

    LARGE_INTEGER elapsed;
    elapsed.QuadPart = timeNow.QuadPart - startTime.QuadPart;

    //elapsed.QuadPart *= 1000000;	//Microseconds.
    elapsed.QuadPart *= 1000;		//Milliseconds.

    return (uint32_t)(elapsed.QuadPart / frequency.QuadPart);
}
