#ifndef APP_H
#define APP_H

#include "types.h"

bool AppInit();
void AppDestroy();

bool AppTick();

void AppPreRender();
void AppRender();
void AppPostRender();

uint32_t AppGetTimeMS();

#endif

