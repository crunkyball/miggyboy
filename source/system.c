#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "system.h"
#include "utils.h"
#include "cpu.h"
#include "ppu.h"

#include "debug.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_app.h)
#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_debug.h)

//Addressable Memory. This should be accessed via the Read/Write/Access functions to allow for memory mapping.
static byte Mem[MEM_SIZE];

//Shortcuts
byte* ROM = &Mem[ROM_ADDR];
byte* VRAM = &Mem[VRAM_ADDR];
byte* RAM = &Mem[RAM_ADDR];

//Hardware Registers
byte* Register_P1 = &Mem[REGISTER_P1_ADDR];
byte* Register_DIV = &Mem[REGISTER_DIV_ADDR];
byte* Register_TIMA = &Mem[REGISTER_TIMA_ADDR];
byte* Register_TMA = &Mem[REGISTER_TMA_ADDR];
byte* Register_TAC = &Mem[REGISTER_TAC_ADDR];
byte* Register_IF = &Mem[REGISTER_IF_ADDR];
byte* Register_LCDC = &Mem[REGISTER_LCDC_ADDR];
byte* Register_STAT = &Mem[REGISTER_STAT_ADDR];
byte* Register_SCY = &Mem[REGISTER_SCY_ADDR];
byte* Register_SCX = &Mem[REGISTER_SCX_ADDR];
byte* Register_LY = &Mem[REGISTER_LY_ADDR];
byte* Register_DMA = &Mem[REGISTER_DMA_ADDR];
byte* Register_BGP = &Mem[REGISTER_BGP_ADDR];
byte* Register_WY = &Mem[REGISTER_WY_ADDR];
byte* Register_WX = &Mem[REGISTER_WX_ADDR];
byte* Register_IE = &Mem[REGISTER_IE_ADDR];

#define CLOCK_CYCLES 4194304
static const int CLOCK_CYCLES_PER_MS = CLOCK_CYCLES / 1000;
static int TickCycles = 0;

//For calculating emulation speed.
static int TickCounter = 0;
static int CycleCounter = 0;
static float EmulationSpeed = 0;

static int TimerInterval[4] = {
    1024,   //4096Hz
    16,     //262144Hz
    64,     //65536Hz
    256     //16384Hz
};

static int DivIntervalCount = 0;
static int TimerIntervalCount = 0;

#define BOOT_ROM_SIZE 0x100
static byte BootROM[BOOT_ROM_SIZE];

struct CartridgeHeader
{
    byte Title[16];         //0x0134-0x0143
    byte LicenseeCode[2];   //0x0144-0x0145
    byte SuperGBSupport;    //0x0146
    byte CartridgeType;     //0x0147
    byte ROMSize;           //0x0148
    byte RAMSize;           //0x0149
    byte Region;            //0x014A
    byte OldLicenseeCode;   //0x014B
    byte GameVersion;       //0x014C
    byte HeaderChecksum;    //0x014D
    byte GlobalChecksum[2]; //0x014E-0x014F
};

static DirectionInputCallbackFunc DirectionInputCallback = NULL;
static ButtonInputCallbackFunc ButtonInputCallback = NULL;

byte DirectionInputState = 0xFF;
byte ButtonInputState = 0xFF;

#if DEBUG_ENABLED
static bool SingleStepMode = false;
static bool SingleStepPending = false;
static CallbackFunc StepCallback = NULL;
static CallbackFunc ROMChangedCallback = NULL;

void ToggleSingleStepMode()
{
    SingleStepMode = !SingleStepMode;
}

void EnableSingleStepMode()
{
    SingleStepMode = true;
}

void RequestSingleStep()
{
    SingleStepPending = true;
}

void RegisterStepCallback(CallbackFunc callback)
{
    StepCallback = callback;
}

void RegisterROMChangedCallback(CallbackFunc callback)
{
    ROMChangedCallback = callback;
}
#endif

static void SetInputRegisterState()
{
    byte directionButtonsMask = 1 << 4;
    byte actionButtonsMask = 1 << 5;

    bool directionButtons = (*Register_P1 & directionButtonsMask) == 0;
bool actionButtons = (*Register_P1 & actionButtonsMask) == 0;

*Register_P1 = 0xCF;

if (directionButtons)
{
    *Register_P1 |= directionButtonsMask;
    *Register_P1 &= DirectionInputState;
}
else if (actionButtons)
{
    *Register_P1 |= actionButtonsMask;
    *Register_P1 &= ButtonInputState;
}
}

