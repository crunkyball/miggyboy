#include <stdlib.h>

#include "system.h"
#include "debug.h"
#include <string.h>

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

#if DEBUG_ENABLED
        DebugTick(dt);
#endif

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
        for (int arg = 2; arg < argc; ++arg)
        {
            const char* argStr = argv[arg];

            if (argStr[0] == '-')
            {
                if (strcmp(argStr, "-bp") == 0 && (arg + 1) < argc)
                {
                    char* pRet;
                    uint16_t bpAddr = (uint16_t)strtoul(argv[arg + 1], &pRet, 16);
                    DebugToggleBreakpoint(bpAddr);
                    arg++;
                }
                else if (strcmp(argStr, "-ss") == 0 && (arg + 1) < argc)
                {
                    char* pRet;
                    uint32_t ssTime = (uint32_t)strtoul(argv[arg + 1], &pRet, 10);
                    DebugSetScreenshotTime(ssTime);
                    arg++;
                }
            }
        }
    }
#endif

    Run();

    AppDestroy();

    return 0;
}
