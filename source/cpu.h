#ifndef CPU_H
#define CPU_H

#include "types.h"

struct CPURegisters
{
    union
    {
        struct
        {
            byte F; //Flags
            byte A; //Accumulator
        };

        uint16_t AF;
    };

    union
    {
        struct
        {
            byte C;
            byte B;
        };

        uint16_t BC;
    };

    union
    {
        struct
        {
            byte E;
            byte D;
        };

        uint16_t DE;
    };

    union
    {
        struct
        {
            byte L;
            byte H;
        };

        uint16_t HL;
    };

    uint16_t SP; //Stack pointer
    uint16_t PC; //Program counter
};

void CPUSetInterrupt(int interruptIdx);

bool CPUInit(uint16_t startAddr, byte interruptOps[], int numInterrupts);
cycles CPUTick();

#if DEBUG_ENABLED
const struct CPURegisters* DebugGetCPURegisters();
#endif

#endif
