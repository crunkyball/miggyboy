#include "debug.h"

#if DEBUG_ENABLED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "cpu.h"
#include "opcode_debug.h"
#include "utils.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_debug.h)

//When using just-in-time mode, only the instructions that are being run are disassembled. This prevents 
//us for inadvertently disassembling unknown data blocks but it also means that we won't be able to see 
//any disassembly for code that hasn't been run yet. Both JIT and Immediate mode have their uses.
bool JustInTimeDebugMode = false;

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

static int DisassembledROMCmpFunc(const void* pLhs, const void* pRhs)
{
    return ((struct DisassembledROMInstr*)pLhs)->ROMAddr - ((struct DisassembledROMInstr*)pRhs)->ROMAddr;
}

void SortDisassembly()
{
    qsort(&DisassembledROM[0], DisassembledROMSize, sizeof(DisassembledROM[0]), DisassembledROMCmpFunc);
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

void DebugDumpDisassembly()
{
    static const int DUMP_BUFFER_SIZE = 1024 * 1024;
    char* pDumpBuffer = malloc(DUMP_BUFFER_SIZE);

    int dumpBufferPos = 0;

    for (int i = 0; i < DisassembledROMSize; ++i)
    {
        #define LINE_BUFFER_SIZE 64
        char lineBuffer[LINE_BUFFER_SIZE];
        snprintf(lineBuffer, LINE_BUFFER_SIZE, "0x%.4X: %s\n", DisassembledROM[i].ROMAddr, DisassembledROM[i].OpStr);

        int lineLen = (int)strlen(lineBuffer);

        assert(dumpBufferPos + lineLen < DUMP_BUFFER_SIZE);

        strcpy(&pDumpBuffer[dumpBufferPos], lineBuffer);

        dumpBufferPos += lineLen;
    }

    if (!FileWrite("disassembly_dump.txt", pDumpBuffer, dumpBufferPos))
    {
        DebugPrint("Failed to dump disassembly!\n");
        assert(0);
    }

    free(pDumpBuffer);
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

void ResetDisassembly()
{
    memset(DebugCallstack, -1, sizeof(DebugCallstack));
    DebugCallstackHead = 0;

    memset(DisassembledROM, 0, sizeof(DisassembledROM));
    DisassembledROMSize = 0;
}

int DisassembleROMInstruction(uint16_t addr)
{
    const char* pOpStr = NULL;
    int opSize = 0;

    byte opCode = ReadMem(addr);
    uint16_t dataAddr = addr + 1;

    if (opCode == 0xCB)
    {
        dataAddr++;
        opCode = ReadMem(addr + 1);
        opSize = CPUExtendedOpGetDebugInfo(opCode, &pOpStr);
    }
    else
    {
        opSize = CPUOpGetDebugInfo(opCode, &pOpStr);
    }

    char dataStr[16];

    char* pDataLoc = NULL;
    int dataLocLen = 0;

    //"d8" 8-bit data
    if (pDataLoc = strstr(pOpStr, "d8"))
    {
        dataLocLen = 2; //strlen("d8")
        byte val = ReadMem(dataAddr);
        snprintf(dataStr, 16, "$%x", val);
    }
    //"d16" 16-bit data
    else if (pDataLoc = strstr(pOpStr, "d16"))
    {
        dataLocLen = 3; //strlen("d16")
        uint16_t val = ReadMem16(dataAddr);
        snprintf(dataStr, 16, "$%.4x", val);
    }
    //"a8" 8-bit unsigned data added to 0xFF00
    else if (pDataLoc = strstr(pOpStr, "a8"))
    {
        dataLocLen = 2; //strlen("a8")
        byte val = ReadMem(dataAddr);
        snprintf(dataStr, 16, "$FF00+$%x", val);
    }
    //"a16" little-endian 16-bit address
    else if (pDataLoc = strstr(pOpStr, "a16"))
    {
        dataLocLen = 3; //strlen("a16")
        uint16_t val = ReadMem16(dataAddr);
        snprintf(dataStr, 16, "$%.4x", val);
    }
    //"r8" 8-bit signed data
    else if (pDataLoc = strstr(pOpStr, "r8"))
    {
        dataLocLen = 2; //strlen("r8")
        int8_t val = (int8_t)ReadMem(dataAddr);
        snprintf(dataStr, 16, "%d", val);
    }

    if (pDataLoc != NULL)
    {
        int leftLen = (int)(pDataLoc - pOpStr);
        strncpy(DisassembledROM[DisassembledROMSize].OpStr, pOpStr, leftLen);
        snprintf(DisassembledROM[DisassembledROMSize].OpStr + leftLen, MAX_OP_LENGTH - leftLen, "%s%s", dataStr, pDataLoc + dataLocLen);
    }
    else
    {
        snprintf(DisassembledROM[DisassembledROMSize].OpStr, MAX_OP_LENGTH, "%s", pOpStr);
    }

    DisassembledROM[DisassembledROMSize].ROMAddr = addr;

    DisassembledROMSize++;

    return opSize;
}

void DisassembleROM()
{
    //Playing fast and loose with the term "disassemble". There's no analysis of the code other than 
    //just translating the opcodes one at a time which means that some of this should be taken with 
    //a pinch of salt as we could be translating some data blocks as code. To try and mitigate this 
    //I'm populating data blocks that are known about (ie boot rom Nintendo logo).

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
            int opSize = DisassembleROMInstruction(memAddr);

            memAddr += opSize;
        }
    }
}

void OnStep()
{
    const struct CPURegisters* cpuRegisters = DebugGetCPURegisters();

    int disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC);

    //If we haven't disassembled this instruction yet then disassemble it.
    if (JustInTimeDebugMode && disassemblyIdx == -1)
    {
        DisassembleROMInstruction(cpuRegisters->PC);

        //We need to ensure that the disassembly is sorted and re-fetch the index.
        SortDisassembly();
        disassemblyIdx = GetDisassembledROMIdxFromAddr(cpuRegisters->PC);
    }

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

void OnROMChanged()
{
    ResetDisassembly();

    //If we're not disassembling just-in-time then we need to disassembly the whole thing now.
    if (!JustInTimeDebugMode)
    {
        DisassembleROM();
    }
}

void DebugInit()
{
    memset(DebugBreakpoints, INVALID_BREAKPOINT, sizeof(DebugBreakpoints));

    RegisterStepCallback(&OnStep);
    RegisterROMChangedCallback(&OnROMChanged);

    AddKnownDataBlock(0x00A8, 0x00DF); //Nintendo logo and r symbol in boot rom.
    AddKnownDataBlock(0x0104, 0x014F); //Nintendo logo in cartridge and cartridge header.

    OnROMChanged();

    //Force an initial step so we have the debug info for the first instruction.
    OnStep();
}

#endif