static void OnDirectionInput(enum DirectionInput input, bool pressed)
{
    if (pressed)
    {
        DirectionInputState ^= input;
    }
    else
    {
        DirectionInputState |= input;
    }
}

static void OnButtonInput(enum ButtonInput input, bool pressed)
{
    if (pressed)
    {
        ButtonInputState &= ~input;
    }
    else
    {
        ButtonInputState |= input;
    }
}

static void DMAToSpriteTable()
{
    for (uint16_t destAddr = VRAM_SPRITE_TABLE_ADDR, sourceAddr = *Register_DMA * 0x100; destAddr < VRAM_SPRITE_TABLE_ADDR + VRAM_SPRITE_TABLE_SIZE; ++destAddr, ++sourceAddr)
    {
        byte val = ReadMem(sourceAddr);
        WriteMem(destAddr, val);
    }
}

//Memory access functions. Ideally, most things should be using Read/WriteMem in-case I need 
//to add bus timing emulation at some point. Maybe AccessMem should be removed in future...
byte* AccessMem(uint16_t addr)
{
    if (Mem[0xFF50] == 0 && addr < BOOT_ROM_SIZE)   //Boot ROM mapped.
    {
        return &BootROM[addr];
    }

    return &Mem[addr];
}

byte ReadMem(uint16_t addr)
{
    return *AccessMem(addr);
}

void WriteMem(uint16_t addr, byte val)
{
    byte* pAddr = AccessMem(addr);

    //Not allowed to write to ROM!
    if (addr >= ROM_SIZE)
    {
        *pAddr = val;
    }

    if (addr == REGISTER_P1_ADDR)
    {
        SetInputRegisterState();
    }
    else if (addr == REGISTER_DMA_ADDR)
    {
        DMAToSpriteTable();
    }
    else if (addr == REGISTER_DIV_ADDR)
    {
        *Register_DIV = 0;
    }
}

uint16_t ReadMem16(uint16_t addr)
{
    byte* pAddr = AccessMem(addr);
    return *pAddr | (*(pAddr + 1) << 8);
}

void FireInterrupt(enum Interrupt interrupt)
{
    CPUSetInterrupt(interrupt);
}

void TimerTick(cycles numCycles)
{
    if ((DivIntervalCount += numCycles) >= 0x4000)
    {
        DivIntervalCount -= 0x4000;

        if (*Register_DIV == 0xFF)
        {
            *Register_DIV = 0;
        }
        else
        {
            (*Register_DIV)++;
        }
    }

    bool timerEnabled = (*Register_TAC & 0b100);
    int timerMode = (*Register_TAC & 0b11);

    if (timerEnabled)
    {
        if ((TimerIntervalCount += numCycles) >= TimerInterval[timerMode])
        {
            TimerIntervalCount -= TimerInterval[timerMode];

            if (*Register_TIMA == 0xFF)
            {
                *Register_TIMA = *Register_TMA;
                FireInterrupt(Interrupt_Timer);
            }
            else
            {
                (*Register_TIMA)++;
            }
        }
    }
}

