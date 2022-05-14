#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "system.h"
#include "utils.h"
#include "cpu.h"
#include "ppu.h"

byte Mem[MEM_SIZE];

//Shortcuts
byte* ROM = &Mem[ROM_ADDR];
byte* VRAM = &Mem[VRAM_ADDR];
byte* RAM = &Mem[RAM_ADDR];

//I/O Registers
byte* Register_IF = &Mem[REGISTER_IF_ADDR];
byte* Register_LCDC = &Mem[REGISTER_LCDC_ADDR];
byte* Register_SCY = &Mem[REGISTER_SCY_ADDR];
byte* Register_SCX = &Mem[REGISTER_SCX_ADDR];
byte* Register_LY = &Mem[REGISTER_LY_ADDR];
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

bool SystemInit(const char* pRomFile)
{
    uint16_t startAddr = 0;

    //If there's no rom file then run the boot rom.
    if (pRomFile == NULL)
    {
        //Fake for now so the boot rom works.
        byte nintendoLogoData[] = { 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
                                    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
                                    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E };
        memcpy(&Mem[0x104], nintendoLogoData, sizeof(nintendoLogoData));

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

        //Load the boot rom.
        if (!readFile("dmg_boot.bin", ROM, ROM_SIZE))
        {
            return false;
        }
    }
    else
    {
        if (!readFile(pRomFile, ROM, ROM_SIZE))
        {
            return false;
        }

        startAddr = 0x100;
    }

    if (!CPUInit(startAddr))
    {
        return false;
    }

    if (!PPUInit())
    {
        return false;
    }

    return true;
}

void SystemTick(uint32_t dt)
{
    if (dt > 0)
    {
        //Run enough cycles for this dt.
        cycles numCyclesForDt = CLOCK_CYCLES_PER_MS * dt;

        if (numCyclesForDt > 0)
        {
            const cycles MAX_CLOCK_CYCLES_FOR_DT = CLOCK_CYCLES_PER_MS * 100;

            //To prevent spiral of death.
            if (numCyclesForDt > MAX_CLOCK_CYCLES_FOR_DT)
            {
                numCyclesForDt = MAX_CLOCK_CYCLES_FOR_DT;
                printf("Warning: Capping clock cycles!\n");
            }

            while (TickCycles < numCyclesForDt)
            {
                cycles cpuCycles = CPUTick();
                PPUTick(cpuCycles); //Run the same number of cycles on the PPU.
                TickCycles += cpuCycles;
            }

            assert(TickCycles >= numCyclesForDt);
            TickCycles -= numCyclesForDt;

            //Calculate emulation speed.
            CycleCounter += numCyclesForDt;

            if ((TickCounter += dt) > 1000)
            {
                EmulationSpeed = (float)CycleCounter / CLOCK_CYCLES;
                TickCounter = 0;
                CycleCounter = 0;

                if (EmulationSpeed < 1.f)
                {
                    printf("Warning: Emulation speed %.2f!\n", EmulationSpeed);
                }
            }
        }
    }
}
