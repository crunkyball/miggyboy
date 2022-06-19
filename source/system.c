#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "system.h"
#include "utils.h"
#include "cpu.h"
#include "ppu.h"

#include "debug.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_debug.h)

byte Mem[MEM_SIZE];

//Shortcuts
byte* ROM = &Mem[ROM_ADDR];
byte* VRAM = &Mem[VRAM_ADDR];
byte* RAM = &Mem[RAM_ADDR];

//I/O Registers
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

static int TimerIntervalCount = 0;
static byte Timer = 0;

#define BOOT_ROM_SIZE 0x100
static byte BootROM[BOOT_ROM_SIZE];
static bool BootROMMapped = false;

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

//A store where we swap out the cartride data for the boot rom and then swap it back when the boot rom is done.
//This is temporary until I add support for some kind of memory mapping. That'll be required for bank switching, anyway.
static byte CartridgeTemp[BOOT_ROM_SIZE];

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

void FireInterrupt(enum Interrupt interrupt)
{
    CPUSetInterrupt(interrupt);
}

void TimerTick(cycles numCycles)
{
    bool timerEnabled = (*Register_TAC & 0b100);
    int timerMode = (*Register_TAC & 0b11);

    if (timerEnabled)
    {
        if ((TimerIntervalCount += numCycles) >= TimerInterval[timerMode])
        {
            TimerIntervalCount -= TimerInterval[timerMode];

            if (Timer == 0xFF)
            {
                Timer = *Register_TMA;
                FireInterrupt(Interrupt_Timer);
            }
            else
            {
                Timer++;
            }
        }
    }
}

bool SystemInit(const char* pRomFile)
{
    uint16_t startAddr = 0;

    //Reading from the cartridge when there isn't one in results in 0xFF.
    memset(ROM, 0xFF, ROM_SIZE);

    if (!FileRead("dmg_boot.bin", BootROM, BOOT_ROM_SIZE))
    {
        DebugPrint("Failed to read boot rom file!\n");
        assert(0);
        return false;
    }

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
    else
    {
        //We don't have a cartride so just fake some stuff to make the emulator nice to run.
        byte nintendoLogoData[] = { 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
                                    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
                                    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E };
        memcpy(&Mem[0x0104], nintendoLogoData, sizeof(nintendoLogoData));

        //Fake cartridge header.
        Mem[0x0134] = 'T';
        Mem[0x0135] = 'E';
        Mem[0x0136] = 'S';
        Mem[0x0137] = 'T';

        byte checksum = 0;
        for (int i = 0x0134; i <= 0x014C; ++i)
        {
            checksum -= Mem[i] + 1;
        }

        Mem[0x014D] = checksum;

        //Hack to force the boot rom to stay on screen until games are actually supported.
        //This is where the cartridge code is executed so just force it to loop.
        Mem[0x100] = 0xC3;  //Jump to address
        *((uint16_t*)&Mem[0x101]) = 0x100;
    }

    //Copy the cartride data into the temporary store until the boot rom is finished.
    memcpy(CartridgeTemp, ROM, BOOT_ROM_SIZE);

    //Load the boot rom into memory.
    memcpy(ROM, BootROM, BOOT_ROM_SIZE);
    BootROMMapped = true;

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
                cycles stepCycles = Step();

                //Hacky hacky hack hack!
                if (BootROMMapped && Mem[0xFF50] != 0)
                {
                    memcpy(ROM, CartridgeTemp, BOOT_ROM_SIZE);
                    BootROMMapped = false;

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
