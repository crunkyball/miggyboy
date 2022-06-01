#include <SDL.h>
#include <SDL_ttf.h>
#include <Windows.h>
#include <stdio.h>

#include "platform_app.h"
#include "system.h"
#include "cpu.h"
#include "utils.h"

#include "debug.h"

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

static int ProgramOffset = 0;

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

static void DrawDebugInfo()
{
    static const int kTextGap = 16;

    const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

    //Program
    int textY = 5;
    int textX = 170;
    DrawDebugText(textX, textY, 0x000000FF, "Program");

    int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC);
    disassemblyIdx = disassemblyIdx >= 0 ? disassemblyIdx + ProgramOffset : disassemblyIdx;

    for (int i = 0; i < CALLSTACK_SIZE; ++i)
    {
        const char* pOpStr = "???";
        uint16_t opAddr = 0;

        if (disassemblyIdx >= 0 && disassemblyIdx < DisassembledROMSize)
        {
            pOpStr = DisassembledROM[disassemblyIdx].OpStr;
            opAddr = DisassembledROM[disassemblyIdx].ROMAddr;

            disassemblyIdx++;
        }

        DrawDebugText(textX, textY += kTextGap, 0x000000FF, "%s0x%.4X: %s", opAddr == cpuRegisters->PC ? ">" : " ", opAddr, pOpStr);
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
    DrawDebugText(textX, textY, 0x000000FF, "Registers");
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "A: 0x%.2X", cpuRegisters->A);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "B: 0x%.2X", cpuRegisters->B);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "C: 0x%.2X", cpuRegisters->C);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "D: 0x%.2X", cpuRegisters->D);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "E: 0x%.2X", cpuRegisters->E);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "H: 0x%.2X", cpuRegisters->H);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "L: 0x%.2X", cpuRegisters->L);

    //Flags
    textY = 150;
    textX = 100;
    DrawDebugText(textX, textY, 0x000000FF, "Flags");
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "Z: %d", (cpuRegisters->F & (1 << 7)) == 0 ? 0 : 1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "S: %d", (cpuRegisters->F & (1 << 6)) == 0 ? 0 : 1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "H: %d", (cpuRegisters->F & (1 << 5)) == 0 ? 0 : 1);
    DrawDebugText(textX, textY += kTextGap, 0x000000FF, "C: %d", (cpuRegisters->F & (1 << 4)) == 0 ? 0 : 1);


    //Instructions
    DrawDebugText(10, 450, 0x0000FF, "(P) Toggle Single Step Mode, ([) Step CPU, (-/=) Browse Program");
}

#endif

static void DrawBackground()
{
    //Need to do background select and whatnot.
    /*for (byte* addr = &Mem[0x9800]; addr <= &Mem[0x9BFF]; ++addr)
    {
        byte id = *addr;
    }*/

    for (int y = 0; y < SCREEN_RES_Y; ++y)
    {
        for (int x = 0; x < SCREEN_RES_X; ++x)
        {
            int backgroundX = x + *Register_SCX;
            int backgroundY = y + *Register_SCY;

            int tileX = backgroundX / TILE_WIDTH;
            int tileY = backgroundY / TILE_HEIGHT;
            int tileIdx = (tileY * BACKGROUND_TILES_PER_LINE) + tileX;

            byte* TileLayout = &Mem[0x9800];
            byte* TileData = &Mem[0x8000];

            byte tileId = TileLayout[tileIdx];

            int tilePixX = backgroundX % TILE_WIDTH;
            int tilePixY = backgroundY % TILE_HEIGHT;

            byte* pTileData = &TileData[tileId * BYTES_PER_TILE];
            byte* pLineData = &pTileData[tilePixY * 2];	//2 bytes per line.

            //Need to do colour select.
            byte bit = 1 << (7 - tilePixX);
            byte bit1 = (*pLineData & bit) == 0 ? 0 : 1;
            byte bit2 = (*(pLineData + 1) & bit) == 0 ? 0 : 2;

            byte col = bit1 | bit2;

            if (col != 0)
            {
                SDL_SetRenderDrawColor(WindowRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
            }
            else
            {
                SDL_SetRenderDrawColor(WindowRenderer, 0x00, 0x00, 0x00, 0xFF);
            }

            SDL_RenderDrawPoint(WindowRenderer, x, y);
        }
    }
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

        if (e.type == SDL_KEYUP)
        {
            switch (e.key.keysym.sym)
            {
#if DEBUG_ENABLED
                case SDLK_p: ToggleSingleStepMode(); break;
                case SDLK_LEFTBRACKET: RequestSingleStep(); break;
                case SDLK_MINUS: ProgramOffset--; break;
                case SDLK_EQUALS: ProgramOffset++; break;
#endif
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
    DrawBackground();

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
