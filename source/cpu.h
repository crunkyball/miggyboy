#ifndef CPU_H
#define CPU_H

#include "types.h"

void CPUSetInterrupt(int interruptIdx);

bool CPUInit(uint16_t startAddr, byte interruptOps[], int numInterrupts);
cycles CPUTick();

#endif
