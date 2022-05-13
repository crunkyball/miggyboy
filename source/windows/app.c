#include <SDL.h>
#include <Windows.h>

#include "app.h"
#include "system.h"

#include "utils.h"

static LARGE_INTEGER startTime;
static LARGE_INTEGER frequency;

static SDL_Window* Window;
static SDL_Renderer* WindowRenderer;

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
				SDL_RenderDrawPoint(WindowRenderer, x, y);
			}
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

	Window = SDL_CreateWindow("MiggyBoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_RES_X, SCREEN_RES_Y, SDL_WINDOW_OPENGL);

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
	}

	return true;
}

void AppPreRender()
{
	SDL_SetRenderDrawColor(WindowRenderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(WindowRenderer);
}

void AppRender()
{
	SDL_SetRenderDrawColor(WindowRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

	DrawBackground();
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
