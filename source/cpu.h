#ifndef CPU_H
#define CPU_H

#include "types.h"

bool CPUInit(uint16_t startAddr);
cycles CPUTick();

#endif
