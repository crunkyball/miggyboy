#include "system.h"
#include "windows/app.h"   //Need a better way to get this.

#include <stdio.h>

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

    Run();

    AppDestroy();

    return 0;
}
