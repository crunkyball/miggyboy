#include <stdlib.h>

#include "system.h"
#include "debug.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_app.h)

void Run()
{
    uint32_t lastTime = AppGetTimeMS();

    for (;;)
    {
        uint32_t timeNow = AppGetTimeMS();
        uint32_t dt = timeNow - lastTime;
        lastTime = timeNow;

        SystemTick(dt);

        if (!AppTick())
        {
            break;
        }

        //Currently this will render as fast as possible, which isn't what we want.
        AppPreRender();
        AppRender();
        AppPostRender();
    }
}

int main(int argc, char** argv)
{
    const char* pRomFile = NULL;
    
    if (argc > 1)
    {
        pRomFile = argv[1];
    }

    if (!SystemInit(pRomFile))
    {
        return -1;
    }

    if (!AppInit())
    {
        return -1;
    }

#if DEBUG_ENABLED
    DebugInit();

    if (argc > 2)
    {        
        char* pRet;
        uint16_t bp = (uint16_t)strtoul(argv[2], &pRet, 16);
        DebugToggleBreakpoint(bp);
    }
#endif

    Run();

    AppDestroy();

    return 0;
}
