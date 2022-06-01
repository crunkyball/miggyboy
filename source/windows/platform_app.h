#ifndef PLATFORM_APP_H
#define PLATFORM_APP_H

#include <SDL.h>

#include "types.h"

bool AppInit();
void AppDestroy();

bool AppTick();

void AppPreRender();
void AppRender();
void AppPostRender();

uint32_t AppGetTimeMS();

#endif
