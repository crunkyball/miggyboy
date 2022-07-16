#include <stddef.h>
#include <sys/time.h>
#include <SDL2/SDL.h>

#include "system.h"
#include "ppu.h"
#include "platform_app.h"

static DirectionInputCallbackFunc DirectionInputCallback = NULL;
static ButtonInputCallbackFunc ButtonInputCallback = NULL;

static struct timeval StartTime;

static SDL_Window* Window;
static SDL_Renderer* WindowRenderer;

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

bool AppInit()
{
    gettimeofday(&StartTime, NULL);

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

        if (e.type == SDL_KEYDOWN)
        {
            switch (e.key.keysym.sym)
            {
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
}

void AppPostRender()
{
    SDL_RenderPresent(WindowRenderer);
}

void AppRegisterDirectionInputCallback(DirectionInputCallbackFunc callback)
{
    DirectionInputCallback = callback;
}

void AppRegisterButtonInputCallback(ButtonInputCallbackFunc callback)
{
    ButtonInputCallback = callback;
}

uint32_t AppGetTimeMS()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - StartTime.tv_sec) * 1000 + (now.tv_usec - StartTime.tv_usec) / 1000;
}
