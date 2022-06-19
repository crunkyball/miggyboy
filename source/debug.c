#include "debug.h"

#if DEBUG_ENABLED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "cpu.h"
#include "opcode_debug.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_debug.h)

struct DisassembledROMInstr DisassembledROM[ROM_SIZE];
int DisassembledROMSize = 0;

int DebugCallstack[CALLSTACK_SIZE];
int DebugCallstackHead = 0;

uint16_t DebugBreakpoints[MAX_BREAKPOINTS];

static CallbackFunc BreakpointHitCallback = NULL;

struct KnownDataBlock
{
    uint16_t FromAddr;
    uint16_t ToAddr;
};

#define MAX_KNOWN_DATA_BLOCKS 16
static struct KnownDataBlock KnownDataBlocks[MAX_KNOWN_DATA_BLOCKS];
static int NumKnownDataBlocks = 0;

void AddKnownDataBlock(uint16_t fromAddr, uint16_t toAddr)
{
    if (NumKnownDataBlocks < MAX_KNOWN_DATA_BLOCKS)
    {
        KnownDataBlocks[NumKnownDataBlocks].FromAddr = fromAddr;
        KnownDataBlocks[NumKnownDataBlocks].ToAddr = toAddr;
        NumKnownDataBlocks++;
    }
    else
    {
        assert(0);
    }
}

void OnStep()
{
    const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

    //We should have already disassembled the op.
    int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC);

    DebugCallstack[DebugCallstackHead] = disassemblyIdx;

    DebugCallstackHead++;

    if (DebugCallstackHead == CALLSTACK_SIZE)
    {
        DebugCallstackHead = 0;
    }

    //Do we need to break at this address?
    if (DebugHasBreakpoint(cpuRegisters->PC))
    {
        DebugPrint("Breakpoint hit at 0x%.4X!\n", cpuRegisters->PC);
        EnableSingleStepMode();

        if (BreakpointHitCallback != NULL)
        {
            BreakpointHitCallback();
        }
    }
}

static int DisassembledROMCmpFunc(const void* pLhs, const void* pRhs)
{
    return ((struct DisassembledROMInstr*)pLhs)->ROMAddr - ((struct DisassembledROMInstr*)pRhs)->ROMAddr;
}

int GetDisassembledROMIdxFromAddr(uint16_t romAddr)
{
    struct DisassembledROMInstr* pInstr = bsearch(&romAddr, DisassembledROM, DisassembledROMSize, sizeof(DisassembledROM[0]), DisassembledROMCmpFunc);

    if (pInstr != NULL)
    {
        return (int)(pInstr - &DisassembledROM[0]);
    }

    return -1;
}

void DebugToggleBreakpoint(uint16_t disassemblyAddr)
{
    //If it's set, unset it.
    for (int i = 0; i < MAX_BREAKPOINTS; ++i)
    {
        if (DebugBreakpoints[i] == disassemblyAddr)
        {
            DebugBreakpoints[i] = INVALID_BREAKPOINT;
            return;
        }
    }

    //Otherwise set it, if we have space.
    for (int i = 0; i < MAX_BREAKPOINTS; ++i)
    {
        if (DebugBreakpoints[i] == INVALID_BREAKPOINT)
        {
            DebugBreakpoints[i] = disassemblyAddr;
            return;
        }
    }
}

bool DebugHasBreakpoint(uint16_t disassemblyAddr)
{
    for (int i = 0; i < MAX_BREAKPOINTS; ++i)
    {
        if (DebugBreakpoints[i] == disassemblyAddr)
        {
            return true;
        }
    }

    return false;
}

void RegisterBreakpointHitCallback(CallbackFunc callback)
{
    BreakpointHitCallback = callback;
}

