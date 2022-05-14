#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "system.h"

struct
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
} static Register;

static bool IME;   //Interrupt master flag.

//Operator helpers
enum Flag
{
    Flag_Zero = 1 << 7,
    Flag_Subtract = 1 << 6,
    Flag_HalfCarry = 1 << 5,
    Flag_Carry = 1 << 4
};

enum FlagSet
{
    FlagSet_Leave = -1,
    FlagSet_Off = 0,
    FlagSet_On = 1
};

static void SetFlag(enum Flag flag, enum FlagSet flagSet)
{
    Register.F = flagSet != FlagSet_Leave ? flagSet == FlagSet_On ? Register.F | flag : Register.F & ~flag : Register.F;
}

static void SetFlags(enum FlagSet zeroFlag, enum FlagSet subtractFlag, enum FlagSet halfCarryFlag, enum FlagSet carryFlag)
{
    SetFlag(Flag_Zero, zeroFlag);
    SetFlag(Flag_Subtract, subtractFlag);
    SetFlag(Flag_HalfCarry, halfCarryFlag);
    SetFlag(Flag_Carry, carryFlag);
}

//Set the zero flag and reset all other flags
static void SetZeroFlag(bool bOn)
{
    SetFlags(bOn ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, FlagSet_Off);
}

static bool IsFlagSet(enum Flag flag)
{
    return (Register.F & flag) == flag;
}

static bool IsRegisterBitSet(const byte* pR, int bit)
{
    return (*pR & (1 << bit)) != 0;
}

static void SetRegisterBit(byte* pR, int bit)
{
    *pR |= (1 << bit);
}

static void UnsetRegisterBit(byte* pR, int bit)
{
    *pR &= ~(1 << bit);
}

static int8_t FromTwosComplement(byte b)
{
    /*if ((b & (1 << 7)) == 0)
    {
        return b;
    }

    return -(~b+1);*/

    //Should be good enough on most platforms.
    return (int8_t)b;
}

static void StackPush(uint16_t value)
{
    Mem[Register.SP] = value & 0xFF;
    Mem[Register.SP - 1] = value >> 8;
    Register.SP -= 2;
}

static uint16_t StackPop()
{
    uint16_t val = Mem[Register.SP + 2] | (Mem[Register.SP + 1] << 8);
    Register.SP += 2;
    return val;
}

//Operators
static cycles Op_NOP()
{
    //1 byte, 4 cycles, No flags
    Register.PC += 1;
    return 4;
}

static cycles Op_LoadImmediate8(byte* pR)
{
    //2 bytes, 8 cycles, No flags
    *pR = Mem[Register.PC + 1];
    Register.PC += 2;
    return 8;
}

static cycles Op_LoadImmediate16(uint16_t* pR)
{
    //3 bytes, 12 cycles, No flags
    *pR = Mem[Register.PC + 1] | (Mem[Register.PC + 2] << 8);
    Register.PC += 3;
    return 12;
}

static cycles Op_LoadRegister8(byte* pToR, const byte* pFromR)
{
    //1 byte, 4 cycles, No flags
    *pToR = *pFromR;
    Register.PC += 1;
    return 4;
}

