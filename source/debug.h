#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

#if DEBUG_ENABLED

#include "system.h"

#define MAX_OP_LENGTH 32
#define CALLSTACK_SIZE 8

struct DisassembledROMInstr
{
    uint16_t ROMAddr;
    char OpStr[MAX_OP_LENGTH];
};

extern struct DisassembledROMInstr DisassembledROM[ROM_SIZE];
extern int DisassembledROMSize;

extern int DebugCallstack[CALLSTACK_SIZE];  //Indexes into the disassembled ROM.
extern int DebugCallstackHead;

int GetDisassembledROMIdxFromAddr(uint16_t romAddr);

void DebugInit();

#endif

#endif