void DisassembleROM()
{
    //Playing fast and loose with the term "disassemble". There's no analysis of the code other than 
    //just translating the opcodes one at a time which means that some of this should be taken with 
    //a pinch of salt as we could be translating some data blocks as code. To try and mitigate this 
    //I'm populating data blocks that are known about (ie boot rom Nintendo logo).
    //Perhaps a better solution would be to disassemble the code as it's running? That way we'd only 
    //have disassembly of actual code but we wouldn't be able to see code yet to be executed...

    memset(DebugCallstack, -1, sizeof(DebugCallstack));
    DebugCallstackHead = 0;

    memset(DisassembledROM, 0, sizeof(DisassembledROM));
    DisassembledROMSize = 0;

    int memAddr = 0;

    while (memAddr < ROM_SIZE)
    {
        bool dataBlock = false;

        for (int i = 0; i < NumKnownDataBlocks; ++i)
        {
            if (memAddr >= KnownDataBlocks[i].FromAddr && memAddr <= KnownDataBlocks[i].ToAddr)
            {
                dataBlock = true;
                break;
            }
        }

        if (dataBlock)
        {
            memAddr++;
        }
        else
        {
            const char* pOpStr = NULL;
            int opSize = CPUOpGetDebugInfo(&Mem[memAddr], &pOpStr);
            char dataStr[16];

            char* pDataLoc = NULL;
            int dataLocLen = 0;

            //"d8" 8-bit data
            if (pDataLoc = strstr(pOpStr, "d8"))
            {
                dataLocLen = 2; //strlen("d8")
                sprintf_s(dataStr, 16, "$%x", Mem[memAddr + 1]);
            }
            //"d16" 16-bit data
            else if (pDataLoc = strstr(pOpStr, "d16"))
            {
                dataLocLen = 3; //strlen("d16")
                sprintf_s(dataStr, 16, "$%.4x", *(uint16_t*)&Mem[memAddr + 1]);
            }
            //"a8" 8-bit unsigned data added to 0xFF00
            else if (pDataLoc = strstr(pOpStr, "a8"))
            {
                dataLocLen = 2; //strlen("a8")
                sprintf_s(dataStr, 16, "$FF00+$%x", Mem[memAddr + 1]);
            }
            //"a16" little-endian 16-bit address
            else if (pDataLoc = strstr(pOpStr, "a16"))
            {
                dataLocLen = 3; //strlen("a16")
                sprintf_s(dataStr, 16, "$%.4x", *(uint16_t*)&Mem[memAddr + 1]);
            }
            //"r8" 8-bit signed data
            else if (pDataLoc = strstr(pOpStr, "r8"))
            {
                dataLocLen = 2; //strlen("r8")
                sprintf_s(dataStr, 16, "%d", *(int8_t*)&Mem[memAddr + 1]);
            }

            if (pDataLoc != NULL)
            {
                int leftLen = (int)(pDataLoc - pOpStr);
                strncpy_s(DisassembledROM[DisassembledROMSize].OpStr, MAX_OP_LENGTH, pOpStr, leftLen);
                sprintf_s(DisassembledROM[DisassembledROMSize].OpStr + leftLen, MAX_OP_LENGTH - leftLen, "%s%s", dataStr, pDataLoc + dataLocLen);
                int a = 0;
            }
            else
            {
                sprintf_s(DisassembledROM[DisassembledROMSize].OpStr, MAX_OP_LENGTH, "%s", pOpStr);
            }

            DisassembledROM[DisassembledROMSize].ROMAddr = memAddr;

            memAddr += opSize;

            DisassembledROMSize++;
        }
    }
}

void OnROMChanged()
{
    DisassembleROM();
}

void DebugInit()
{
    memset(DebugBreakpoints, INVALID_BREAKPOINT, sizeof(DebugBreakpoints));

    RegisterStepCallback(&OnStep);
    RegisterROMChangedCallback(&OnROMChanged);

    AddKnownDataBlock(0x00A8, 0x00DF); //Nintendo logo and r symbol in boot rom.
    AddKnownDataBlock(0x0104, 0x014F); //Nintendo logo in cartridge and cartridge header.

    DisassembleROM();

    //Force an initial step so we have the debug info for the first instruction.
    OnStep();
}

#endif