static cycles Op_LoadAddrRegister(uint16_t addr, const byte* pR)
{
    //1 byte, 8 cycles, No flags
    Mem[addr] = *pR;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadRegisterAddr(byte* pR, uint16_t addr)
{
    //1 byte, 8 cycles, No flags
    *pR = Mem[addr];
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadAddrImmediate(uint16_t* pRAddr)
{
    //2 bytes, 12 cycles, No flags
    Mem[*pRAddr] = Mem[Register.PC + 1];
    Register.PC += 2;
    return 12;
}

static cycles Op_LoadAddrRegisterAndInc(uint16_t* pRAddr, const byte* pR)
{
    //1 byte, 8 cycles, No flags
    Mem[*pRAddr] = *pR;
    (*pRAddr)++;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadAddrRegisterAndDec(uint16_t* pRAddr, const byte* pR)
{
    //1 byte, 8 cycles, No flags
    Mem[*pRAddr] = *pR;
    (*pRAddr)--;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadRegisterAddrAndInc(byte* pR, uint16_t* pRAddr)
{
    //1 byte, 8 cycles, No flags
    *pR = Mem[*pRAddr];
    (*pRAddr)++;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadRegisterAddrAndDec(byte* pR, uint16_t* pRAddr)
{
    //1 byte, 8 cycles, No flags
    *pR = Mem[*pRAddr];
    (*pRAddr)--;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadImmediateAddr8FromA()
{
    //2 bytes, 12 cycles, No flags
    uint16_t addr = 0xFF00 + Mem[Register.PC + 1];
    Mem[addr] = Register.A;
    Register.PC += 2;
    return 12;
}

static cycles Op_LoadAFromImmediateAddr8()
{
    //2 bytes, 12 cycles, No flags
    uint16_t addr = 0xFF00 + Mem[Register.PC + 1];
    Register.A = Mem[addr];
    Register.PC += 2;
    return 12;
}

static cycles Op_LoadImmediateAddr16FromA()
{
    //3 bytes, 16 cycles, No flags
    uint16_t addr = Mem[Register.PC + 1] | (Mem[Register.PC + 2] << 8);
    Mem[addr] = Register.A;
    Register.PC += 3;
    return 16;
}

static cycles Op_LoadAFromImmediateAddr16()
{
    //3 bytes, 16 cycles, No flags
    uint16_t addr = Mem[Register.PC + 1] | (Mem[Register.PC + 2] << 8);
    Register.A = Mem[addr];
    Register.PC += 3;
    return 16;
}

static cycles Op_Push(const uint16_t* pR)
{
    //1 byte, 16 cycles, No flags
    StackPush(*pR);
    Register.PC += 1;
    return 16;
}

static cycles Op_Pop(uint16_t* pR)
{
    //1 byte, 12 cycles, No flags
    *pR = StackPop();
    Register.PC += 1;
    return 16;
}

static void DoCompare(byte val)
{
    if (Register.A == val)
    {
        SetFlags(FlagSet_On, FlagSet_On, FlagSet_Off, FlagSet_Off);
    }
    else
    {
        bool carry = Register.A < val;
        bool halfCarry = (int)(Register.A & 0xF) - (int)(val & 0xF) < 0;

        SetFlags(FlagSet_Off, FlagSet_On, halfCarry ? FlagSet_On : FlagSet_Off, carry ? FlagSet_On : FlagSet_Off);
    }
}

static cycles Op_CompareRegister(const byte* pR)
{
    //1 byte, 4 cycles, Z1HC
    DoCompare(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_CompareAddr()
{
    //1 byte, 8 cycles, Z1HC
    DoCompare(Mem[Register.HL]);
    Register.PC += 1;
    return 8;
}

static cycles Op_CompareImmediate()
{
    //2 bytes, 8 cycles, Z1HC
    DoCompare(Mem[Register.PC + 1]);
    Register.PC += 2;
    return 8;
}

static cycles Op_And(byte* pR)
{
    //1 byte, 4 cycles, Flags Z010
    Register.A &= *pR;
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_On, FlagSet_Off);
    Register.PC += 1;
    return 4;
}

static cycles Op_Or(byte* pR)
{
    //1 byte, 4 cycles, Flags Z000
    Register.A |= *pR;
    SetZeroFlag(Register.A == 0);
    Register.PC += 1;
    return 4;
}

static cycles Op_Xor(byte* pR)
{
    //1 byte, 4 cycles, Flags Z000
    Register.A ^= *pR;
    SetZeroFlag(Register.A == 0);
    Register.PC += 1;
    return 4;
}

static cycles Op_Complement()
{
    //1 byte, 4 cycles, Flags -11-
    Register.A = ~Register.A;
    SetFlags(FlagSet_Leave, FlagSet_On, FlagSet_On, FlagSet_Leave);
    Register.PC += 1;
    return 4;
}

static cycles Op_Increment8(byte* pR)
{
    //In this case we can just check if the bottom four bits are set because increasing by one will carry.
    bool halfCarry = ((Register.C & 0xF) == 0xF);

    //1 byte, 4 cycles, Flags Z0H-
    (*pR)++;
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, halfCarry ? FlagSet_On : FlagSet_Off, FlagSet_Leave);
    Register.PC += 1;
    return 4;
}

static cycles Op_Increment16(uint16_t* pR)
{
    //1 byte, 8 cycles, No flags
    (*pR)++;
    Register.PC += 1;
    return 8;
}

static cycles Op_Decrement8(byte* pR)
{
    //In this case we can just check if the bottom four bits are unset because decreasing by one will carry.
    bool halfCarry = ((Register.C & 0xF) == 0x0);

    //1 byte, 4 cycles, Flags Z1H-
    (*pR)--;
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_On, halfCarry ? FlagSet_On : FlagSet_Off, FlagSet_Leave);
    Register.PC += 1;
    return 4;
}

static cycles Op_Decrement16(uint16_t* pR)
{
    //1 byte, 8 cycles, No flags
    (*pR)--;
    Register.PC += 1;
    return 8;
}

static void DoAdd(byte val)
{
    bool halfCarry = (Register.A & 0xf) + (val & 0xf) > 0xf;
    bool carry = (Register.A + val) > 0xFF;
    Register.A = FromTwosComplement(Register.A) + FromTwosComplement(val);
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, halfCarry ? FlagSet_On : FlagSet_Off, carry ? FlagSet_On : FlagSet_Off);
}

static cycles Op_AddRegister(const byte* pR)
{
    //1 byte, 4 cycles, Z0HC
    DoAdd(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_AddAddr()
{
    //1 byte, 8 cycles, Z0HC
    DoAdd(Mem[Register.HL]);
    Register.PC += 1;
    return 8;
}

static cycles Op_AddImmediate()
{
    //2 bytes, 8 cycles, Z0HC
    DoAdd(Mem[Register.PC + 1]);
    Register.PC += 2;
    return 8;
}

static void DoSubtract(byte val)
{
    bool halfCarry = (int)(Register.A & 0xf) - (int)(val & 0xf) < 0;
    bool carry = val > Register.A;
    //Register.A = FromTwosComplement(Register.A) - FromTwosComplement(val);    Don't need to do this.
    Register.A -= val;
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_On, halfCarry ? FlagSet_On : FlagSet_Off, carry ? FlagSet_On : FlagSet_Off);
}

static cycles Op_SubtractRegister(const byte* pR)
{
    //1 byte, 4 cycles, Z1HC
    DoSubtract(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_SubtractAddr()
{
    //1 byte, 8 cycles, Z1HC
    DoSubtract(Mem[Register.HL]);
    Register.PC += 1;
    return 8;
}

static cycles Op_SubtractImmediate()
{
    //2 bytes, 8 cycles, Z1HC
    DoSubtract(Mem[Register.PC + 1]);
    Register.PC += 2;
    return 8;
}

static cycles Op_Jump()
{
    //2 bytes, 12 cycles, No flags
    int8_t jumpAmount = FromTwosComplement(Mem[Register.PC + 1]);    
    Register.PC += (2 + jumpAmount);
    return 12;
}

static cycles Op_JumpAddr()
{
    //3 bytes, 16 cycles, No flags
    Register.PC = (Register.PC = Mem[Register.PC + 1] | (Mem[Register.PC + 2] << 8));
    return 16;
}

static cycles Op_JumpIf(enum Flag flag, bool ifTrue)
{
    //2 bytes, 12/8 cycles, No flags
    int8_t jumpAmount = FromTwosComplement(Mem[Register.PC + 1]);
    Register.PC += 2;

    if (ifTrue == IsFlagSet(flag))
    {
        Register.PC += jumpAmount;
        return 12;
    }
    
    return 8;
}

static cycles Op_Call()
{
    //3 bytes, 24 cycles, No flags
    StackPush(Register.PC + 3);
    Register.PC = Mem[Register.PC + 1] | (Mem[Register.PC + 2] << 8);
    return 24;
}

static cycles Op_DisableInterrupts()
{
    //1 byte, 4 cycles, No flags
    Register.PC += 1;
    IME = false;
    return 4;
}

static cycles Op_EnableInterrupts()
{
    //1 byte, 4 cycles, No flags
    Register.PC += 1;
    IME = true;
    return 4;
}

static cycles Op_Return()
{
    //1 byte, 16 cycles, No flags
    Register.PC = StackPop();
    return 16;
}

static cycles Op_EnableInterruptsAndReturn()
{
    //1 byte, 16 cycles, No flags
    IME = true;
    Register.PC = StackPop();
    return 16;
}

static cycles Op_TestBit(const byte* pR, byte bit)
{
    //2 bytes, 8 cycles, Flags Z01-
    bool bitZero = (*pR & (1 << bit)) == 0;
    SetFlags(bitZero ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_On, FlagSet_Leave);
    Register.PC += 2;
    return 8;
}

static cycles Op_RotateLeftWithCarry(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z00C
    bool bit7 = *pR >> 7;
    *pR = ((*pR << 1) | bit7);
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
    Register.PC += 2;
    return 8;
}

static cycles Op_RotateLeftThroughCarry(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z00C
    bool bit7 = *pR >> 7;
    *pR = ((*pR << 1) | IsFlagSet(Flag_Carry));
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
    Register.PC += 2;
    return 8;
}

//Same as above but only works with register A and shorter as they're not used from extended opcodes.
static cycles Op_RotateLeftWithCarryA()
{
    //1 byte, 4 cycles, Flags Z00C
    bool bit7 = Register.A >> 7;
    Register.A = ((Register.A << 1) | bit7);
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
    Register.PC += 1;
    return 4;
}

static cycles Op_RotateLeftThroughCarryA()
{
    //1 byte, 4 cycles, Flags Z00C
    bool bit7 = Register.A >> 7;
    Register.A = ((Register.A << 1) | IsFlagSet(Flag_Carry));
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
    Register.PC += 1;
    return 4;
}

static cycles HandleOpCode()
{
    byte opCode = Mem[Register.PC];

    switch (opCode)
    {
        //Loads
        case 0x3E: return Op_LoadImmediate8(&Register.A);
        case 0x06: return Op_LoadImmediate8(&Register.B);
        case 0x0E: return Op_LoadImmediate8(&Register.C);
        case 0x16: return Op_LoadImmediate8(&Register.D);
        case 0x1E: return Op_LoadImmediate8(&Register.E);
        case 0x26: return Op_LoadImmediate8(&Register.H);
        case 0x2E: return Op_LoadImmediate8(&Register.L);

        case 0x01: return Op_LoadImmediate16(&Register.BC);
        case 0x11: return Op_LoadImmediate16(&Register.DE);
        case 0x21: return Op_LoadImmediate16(&Register.HL);
        case 0x31: return Op_LoadImmediate16(&Register.SP);

        case 0x7F: return Op_LoadRegister8(&Register.A, &Register.A);
        case 0x78: return Op_LoadRegister8(&Register.A, &Register.B);
        case 0x79: return Op_LoadRegister8(&Register.A, &Register.C);
        case 0x7A: return Op_LoadRegister8(&Register.A, &Register.D);
        case 0x7B: return Op_LoadRegister8(&Register.A, &Register.E);
        case 0x7C: return Op_LoadRegister8(&Register.A, &Register.H);
        case 0x7D: return Op_LoadRegister8(&Register.A, &Register.L);
        case 0x47: return Op_LoadRegister8(&Register.B, &Register.A);
        case 0x40: return Op_LoadRegister8(&Register.B, &Register.B);
        case 0x41: return Op_LoadRegister8(&Register.B, &Register.C);
        case 0x42: return Op_LoadRegister8(&Register.B, &Register.D);
        case 0x43: return Op_LoadRegister8(&Register.B, &Register.E);
        case 0x44: return Op_LoadRegister8(&Register.B, &Register.H);
        case 0x45: return Op_LoadRegister8(&Register.B, &Register.L);
        case 0x4F: return Op_LoadRegister8(&Register.C, &Register.A);
        case 0x48: return Op_LoadRegister8(&Register.C, &Register.B);
        case 0x49: return Op_LoadRegister8(&Register.C, &Register.C);
        case 0x4A: return Op_LoadRegister8(&Register.C, &Register.D);
        case 0x4B: return Op_LoadRegister8(&Register.C, &Register.E);
        case 0x4C: return Op_LoadRegister8(&Register.C, &Register.H);
        case 0x4D: return Op_LoadRegister8(&Register.C, &Register.L);
        case 0x57: return Op_LoadRegister8(&Register.D, &Register.A);
        case 0x50: return Op_LoadRegister8(&Register.D, &Register.B);
        case 0x51: return Op_LoadRegister8(&Register.D, &Register.C);
        case 0x52: return Op_LoadRegister8(&Register.D, &Register.D);
        case 0x53: return Op_LoadRegister8(&Register.D, &Register.E);
        case 0x54: return Op_LoadRegister8(&Register.D, &Register.H);
        case 0x55: return Op_LoadRegister8(&Register.D, &Register.L);
        case 0x5F: return Op_LoadRegister8(&Register.E, &Register.A);
        case 0x58: return Op_LoadRegister8(&Register.E, &Register.B);
        case 0x59: return Op_LoadRegister8(&Register.E, &Register.C);
        case 0x5A: return Op_LoadRegister8(&Register.E, &Register.D);
        case 0x5B: return Op_LoadRegister8(&Register.E, &Register.E);
        case 0x5C: return Op_LoadRegister8(&Register.E, &Register.H);
        case 0x5D: return Op_LoadRegister8(&Register.E, &Register.L);
        case 0x67: return Op_LoadRegister8(&Register.H, &Register.A);
        case 0x60: return Op_LoadRegister8(&Register.H, &Register.B);
        case 0x61: return Op_LoadRegister8(&Register.H, &Register.C);
        case 0x62: return Op_LoadRegister8(&Register.H, &Register.D);
        case 0x63: return Op_LoadRegister8(&Register.H, &Register.E);
        case 0x64: return Op_LoadRegister8(&Register.H, &Register.H);
        case 0x65: return Op_LoadRegister8(&Register.H, &Register.L);
        case 0x6F: return Op_LoadRegister8(&Register.L, &Register.A);
        case 0x68: return Op_LoadRegister8(&Register.L, &Register.B);
        case 0x69: return Op_LoadRegister8(&Register.L, &Register.C);
        case 0x6A: return Op_LoadRegister8(&Register.L, &Register.D);
        case 0x6B: return Op_LoadRegister8(&Register.L, &Register.E);
        case 0x6C: return Op_LoadRegister8(&Register.L, &Register.H);
        case 0x6D: return Op_LoadRegister8(&Register.L, &Register.L);

        case 0x0A: return Op_LoadRegisterAddr(&Register.A, Register.BC);
        case 0x1A: return Op_LoadRegisterAddr(&Register.A, Register.DE);
        case 0x7E: return Op_LoadRegisterAddr(&Register.A, Register.HL);
        case 0x46: return Op_LoadRegisterAddr(&Register.B, Register.HL);
        case 0x4E: return Op_LoadRegisterAddr(&Register.C, Register.HL);
        case 0x56: return Op_LoadRegisterAddr(&Register.D, Register.HL);
        case 0x5E: return Op_LoadRegisterAddr(&Register.E, Register.HL);
        case 0x66: return Op_LoadRegisterAddr(&Register.H, Register.HL);
        case 0x6E: return Op_LoadRegisterAddr(&Register.L, Register.HL);

        case 0x02: return Op_LoadAddrRegister(Register.BC, &Register.A);
        case 0x12: return Op_LoadAddrRegister(Register.DE, &Register.A);
        case 0x77: return Op_LoadAddrRegister(Register.HL, &Register.A);
        case 0x70: return Op_LoadAddrRegister(Register.HL, &Register.B);
        case 0x71: return Op_LoadAddrRegister(Register.HL, &Register.C);
        case 0x72: return Op_LoadAddrRegister(Register.HL, &Register.D);
        case 0x73: return Op_LoadAddrRegister(Register.HL, &Register.E);
        case 0x74: return Op_LoadAddrRegister(Register.HL, &Register.H);
        case 0x75: return Op_LoadAddrRegister(Register.HL, &Register.L);

        case 0xE2: return Op_LoadAddrRegister(0xFF00 + Register.C, &Register.A);
        case 0xF2: return Op_LoadRegisterAddr(&Register.A, 0xFF00 + Register.C);

        case 0x36: return Op_LoadAddrImmediate(&Register.HL);

        case 0x22: return Op_LoadAddrRegisterAndInc(&Register.HL, &Register.A);
        case 0x32: return Op_LoadAddrRegisterAndDec(&Register.HL, &Register.A);

        case 0x2A: return Op_LoadRegisterAddrAndInc(&Register.A, &Register.HL);
        case 0x3A: return Op_LoadRegisterAddrAndDec(&Register.A, &Register.HL);

        //These don't need to be any more complicated as they only work with the A register.
        case 0xE0: return Op_LoadImmediateAddr8FromA();
        case 0xF0: return Op_LoadAFromImmediateAddr8();
        case 0xEA: return Op_LoadImmediateAddr16FromA();
        case 0xFA: return Op_LoadAFromImmediateAddr16();

        //Push/Pop
        case 0xF5: return Op_Push(&Register.AF);
        case 0xC5: return Op_Push(&Register.BC);
        case 0xD5: return Op_Push(&Register.DE);
        case 0xE5: return Op_Push(&Register.HL);
        case 0xF1: return Op_Pop(&Register.AF);
        case 0xC1: return Op_Pop(&Register.BC);
        case 0xD1: return Op_Pop(&Register.DE);
        case 0xE1: return Op_Pop(&Register.HL);

        //Compare
        case 0xBF: return Op_CompareRegister(&Register.A);
        case 0xB8: return Op_CompareRegister(&Register.B);
        case 0xB9: return Op_CompareRegister(&Register.C);
        case 0xBA: return Op_CompareRegister(&Register.D);
        case 0xBB: return Op_CompareRegister(&Register.E);
        case 0xBC: return Op_CompareRegister(&Register.H);
        case 0xBD: return Op_CompareRegister(&Register.L);
        case 0xBE: return Op_CompareAddr();
        case 0xFE: return Op_CompareImmediate();

        //And
        case 0xA7: return Op_And(&Register.A);
        case 0xA0: return Op_And(&Register.B);
        case 0xA1: return Op_And(&Register.C);
        case 0xA2: return Op_And(&Register.D);
        case 0xA3: return Op_And(&Register.E);
        case 0xA4: return Op_And(&Register.H);
        case 0xA5: return Op_And(&Register.L);

        //Or
        case 0xB7: return Op_Or(&Register.A);
        case 0xB0: return Op_Or(&Register.B);
        case 0xB1: return Op_Or(&Register.C);
        case 0xB2: return Op_Or(&Register.D);
        case 0xB3: return Op_Or(&Register.E);
        case 0xB4: return Op_Or(&Register.H);
        case 0xB5: return Op_Or(&Register.L);

        //Xor
        case 0xAF: return Op_Xor(&Register.A);
        case 0xA8: return Op_Xor(&Register.B);
        case 0xA9: return Op_Xor(&Register.C);
        case 0xAA: return Op_Xor(&Register.D);
        case 0xAB: return Op_Xor(&Register.E);
        case 0xAC: return Op_Xor(&Register.H);
        case 0xAD: return Op_Xor(&Register.L);

        //Complement
        case 0x2F: return Op_Complement();

        //Increment
        case 0x3C: return Op_Increment8(&Register.A);
        case 0x04: return Op_Increment8(&Register.B);
        case 0x0C: return Op_Increment8(&Register.C);
        case 0x14: return Op_Increment8(&Register.D);
        case 0x1C: return Op_Increment8(&Register.E);
        case 0x24: return Op_Increment8(&Register.H);
        case 0x2C: return Op_Increment8(&Register.L);

        case 0x03: return Op_Increment16(&Register.BC);
        case 0x13: return Op_Increment16(&Register.DE);
        case 0x23: return Op_Increment16(&Register.HL);
        case 0x33: return Op_Increment16(&Register.SP);

        //Decrement
        case 0x3D: return Op_Decrement8(&Register.A);
        case 0x05: return Op_Decrement8(&Register.B);
        case 0x0D: return Op_Decrement8(&Register.C);
        case 0x15: return Op_Decrement8(&Register.D);
        case 0x1D: return Op_Decrement8(&Register.E);
        case 0x25: return Op_Decrement8(&Register.H);
        case 0x2D: return Op_Decrement8(&Register.L);

        case 0x0B: return Op_Decrement16(&Register.BC);
        case 0x1B: return Op_Decrement16(&Register.DE);
        case 0x2B: return Op_Decrement16(&Register.HL);
        case 0x3B: return Op_Decrement16(&Register.SP);

        //Add
        case 0x87: return Op_AddRegister(&Register.A);
        case 0x80: return Op_AddRegister(&Register.B);
        case 0x81: return Op_AddRegister(&Register.C);
        case 0x82: return Op_AddRegister(&Register.D);
        case 0x83: return Op_AddRegister(&Register.E);
        case 0x84: return Op_AddRegister(&Register.H);
        case 0x85: return Op_AddRegister(&Register.L);
        case 0x86: return Op_AddAddr();
        case 0xC6: return Op_AddImmediate();

        //Subtract
        case 0x97: return Op_SubtractRegister(&Register.A);
        case 0x90: return Op_SubtractRegister(&Register.B);
        case 0x91: return Op_SubtractRegister(&Register.C);
        case 0x92: return Op_SubtractRegister(&Register.D);
        case 0x93: return Op_SubtractRegister(&Register.E);
        case 0x94: return Op_SubtractRegister(&Register.H);
        case 0x95: return Op_SubtractRegister(&Register.L);
        case 0x96: return Op_SubtractAddr();
        case 0xD6: return Op_SubtractImmediate();

        //Jumps
        case 0x18: return Op_Jump();
        case 0xC3: return Op_JumpAddr();
        case 0x20: return Op_JumpIf(Flag_Zero, false);
        case 0x28: return Op_JumpIf(Flag_Zero, true);
        case 0x30: return Op_JumpIf(Flag_Carry, false);
        case 0x38: return Op_JumpIf(Flag_Carry, true);

        //Calls
        case 0xCD: return Op_Call();

        //Interrupts
        case 0xF3: return Op_DisableInterrupts();
        case 0xFB: return Op_EnableInterrupts();

        //Returns
        case 0xC9: return Op_Return();
        case 0xD9: return Op_EnableInterruptsAndReturn();

        //Rotate A Left
        case 0x07: return Op_RotateLeftWithCarryA();
        case 0x17: return Op_RotateLeftThroughCarryA();

        //CB
        case 0xCB:
        {
            byte extOpCode = Mem[Register.PC + 1];

            switch (extOpCode)
            {
                //BIT
                case 0x47: return Op_TestBit(&Register.A, 0);
                case 0x4F: return Op_TestBit(&Register.A, 1);
                case 0x57: return Op_TestBit(&Register.A, 2);
                case 0x5F: return Op_TestBit(&Register.A, 3);
                case 0x67: return Op_TestBit(&Register.A, 4);
                case 0x6F: return Op_TestBit(&Register.A, 5);
                case 0x77: return Op_TestBit(&Register.A, 6);
                case 0x7F: return Op_TestBit(&Register.A, 7);
                case 0x40: return Op_TestBit(&Register.B, 0);
                case 0x48: return Op_TestBit(&Register.B, 1);
                case 0x50: return Op_TestBit(&Register.B, 2);
                case 0x58: return Op_TestBit(&Register.B, 3);
                case 0x60: return Op_TestBit(&Register.B, 4);
                case 0x68: return Op_TestBit(&Register.B, 5);
                case 0x70: return Op_TestBit(&Register.B, 6);
                case 0x78: return Op_TestBit(&Register.B, 7);
                case 0x41: return Op_TestBit(&Register.C, 0);
                case 0x49: return Op_TestBit(&Register.C, 1);
                case 0x51: return Op_TestBit(&Register.C, 2);
                case 0x59: return Op_TestBit(&Register.C, 3);
                case 0x61: return Op_TestBit(&Register.C, 4);
                case 0x69: return Op_TestBit(&Register.C, 5);
                case 0x71: return Op_TestBit(&Register.C, 6);
                case 0x79: return Op_TestBit(&Register.C, 7);
                case 0x42: return Op_TestBit(&Register.D, 0);
                case 0x4A: return Op_TestBit(&Register.D, 1);
                case 0x52: return Op_TestBit(&Register.D, 2);
                case 0x5A: return Op_TestBit(&Register.D, 3);
                case 0x62: return Op_TestBit(&Register.D, 4);
                case 0x6A: return Op_TestBit(&Register.D, 5);
                case 0x72: return Op_TestBit(&Register.D, 6);
                case 0x7A: return Op_TestBit(&Register.D, 7);
                case 0x43: return Op_TestBit(&Register.E, 0);
                case 0x4B: return Op_TestBit(&Register.E, 1);
                case 0x53: return Op_TestBit(&Register.E, 2);
                case 0x5B: return Op_TestBit(&Register.E, 3);
                case 0x63: return Op_TestBit(&Register.E, 4);
                case 0x6B: return Op_TestBit(&Register.E, 5);
                case 0x73: return Op_TestBit(&Register.E, 6);
                case 0x7B: return Op_TestBit(&Register.E, 7);
                case 0x44: return Op_TestBit(&Register.H, 0);
                case 0x4C: return Op_TestBit(&Register.H, 1);
                case 0x54: return Op_TestBit(&Register.H, 2);
                case 0x5C: return Op_TestBit(&Register.H, 3);
                case 0x64: return Op_TestBit(&Register.H, 4);
                case 0x6C: return Op_TestBit(&Register.H, 5);
                case 0x74: return Op_TestBit(&Register.H, 6);
                case 0x7C: return Op_TestBit(&Register.H, 7);
                case 0x45: return Op_TestBit(&Register.L, 0);
                case 0x4D: return Op_TestBit(&Register.L, 1);
                case 0x55: return Op_TestBit(&Register.L, 2);
                case 0x5D: return Op_TestBit(&Register.L, 3);
                case 0x65: return Op_TestBit(&Register.L, 4);
                case 0x6D: return Op_TestBit(&Register.L, 5);
                case 0x75: return Op_TestBit(&Register.L, 6);
                case 0x7D: return Op_TestBit(&Register.L, 7);

                //Rotate Left
                case 0x07: return Op_RotateLeftWithCarry(&Register.A);
                case 0x00: return Op_RotateLeftWithCarry(&Register.B);
                case 0x01: return Op_RotateLeftWithCarry(&Register.C);
                case 0x02: return Op_RotateLeftWithCarry(&Register.D);
                case 0x03: return Op_RotateLeftWithCarry(&Register.E);
                case 0x04: return Op_RotateLeftWithCarry(&Register.H);
                case 0x05: return Op_RotateLeftWithCarry(&Register.L);

                case 0x17: return Op_RotateLeftThroughCarry(&Register.A);
                case 0x10: return Op_RotateLeftThroughCarry(&Register.B);
                case 0x11: return Op_RotateLeftThroughCarry(&Register.C);
                case 0x12: return Op_RotateLeftThroughCarry(&Register.D);
                case 0x13: return Op_RotateLeftThroughCarry(&Register.E);
                case 0x14: return Op_RotateLeftThroughCarry(&Register.H);
                case 0x15: return Op_RotateLeftThroughCarry(&Register.L);

                default:
                    fprintf(stderr, "Unhandled extended opcode 0x%02X!\n", extOpCode);
                    assert(0);
            }
        }
        break;

        case 0x00: return Op_NOP();

        default:
            fprintf(stderr, "Unhandled opcode 0x%02X!\n", opCode);
            assert(0);
    }

    return 0;
}

bool CPUInit(uint16_t startAddr)
{
    Register.PC = startAddr;
    return true;
}

cycles CPUTick()
{
    return HandleOpCode();
}