bool SystemInit(const char* pRomFile)
{
    uint16_t startAddr = 0;

    //Initialise system state as required (https://gbdev.io/pandocs/Power_Up_Sequence.html).
    *Register_P1 = 0xCF;
    *Register_DIV = 0x18;
    *Register_TIMA = 0x00;
    *Register_TMA = 0x00;
    *Register_TAC = 0x00;
    *Register_IF = 0xE1;
    *Register_LCDC = 0x91;
    *Register_STAT = 0x81;
    *Register_SCY = 0x00;
    *Register_SCX = 0x00;
    *Register_LY = 0x91;
    *Register_BGP = 0xFC;
    *Register_WY = 0x00;
    *Register_WX = 0x00;
    *Register_IE = 0x00;

    //Reading from the cartridge when there isn't one in results in 0xFF.
    memset(ROM, 0xFF, ROM_SIZE);

    #define USE_BOOT_ROM 1

#if USE_BOOT_ROM
    if (!FileRead("dmg_boot.bin", BootROM, BOOT_ROM_SIZE))
    {
        DebugPrint("Failed to read boot rom file!\n");
        assert(0);
        return false;
    }
#else
    Mem[0xFF50] = 1;    //Unmap the boot rom.
    startAddr = 0x100;
#endif

    if (pRomFile != NULL)
    {
        if (!FileRead(pRomFile, ROM, ROM_SIZE))
        {
            DebugPrint("Failed to read file %s!\n", pRomFile);
            assert(0);
            return false;
        }

        struct CartridgeHeader* pHeader = (struct CartridgeHeader*)&Mem[0x0134];
        DebugPrint("Cartridge Loaded: \n");
        DebugPrint("\tTitle: %.16s\n", pHeader->Title);
        DebugPrint("\tLicenseeCode: %.2s\n", pHeader->LicenseeCode);
        DebugPrint("\tSuperGBSupport: %u\n", pHeader->SuperGBSupport);
        DebugPrint("\tCartridgeType: %u\n", pHeader->CartridgeType);
        DebugPrint("\tROMSize: %u\n", pHeader->ROMSize);
        DebugPrint("\tRAMSize: %u\n", pHeader->RAMSize);
        DebugPrint("\tRegion: %u\n", pHeader->Region);
        DebugPrint("\tOldLicenseeCode: %u\n", pHeader->OldLicenseeCode);
        DebugPrint("\tGameVersion: %u\n", pHeader->GameVersion);

        assert(pHeader->CartridgeType == 0);    //We don't support any fancy shit yet!
    }

    byte interruptOps[NUM_INTERRUPTS] = {
        InterruptOp_VBlank,
        InterruptOp_STAT,
        InterruptOp_Timer,
        0,//InterruptOp_Serial,
        InterruptOp_Joypad
    };

    if (!CPUInit(startAddr, interruptOps, NUM_INTERRUPTS))
    {
        return false;
    }

    if (!PPUInit())
    {
        return false;
    }

    AppRegisterDirectionInputCallback(&OnDirectionInput);
    AppRegisterButtonInputCallback(&OnButtonInput);

    return true;
}

cycles Step()
{
    cycles cpuCycles = CPUTick();

    if (cpuCycles == 0)
    {
        //In the case that the CPU is HALTed we need to keep the rest of the system ticking over.
        cpuCycles = 1;
    }

    //Update PPU.
    PPUTick(cpuCycles);

    //Update timer.
    TimerTick(cpuCycles);

#if DEBUG_ENABLED
    if (StepCallback != NULL)
    {
        StepCallback();
    }
#endif

    return cpuCycles;
}

void SystemTick(uint32_t dt)
{
#if DEBUG_ENABLED
    if (SingleStepMode)
    {
        if (SingleStepPending)
        {
            Step();
            SingleStepPending = false;
        }

        return;
    }
#endif

    if (dt > 0)
    {
        //Run enough cycles for this dt.
        cycles numCyclesForDt = CLOCK_CYCLES_PER_MS * dt;

        if (numCyclesForDt > 0)
        {
            const cycles MAX_CLOCK_CYCLES_FOR_DT = CLOCK_CYCLES_PER_MS * 500;

            //To prevent spiral of death.
            if (numCyclesForDt > MAX_CLOCK_CYCLES_FOR_DT)
            {
                numCyclesForDt = MAX_CLOCK_CYCLES_FOR_DT;
                DebugPrint("Warning: Capping clock cycles!\n");
            }

            while (TickCycles < numCyclesForDt
#if DEBUG_ENABLED
                && !SingleStepMode  //If we hit a breakpoint during a step we need to break out.
#endif
                )
            {
                byte bootROMMapVal = Mem[0xFF50];

                cycles stepCycles = Step();

                //May be a better way of doing this but probably after I've added MBC support.
                if (bootROMMapVal != Mem[0xFF50])
                {
#if DEBUG_ENABLED
                    if (ROMChangedCallback != NULL)
                    {
                        ROMChangedCallback();
                    }
#endif
                }

                TickCycles += stepCycles;
            }

            TickCycles = MAX(0, TickCycles - numCyclesForDt);

            //Calculate emulation speed.
            CycleCounter += numCyclesForDt;

            if ((TickCounter += dt) >= 1000)
            {
                EmulationSpeed = (float)CycleCounter / CLOCK_CYCLES;
                TickCounter = 0;
                CycleCounter = 0;

                if (EmulationSpeed < 0.99f)
                {
                    DebugPrint("Warning: Emulation speed %.2f!\n", EmulationSpeed);
                }
            }
        }
    }
}
