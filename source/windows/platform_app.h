#ifndef PLATFORM_APP_H
#define PLATFORM_APP_H

#include <SDL.h>

#include "types.h"
#include "system_types.h"

bool AppInit();
void AppDestroy();

bool AppTick();

void AppPreRender();
void AppRender();
void AppPostRender();

void AppRegisterDirectionInputCallback(DirectionInputCallbackFunc callback);
void AppRegisterButtonInputCallback(ButtonInputCallbackFunc callback);

uint32_t AppGetTimeMS();

#endif
