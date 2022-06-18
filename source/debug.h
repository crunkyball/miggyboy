#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

#if DEBUG_ENABLED

#include "system.h"

#define MAX_OP_LENGTH 32
#define CALLSTACK_SIZE 8

#define MAX_BREAKPOINTS 16
#define INVALID_BREAKPOINT 0xFFFF

struct DisassembledROMInstr
{
    uint16_t ROMAddr;
    char OpStr[MAX_OP_LENGTH];
};

extern struct DisassembledROMInstr DisassembledROM[ROM_SIZE];
extern int DisassembledROMSize;

extern int DebugCallstack[CALLSTACK_SIZE];  //Indexes into the disassembled ROM.
extern int DebugCallstackHead;

extern uint16_t DebugBreakpoints[MAX_BREAKPOINTS];

int GetDisassembledROMIdxFromAddr(uint16_t romAddr);

void DebugToggleBreakpoint(uint16_t disassemblyAddr);
bool DebugHasBreakpoint(uint16_t disassemblyAddr);

void RegisterBreakpointHitCallback(CallbackFunc callback);

void DebugInit();

#endif

#endif
