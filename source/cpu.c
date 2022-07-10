#include <stdio.h>
#include <assert.h>

#include "cpu.h"
#include "types.h"
#include "system.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_debug.h)

static struct CPURegisters Register;

static bool CPURunning;

static bool IME; //Interrupt master flag.
static byte InterruptOp[8];
static int NumInterrupts = 0;

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
    Register.SP -= 2;
    WriteMem(Register.SP, value >> 8);
    WriteMem(Register.SP + 1, value & 0xFF);
}

static uint16_t StackPop()
{
    uint16_t val = (ReadMem(Register.SP) << 8) | ReadMem(Register.SP + 1);
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

static cycles Op_Halt()
{
    //1 byte, 4 cycles, No flags
    CPURunning = false;
    return 4;
}

static cycles Op_LoadImmediate8(byte* pR)
{
    //2 bytes, 8 cycles, No flags
    *pR = ReadMem(Register.PC + 1);
    Register.PC += 2;
    return 8;
}

static cycles Op_LoadImmediate16(uint16_t* pR)
{
    //3 bytes, 12 cycles, No flags
    *pR = ReadMem16(Register.PC + 1);
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
    WriteMem(addr, *pR);
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadRegisterAddr(byte* pR, uint16_t addr)
{
    //1 byte, 8 cycles, No flags
    *pR = ReadMem(addr);
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadAddrImmediate(uint16_t* pRAddr)
{
    //2 bytes, 12 cycles, No flags
    byte val = ReadMem(Register.PC + 1);
    WriteMem(*pRAddr, val);
    Register.PC += 2;
    return 12;
}

static cycles Op_LoadAddrRegisterAndInc(uint16_t* pRAddr, const byte* pR)
{
    //1 byte, 8 cycles, No flags
    WriteMem(*pRAddr, *pR);
    (*pRAddr)++;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadAddrRegisterAndDec(uint16_t* pRAddr, const byte* pR)
{
    //1 byte, 8 cycles, No flags
    WriteMem(*pRAddr, *pR);
    (*pRAddr)--;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadRegisterAddrAndInc(byte* pR, uint16_t* pRAddr)
{
    //1 byte, 8 cycles, No flags
    *pR = ReadMem(*pRAddr);
    (*pRAddr)++;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadRegisterAddrAndDec(byte* pR, uint16_t* pRAddr)
{
    //1 byte, 8 cycles, No flags
    *pR = ReadMem(*pRAddr);
    (*pRAddr)--;
    Register.PC += 1;
    return 8;
}

static cycles Op_LoadImmediateAddr8FromA()
{
    //2 bytes, 12 cycles, No flags
    uint16_t addr = 0xFF00 + ReadMem(Register.PC + 1);
    WriteMem(addr, Register.A);
    Register.PC += 2;
    return 12;
}

static cycles Op_LoadAFromImmediateAddr8()
{
    //2 bytes, 12 cycles, No flags
    uint16_t addr = 0xFF00 + ReadMem(Register.PC + 1);
    Register.A = ReadMem(addr);
    Register.PC += 2;
    return 12;
}

static cycles Op_LoadImmediateAddr16FromA()
{
    //3 bytes, 16 cycles, No flags
    uint16_t addr = ReadMem16(Register.PC + 1);
    WriteMem(addr, Register.A);
    Register.PC += 3;
    return 16;
}

static cycles Op_LoadAFromImmediateAddr16()
{
    //3 bytes, 16 cycles, No flags
    uint16_t addr = ReadMem16(Register.PC + 1);
    Register.A = ReadMem(addr);
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
    //1 byte, 4 cycles, Flags Z1HC
    DoCompare(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_CompareAddr()
{
    //1 byte, 8 cycles, Flags Z1HC
    byte val = ReadMem(Register.HL);
    DoCompare(val);
    Register.PC += 1;
    return 8;
}

static cycles Op_CompareImmediate()
{
    //2 bytes, 8 cycles, Flags Z1HC
    byte val = ReadMem(Register.PC + 1);
    DoCompare(val);
    Register.PC += 2;
    return 8;
}

static void DoAnd(byte val)
{
    Register.A &= val;
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_On, FlagSet_Off);
}

static cycles Op_AndRegister(byte* pR)
{
    //1 byte, 4 cycles, Flags Z010
    DoAnd(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_AndAddr()
{
    //1 byte, 8 cycles, Flags Z010
    byte val = ReadMem(Register.HL);
    DoAnd(val);
    Register.PC += 1;
    return 8;
}

static cycles Op_AndImmediate()
{
    //2 bytes, 8 cycles, Flags Z010
    byte val = ReadMem(Register.PC + 1);
    DoAnd(val);
    Register.PC += 2;
    return 8;
}

static void DoOr(byte val)
{
    Register.A |= val;
    SetZeroFlag(Register.A == 0);
}

static cycles Op_OrRegister(byte* pR)
{
    //1 byte, 4 cycles, Flags Z000
    DoOr(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_OrAddr()
{
    //1 byte, 8 cycles, Flags Z000
    byte val = ReadMem(Register.HL);
    DoOr(val);
    Register.PC += 1;
    return 8;
}

static cycles Op_OrImmediate()
{
    //2 bytes, 8 cycles, Flags Z000
    byte val = ReadMem(Register.PC + 1);
    DoOr(val);
    Register.PC += 2;
    return 8;
}

static void DoXor(byte val)
{
    Register.A ^= val;
    SetZeroFlag(Register.A == 0);
}

static cycles Op_XorRegister(byte* pR)
{
    //1 byte, 4 cycles, Flags Z000
    DoXor(*pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_XorAddr()
{
    //1 byte, 8 cycles, Flags Z000
    byte val = ReadMem(Register.HL);
    DoXor(val);
    Register.PC += 1;
    return 8;
}

static cycles Op_XorImmediate()
{
    //2 bytes, 8 cycles, Flags Z000
    byte val = ReadMem(Register.PC + 1);
    DoXor(val);
    Register.PC += 2;
    return 8;
}

static cycles Op_Complement()
{
    //1 byte, 4 cycles, Flags -11-
    Register.A = ~Register.A;
    SetFlags(FlagSet_Leave, FlagSet_On, FlagSet_On, FlagSet_Leave);
    Register.PC += 1;
    return 4;
}

static cycles Op_DecimalAdjust()
{
    //1 byte, 4 cycles, Flags Z-0C
    bool carryFlag = false;

    if (!IsFlagSet(Flag_Subtract))
    {
        if (IsFlagSet(Flag_Carry) || Register.A > 0x99)
        {
            Register.A += 0x60;
            carryFlag = true;
        }

        if (IsFlagSet(Flag_HalfCarry) || (Register.A & 0x0F) > 0x09)
        {
            Register.A += 0x6;
        }
    }
    else
    {
        if (IsFlagSet(Flag_Carry))
        {
            Register.A -= 0x60;
        }

        if (IsFlagSet(Flag_HalfCarry))
        {
            Register.A -= 0x6;
        }
    }

    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Leave, FlagSet_Off, carryFlag ? FlagSet_On : FlagSet_Off);
    Register.PC += 1;
    return 4;
}

static void DoIncrement8(byte* pR)
{
    //In this case we can just check if the bottom four bits are set because increasing by one will carry.
    bool halfCarry = ((*pR & 0xF) == 0xF);
    (*pR)++;
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, halfCarry ? FlagSet_On : FlagSet_Off, FlagSet_Leave);
}

static cycles Op_Increment8(byte* pR)
{
    //1 byte, 4 cycles, Flags Z0H-
    DoIncrement8(pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_IncrementAddr(uint16_t addr)
{
    //1 byte, 12 cycles, Flags Z0H-
    byte val = ReadMem(addr);
    DoIncrement8(&val);
    WriteMem(addr, val);
    Register.PC += 1;
    return 12;
}

static cycles Op_Increment16(uint16_t* pR)
{
    //1 byte, 8 cycles, No flags
    (*pR)++;
    Register.PC += 1;
    return 8;
}

static void DoDecrement8(byte* pR)
{
    //In this case we can just check if the bottom four bits are unset because decreasing by one will carry.
    bool halfCarry = ((*pR & 0xF) == 0x0);
    (*pR)--;
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_On, halfCarry ? FlagSet_On : FlagSet_Off, FlagSet_Leave);
}

static cycles Op_Decrement8(byte* pR)
{
    //1 byte, 4 cycles, Flags Z1H-
    DoDecrement8(pR);
    Register.PC += 1;
    return 4;
}

static cycles Op_DecrementAddr(uint16_t addr)
{
    //1 byte, 12 cycles, Flags Z1H-
    byte val = ReadMem(addr);
    DoDecrement8(&val);
    WriteMem(addr, val);
    Register.PC += 1;
    return 12;
}

static cycles Op_Decrement16(uint16_t* pR)
{
    //1 byte, 8 cycles, No flags
    (*pR)--;
    Register.PC += 1;
    return 8;
}

static void DoAdd(byte val, bool plusCarry)
{
    if (plusCarry && IsFlagSet(Flag_Carry))
    {
        val++;
    }

    bool halfCarry = (Register.A & 0xF) + (val & 0xF) > 0xF;
    bool carry = (Register.A + val) > 0xFF;
    Register.A = FromTwosComplement(Register.A) + FromTwosComplement(val);
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, halfCarry ? FlagSet_On : FlagSet_Off, carry ? FlagSet_On : FlagSet_Off);
}

static cycles Op_AddRegister(const byte* pR, bool plusCarry)
{
    //1 byte, 4 cycles, Flags Z0HC
    DoAdd(*pR, plusCarry);
    Register.PC += 1;
    return 4;
}

static cycles Op_AddAddr(bool plusCarry)
{
    //1 byte, 8 cycles, Flags Z0HC
    byte val = ReadMem(Register.HL);
    DoAdd(val, plusCarry);
    Register.PC += 1;
    return 8;
}

static cycles Op_AddImmediate(bool plusCarry)
{
    //2 bytes, 8 cycles, Flags Z0HC
    byte val = ReadMem(Register.PC + 1);
    DoAdd(val, plusCarry);
    Register.PC += 2;
    return 8;
}

static cycles Op_AddHLRegister16(const uint16_t* pR)
{
    //1 byte, 8 cycles, Flags -0CH
    bool halfCarry = (Register.HL & 0xFF) + (*pR & 0xFF) > 0xFF;
    bool carry = (Register.HL + *pR) > 0xFFFF;
    Register.HL += *pR;
    SetFlags(FlagSet_Leave, FlagSet_Off, halfCarry ? FlagSet_On : FlagSet_Off, carry ? FlagSet_On : FlagSet_Off);
    Register.PC += 1;
    return 8;
}

static void DoSubtract(byte val, bool plusCarry)
{
    if (plusCarry && IsFlagSet(Flag_Carry))
    {
        val++;
    }

    bool halfCarry = (int)(Register.A & 0xf) - (int)(val & 0xf) < 0;
    bool carry = val > Register.A;
    //Register.A = FromTwosComplement(Register.A) - FromTwosComplement(val);    Don't need to do this.
    Register.A -= val;
    SetFlags(Register.A == 0 ? FlagSet_On : FlagSet_Off, FlagSet_On, halfCarry ? FlagSet_On : FlagSet_Off, carry ? FlagSet_On : FlagSet_Off);
}

static cycles Op_SubtractRegister(const byte* pR, bool plusCarry)
{
    //1 byte, 4 cycles, Flags Z1HC
    DoSubtract(*pR, plusCarry);
    Register.PC += 1;
    return 4;
}

static cycles Op_SubtractAddr(bool plusCarry)
{
    //1 byte, 8 cycles, Flags Z1HC
    byte val = ReadMem(Register.HL);
    DoSubtract(val, plusCarry);
    Register.PC += 1;
    return 8;
}

static cycles Op_SubtractImmediate(bool plusCarry)
{
    //2 bytes, 8 cycles, Flags Z1HC
    byte val = ReadMem(Register.PC + 1);
    DoSubtract(val, plusCarry);
    Register.PC += 2;
    return 8;
}

static cycles Op_Jump()
{
    //2 bytes, 12 cycles, No flags
    byte val = ReadMem(Register.PC + 1);
    int8_t jumpAmount = FromTwosComplement(val);
    Register.PC += (2 + jumpAmount);
    return 12;
}

static cycles Op_JumpAddr()
{
    //3 bytes, 16 cycles, No flags
    Register.PC = ReadMem16(Register.PC + 1);
    return 16;
}

static cycles Op_JumpRegister(const uint16_t* pR)
{
    //1 byte, 4 cycles, No flags
    Register.PC = *pR;
    return 4;
}

static cycles Op_JumpIf(enum Flag flag, bool ifTrue)
{
    //2 bytes, 12/8 cycles, No flags
    byte val = ReadMem(Register.PC + 1);
    int8_t jumpAmount = FromTwosComplement(val);
    Register.PC += 2;

    if (ifTrue == IsFlagSet(flag))
    {
        Register.PC += jumpAmount;
        return 12;
    }
    
    return 8;
}

static cycles Op_JumpAddrIf(enum Flag flag, bool ifTrue)
{
    //3 bytes, 16/12 cycles, No flags
    if (ifTrue == IsFlagSet(flag))
    {
        Register.PC = ReadMem16(Register.PC + 1);
        return 16;
    }

    Register.PC += 3;
    return 12;
}

static cycles Op_Call()
{
    //3 bytes, 24 cycles, No flags
    StackPush(Register.PC + 3);
    Register.PC = ReadMem16(Register.PC + 1);
    return 24;
}

static cycles Op_CallIf(enum Flag flag, bool ifTrue)
{
    //3 bytes, 24/12 cycles, No flags
    if (ifTrue == IsFlagSet(flag))
    {
        StackPush(Register.PC + 3);
        Register.PC = ReadMem16(Register.PC + 1);
        return 24;
    }

    Register.PC += 3;
    return 12;
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

static cycles Op_ReturnIf(enum Flag flag, bool ifTrue)
{
    //2 bytes, 12/8 cycles, No flags
    if (ifTrue == IsFlagSet(flag))
    {
        Register.PC = StackPop();
        return 20;
    }

    Register.PC += 1;
    return 8;
}

static cycles Op_EnableInterruptsAndReturn()
{
    //1 byte, 16 cycles, No flags
    IME = true;
    Register.PC = StackPop();
    return 16;
}

static cycles Op_Swap(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z000
    *pR = ((*pR & 0xF) << 4) | ((*pR & 0xF0) >> 4);
    SetZeroFlag(*pR == 0);
    Register.PC += 2;
    return 8;
}

static cycles Op_SetRegisterBit(byte* pR, byte bit)
{
    //2 bytes, 8 cycles, No flags
    *pR |= (1 << bit);
    Register.PC += 2;
    return 8;
}

static cycles Op_SetAddrBit(uint16_t addr, byte bit)
{
    //2 bytes, 16 cycles, No flags
    byte val = ReadMem(addr);
    val |= (1 << bit);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

static cycles Op_TestRegisterBit(const byte* pR, byte bit)
{
    //2 bytes, 8 cycles, Flags Z01-
    bool bitZero = (*pR & (1 << bit)) == 0;
    SetFlags(bitZero ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_On, FlagSet_Leave);
    Register.PC += 2;
    return 8;
}

static cycles Op_TestAddrBit(uint16_t addr, byte bit)
{
    //2 bytes, 12 cycles, Flags Z01-
    byte val = ReadMem(addr);
    bool bitZero = (val & (1 << bit)) == 0;
    SetFlags(bitZero ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_On, FlagSet_Leave);
    Register.PC += 2;
    return 12;
}

static cycles Op_ResetRegisterBit(byte* pR, byte bit)
{
    //2 bytes, 8 cycles, No flags
    UnsetRegisterBit(pR, bit);
    Register.PC += 2;
    return 8;
}

static cycles Op_ResetAddrBit(uint16_t addr, byte bit)
{
    //2 bytes, 16 cycles, No flags
    byte val = ReadMem(addr);
    UnsetRegisterBit(&val, bit);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

void DoRotateLeftWithCarry(byte* pR)
{
    bool bit7 = *pR >> 7;
    *pR = ((*pR << 1) | bit7);
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
}

static cycles Op_RotateRegisterLeftWithCarry(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z00C
    DoRotateLeftWithCarry(pR);
    Register.PC += 2;
    return 8;
}

static cycles Op_RotateAddrLeftWithCarry(uint16_t addr)
{
    //2 bytes, 16 cycles, Flags Z00C
    byte val = ReadMem(addr);
    DoRotateLeftWithCarry(&val);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

void DoRotateLeftThroughCarry(byte* pR)
{
    bool bit7 = *pR >> 7;
    *pR = ((*pR << 1) | IsFlagSet(Flag_Carry));
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
}

static cycles Op_RotateRegisterLeftThroughCarry(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z00C
    DoRotateLeftThroughCarry(pR);
    Register.PC += 2;
    return 8;
}

static cycles Op_RotateAddrLeftThroughCarry(uint16_t addr)
{
    //2 bytes, 16 cycles, Flags Z00C
    byte val = ReadMem(addr);
    DoRotateLeftThroughCarry(&val);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

//Same as above but only works with register A and shorter as they're not used from extended opcodes.
static cycles Op_RotateRegisterLeftWithCarryA()
{
    //1 byte, 4 cycles, Flags Z00C
    DoRotateLeftWithCarry(&Register.A);
    Register.PC += 1;
    return 4;
}

static cycles Op_RotateRegisterLeftThroughCarryA()
{
    //1 byte, 4 cycles, Flags Z00C
    DoRotateLeftThroughCarry(&Register.A);
    Register.PC += 1;
    return 4;
}

void DoRotateRightWithCarry(byte* pR)
{
    bool bit0 = *pR & 0x1;
    *pR = ((*pR >> 1) | (bit0 << 7));
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit0 ? FlagSet_On : FlagSet_Off);
}

static cycles Op_RotateRegisterRightWithCarry(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z00C
    DoRotateRightWithCarry(pR);
    Register.PC += 2;
    return 8;
}

static cycles Op_RotateAddrRightWithCarry(uint16_t addr)
{
    //2 bytes, 16 cycles, Flags Z00C
    byte val = ReadMem(addr);
    DoRotateRightWithCarry(&val);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

void DoRotateRightThroughCarry(byte* pR)
{
    bool bit0 = *pR & 0x1;
    *pR = ((*pR >> 1) | (IsFlagSet(Flag_Carry) << 7));
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit0 ? FlagSet_On : FlagSet_Off);
}

static cycles Op_RotateRegisterRightThroughCarry(byte* pR)
{
    //2 bytes, 8 cycles, Flags Z00C
    DoRotateRightThroughCarry(pR);
    Register.PC += 2;
    return 8;
}

static cycles Op_RotateAddrRightThroughCarry(uint16_t addr)
{
    //2 bytes, 16 cycles, Flags Z00C
    byte val = ReadMem(addr);
    DoRotateRightThroughCarry(&val);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

//Same as above but only works with register A and shorter as they're not used from extended opcodes.
static cycles Op_RotateRegisterRightWithCarryA()
{
    //1 byte, 4 cycles, Flags Z00C
    DoRotateRightWithCarry(&Register.A);
    Register.PC += 1;
    return 4;
}

static cycles Op_RotateRegisterRightThroughCarryA()
{
    //1 byte, 4 cycles, Flags Z00C
    DoRotateRightThroughCarry(&Register.A);
    Register.PC += 1;
    return 4;
}

static void DoShiftLeft(byte* pR)
{
    bool bit7 = *pR >> 7;
    *pR <<= 1;
    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit7 ? FlagSet_On : FlagSet_Off);
}

static cycles Op_ShiftRegisterLeft(byte* pR)
{
    //2 byte, 8 cycles, Flags Z00C
    DoShiftLeft(pR);
    Register.PC += 2;
    return 8;
}

static cycles Op_ShiftAddrLeft(uint16_t addr)
{
    //2 byte, 16 cycles, Flags Z00C
    byte val = ReadMem(addr);
    DoShiftLeft(&val);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

static void DoShiftRight(byte* pR, bool resetMSB)
{
    byte bit0 = (*pR & 1);
    byte bit7 = 0;

    if (!resetMSB)
    {
        bit7 = (*pR & (1 << 7));
    }

    *pR = ((*pR >> 1) | bit7);

    SetFlags(*pR == 0 ? FlagSet_On : FlagSet_Off, FlagSet_Off, FlagSet_Off, bit0 != 0 ? FlagSet_On : FlagSet_Off);
}

static cycles Op_ShiftRegisterRight(byte* pR, bool resetMSB)
{
    //2 byte, 8 cycles, Flags Z00C
    DoShiftRight(pR, resetMSB);
    Register.PC += 2;
    return 8;
}

static cycles Op_ShiftAddrRight(uint16_t addr, bool resetMSB)
{
    //2 byte, 16 cycles, Flags Z00C
    byte val = ReadMem(addr);
    DoShiftRight(&val, resetMSB);
    WriteMem(addr, val);
    Register.PC += 2;
    return 16;
}

static cycles Op_SetCarryFlag()
{
    //1 byte, 4 cycles, Flags -001
    SetFlags(FlagSet_Leave, FlagSet_Off, FlagSet_Off, FlagSet_On);
    Register.PC += 1;
    return 4;
}

static cycles Op_Restart(uint16_t addr)
{
    //1 byte, 16 cycles, No flags
    StackPush(Register.PC + 1);
    Register.PC = addr;
    return 16;
}

static cycles HandleOpCode()
{
    byte opCode = ReadMem(Register.PC);

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
        case 0xA7: return Op_AndRegister(&Register.A);
        case 0xA0: return Op_AndRegister(&Register.B);
        case 0xA1: return Op_AndRegister(&Register.C);
        case 0xA2: return Op_AndRegister(&Register.D);
        case 0xA3: return Op_AndRegister(&Register.E);
        case 0xA4: return Op_AndRegister(&Register.H);
        case 0xA5: return Op_AndRegister(&Register.L);
        case 0xA6: return Op_AndAddr();
        case 0xE6: return Op_AndImmediate();

        //Or
        case 0xB7: return Op_OrRegister(&Register.A);
        case 0xB0: return Op_OrRegister(&Register.B);
        case 0xB1: return Op_OrRegister(&Register.C);
        case 0xB2: return Op_OrRegister(&Register.D);
        case 0xB3: return Op_OrRegister(&Register.E);
        case 0xB4: return Op_OrRegister(&Register.H);
        case 0xB5: return Op_OrRegister(&Register.L);
        case 0xB6: return Op_OrAddr();
        case 0xF6: return Op_OrImmediate();

        //Xor
        case 0xAF: return Op_XorRegister(&Register.A);
        case 0xA8: return Op_XorRegister(&Register.B);
        case 0xA9: return Op_XorRegister(&Register.C);
        case 0xAA: return Op_XorRegister(&Register.D);
        case 0xAB: return Op_XorRegister(&Register.E);
        case 0xAC: return Op_XorRegister(&Register.H);
        case 0xAD: return Op_XorRegister(&Register.L);
        case 0xAE: return Op_XorAddr();
        case 0xEE: return Op_XorImmediate();

        //Complement
        case 0x2F: return Op_Complement();

        //Decimal Adjust
        case 0x27: return Op_DecimalAdjust();

        //Increment
        case 0x3C: return Op_Increment8(&Register.A);
        case 0x04: return Op_Increment8(&Register.B);
        case 0x0C: return Op_Increment8(&Register.C);
        case 0x14: return Op_Increment8(&Register.D);
        case 0x1C: return Op_Increment8(&Register.E);
        case 0x24: return Op_Increment8(&Register.H);
        case 0x2C: return Op_Increment8(&Register.L);

        case 0x34: return Op_IncrementAddr(Register.HL);

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

        case 0x35: return Op_DecrementAddr(Register.HL);

        case 0x0B: return Op_Decrement16(&Register.BC);
        case 0x1B: return Op_Decrement16(&Register.DE);
        case 0x2B: return Op_Decrement16(&Register.HL);
        case 0x3B: return Op_Decrement16(&Register.SP);

        //Add
        case 0x87: return Op_AddRegister(&Register.A, false);
        case 0x80: return Op_AddRegister(&Register.B, false);
        case 0x81: return Op_AddRegister(&Register.C, false);
        case 0x82: return Op_AddRegister(&Register.D, false);
        case 0x83: return Op_AddRegister(&Register.E, false);
        case 0x84: return Op_AddRegister(&Register.H, false);
        case 0x85: return Op_AddRegister(&Register.L, false);
        case 0x86: return Op_AddAddr(false);
        case 0xC6: return Op_AddImmediate(false);
        
        case 0x09: return Op_AddHLRegister16(&Register.BC);
        case 0x19: return Op_AddHLRegister16(&Register.DE);
        case 0x29: return Op_AddHLRegister16(&Register.HL);
        case 0x39: return Op_AddHLRegister16(&Register.SP);

        //Add Plus Carry
        case 0x8F: return Op_AddRegister(&Register.A, true);
        case 0x88: return Op_AddRegister(&Register.B, true);
        case 0x89: return Op_AddRegister(&Register.C, true);
        case 0x8A: return Op_AddRegister(&Register.D, true);
        case 0x8B: return Op_AddRegister(&Register.E, true);
        case 0x8C: return Op_AddRegister(&Register.H, true);
        case 0x8D: return Op_AddRegister(&Register.L, true);
        case 0x8E: return Op_AddAddr(true);
        case 0xCE: return Op_AddImmediate(true);

        //Subtract
        case 0x97: return Op_SubtractRegister(&Register.A, false);
        case 0x90: return Op_SubtractRegister(&Register.B, false);
        case 0x91: return Op_SubtractRegister(&Register.C, false);
        case 0x92: return Op_SubtractRegister(&Register.D, false);
        case 0x93: return Op_SubtractRegister(&Register.E, false);
        case 0x94: return Op_SubtractRegister(&Register.H, false);
        case 0x95: return Op_SubtractRegister(&Register.L, false);
        case 0x96: return Op_SubtractAddr(false);
        case 0xD6: return Op_SubtractImmediate(false);

        //Subtract Plus Carry
        case 0x9F: return Op_SubtractRegister(&Register.A, true);
        case 0x98: return Op_SubtractRegister(&Register.B, true);
        case 0x99: return Op_SubtractRegister(&Register.C, true);
        case 0x9A: return Op_SubtractRegister(&Register.D, true);
        case 0x9B: return Op_SubtractRegister(&Register.E, true);
        case 0x9C: return Op_SubtractRegister(&Register.H, true);
        case 0x9D: return Op_SubtractRegister(&Register.L, true);
        case 0x9E: return Op_SubtractAddr(true);
        case 0xDE: return Op_SubtractImmediate(true);

        //Jumps
        case 0x18: return Op_Jump();
        case 0xC3: return Op_JumpAddr();
        case 0xE9: return Op_JumpRegister(&Register.HL);
        case 0x20: return Op_JumpIf(Flag_Zero, false);
        case 0x28: return Op_JumpIf(Flag_Zero, true);
        case 0x30: return Op_JumpIf(Flag_Carry, false);
        case 0x38: return Op_JumpIf(Flag_Carry, true);
        case 0xC2: return Op_JumpAddrIf(Flag_Zero, false);
        case 0xCA: return Op_JumpAddrIf(Flag_Zero, true);
        case 0xD2: return Op_JumpAddrIf(Flag_Carry, false);
        case 0xDA: return Op_JumpAddrIf(Flag_Carry, true);

        //Calls
        case 0xCD: return Op_Call();
        case 0xC4: return Op_CallIf(Flag_Zero, false);
        case 0xCC: return Op_CallIf(Flag_Zero, true);
        case 0xD4: return Op_CallIf(Flag_Carry, false);
        case 0xDC: return Op_CallIf(Flag_Carry, true);

        //Interrupts
        case 0xF3: return Op_DisableInterrupts();
        case 0xFB: return Op_EnableInterrupts();

        //Returns
        case 0xC9: return Op_Return();
        case 0xC0: return Op_ReturnIf(Flag_Zero, false);
        case 0xC8: return Op_ReturnIf(Flag_Zero, true);
        case 0xD0: return Op_ReturnIf(Flag_Carry, false);
        case 0xD8: return Op_ReturnIf(Flag_Carry, true);

        case 0xD9: return Op_EnableInterruptsAndReturn();

        //Rotate A Left
        case 0x07: return Op_RotateRegisterLeftWithCarryA();
        case 0x17: return Op_RotateRegisterLeftThroughCarryA();

        //Rotate A Right
        case 0x0F: return Op_RotateRegisterRightWithCarryA();
        case 0x1F: return Op_RotateRegisterRightThroughCarryA();

        case 0x37: return Op_SetCarryFlag();

        //Restart
        case 0xC7: return Op_Restart(0x00);
        case 0xCF: return Op_Restart(0x08);
        case 0xD7: return Op_Restart(0x10);
        case 0xDF: return Op_Restart(0x18);
        case 0xE7: return Op_Restart(0x20);
        case 0xEF: return Op_Restart(0x28);
        case 0xF7: return Op_Restart(0x30);
        case 0xFF: return Op_Restart(0x38);

        //CB
        case 0xCB:
        {
            byte extOpCode = ReadMem(Register.PC + 1);

            switch (extOpCode)
            {
                //Swap
                case 0x37: return Op_Swap(&Register.A);
                case 0x30: return Op_Swap(&Register.B);
                case 0x31: return Op_Swap(&Register.C);
                case 0x32: return Op_Swap(&Register.D);
                case 0x33: return Op_Swap(&Register.E);
                case 0x34: return Op_Swap(&Register.H);
                case 0x35: return Op_Swap(&Register.L);

                //Set bit
                case 0xC7: return Op_SetRegisterBit(&Register.A, 0);
                case 0xCF: return Op_SetRegisterBit(&Register.A, 1);
                case 0xD7: return Op_SetRegisterBit(&Register.A, 2);
                case 0xDF: return Op_SetRegisterBit(&Register.A, 3);
                case 0xE7: return Op_SetRegisterBit(&Register.A, 4);
                case 0xEF: return Op_SetRegisterBit(&Register.A, 5);
                case 0xF7: return Op_SetRegisterBit(&Register.A, 6);
                case 0xFF: return Op_SetRegisterBit(&Register.A, 7);
                case 0xC0: return Op_SetRegisterBit(&Register.B, 0);
                case 0xC8: return Op_SetRegisterBit(&Register.B, 1);
                case 0xD0: return Op_SetRegisterBit(&Register.B, 2);
                case 0xD8: return Op_SetRegisterBit(&Register.B, 3);
                case 0xE0: return Op_SetRegisterBit(&Register.B, 4);
                case 0xE8: return Op_SetRegisterBit(&Register.B, 5);
                case 0xF0: return Op_SetRegisterBit(&Register.B, 6);
                case 0xF8: return Op_SetRegisterBit(&Register.B, 7);
                case 0xC1: return Op_SetRegisterBit(&Register.C, 0);
                case 0xC9: return Op_SetRegisterBit(&Register.C, 1);
                case 0xD1: return Op_SetRegisterBit(&Register.C, 2);
                case 0xD9: return Op_SetRegisterBit(&Register.C, 3);
                case 0xE1: return Op_SetRegisterBit(&Register.C, 4);
                case 0xE9: return Op_SetRegisterBit(&Register.C, 5);
                case 0xF1: return Op_SetRegisterBit(&Register.C, 6);
                case 0xF9: return Op_SetRegisterBit(&Register.C, 7);
                case 0xC2: return Op_SetRegisterBit(&Register.D, 0);
                case 0xCA: return Op_SetRegisterBit(&Register.D, 1);
                case 0xD2: return Op_SetRegisterBit(&Register.D, 2);
                case 0xDA: return Op_SetRegisterBit(&Register.D, 3);
                case 0xE2: return Op_SetRegisterBit(&Register.D, 4);
                case 0xEA: return Op_SetRegisterBit(&Register.D, 5);
                case 0xF2: return Op_SetRegisterBit(&Register.D, 6);
                case 0xFA: return Op_SetRegisterBit(&Register.D, 7);
                case 0xC3: return Op_SetRegisterBit(&Register.E, 0);
                case 0xCB: return Op_SetRegisterBit(&Register.E, 1);
                case 0xD3: return Op_SetRegisterBit(&Register.E, 2);
                case 0xDB: return Op_SetRegisterBit(&Register.E, 3);
                case 0xE3: return Op_SetRegisterBit(&Register.E, 4);
                case 0xEB: return Op_SetRegisterBit(&Register.E, 5);
                case 0xF3: return Op_SetRegisterBit(&Register.E, 6);
                case 0xFB: return Op_SetRegisterBit(&Register.E, 7);
                case 0xC4: return Op_SetRegisterBit(&Register.H, 0);
                case 0xCC: return Op_SetRegisterBit(&Register.H, 1);
                case 0xD4: return Op_SetRegisterBit(&Register.H, 2);
                case 0xDC: return Op_SetRegisterBit(&Register.H, 3);
                case 0xE4: return Op_SetRegisterBit(&Register.H, 4);
                case 0xEC: return Op_SetRegisterBit(&Register.H, 5);
                case 0xF4: return Op_SetRegisterBit(&Register.H, 6);
                case 0xFC: return Op_SetRegisterBit(&Register.H, 7);
                case 0xC5: return Op_SetRegisterBit(&Register.L, 0);
                case 0xCD: return Op_SetRegisterBit(&Register.L, 1);
                case 0xD5: return Op_SetRegisterBit(&Register.L, 2);
                case 0xDD: return Op_SetRegisterBit(&Register.L, 3);
                case 0xE5: return Op_SetRegisterBit(&Register.L, 4);
                case 0xED: return Op_SetRegisterBit(&Register.L, 5);
                case 0xF5: return Op_SetRegisterBit(&Register.L, 6);
                case 0xFD: return Op_SetRegisterBit(&Register.L, 7);
                case 0xC6: return Op_SetAddrBit(Register.HL, 0);
                case 0xCE: return Op_SetAddrBit(Register.HL, 1);
                case 0xD6: return Op_SetAddrBit(Register.HL, 2);
                case 0xDE: return Op_SetAddrBit(Register.HL, 3);
                case 0xE6: return Op_SetAddrBit(Register.HL, 4);
                case 0xEE: return Op_SetAddrBit(Register.HL, 5);
                case 0xF6: return Op_SetAddrBit(Register.HL, 6);
                case 0xFE: return Op_SetAddrBit(Register.HL, 7);

                //Test bit
                case 0x47: return Op_TestRegisterBit(&Register.A, 0);
                case 0x4F: return Op_TestRegisterBit(&Register.A, 1);
                case 0x57: return Op_TestRegisterBit(&Register.A, 2);
                case 0x5F: return Op_TestRegisterBit(&Register.A, 3);
                case 0x67: return Op_TestRegisterBit(&Register.A, 4);
                case 0x6F: return Op_TestRegisterBit(&Register.A, 5);
                case 0x77: return Op_TestRegisterBit(&Register.A, 6);
                case 0x7F: return Op_TestRegisterBit(&Register.A, 7);
                case 0x40: return Op_TestRegisterBit(&Register.B, 0);
                case 0x48: return Op_TestRegisterBit(&Register.B, 1);
                case 0x50: return Op_TestRegisterBit(&Register.B, 2);
                case 0x58: return Op_TestRegisterBit(&Register.B, 3);
                case 0x60: return Op_TestRegisterBit(&Register.B, 4);
                case 0x68: return Op_TestRegisterBit(&Register.B, 5);
                case 0x70: return Op_TestRegisterBit(&Register.B, 6);
                case 0x78: return Op_TestRegisterBit(&Register.B, 7);
                case 0x41: return Op_TestRegisterBit(&Register.C, 0);
                case 0x49: return Op_TestRegisterBit(&Register.C, 1);
                case 0x51: return Op_TestRegisterBit(&Register.C, 2);
                case 0x59: return Op_TestRegisterBit(&Register.C, 3);
                case 0x61: return Op_TestRegisterBit(&Register.C, 4);
                case 0x69: return Op_TestRegisterBit(&Register.C, 5);
                case 0x71: return Op_TestRegisterBit(&Register.C, 6);
                case 0x79: return Op_TestRegisterBit(&Register.C, 7);
                case 0x42: return Op_TestRegisterBit(&Register.D, 0);
                case 0x4A: return Op_TestRegisterBit(&Register.D, 1);
                case 0x52: return Op_TestRegisterBit(&Register.D, 2);
                case 0x5A: return Op_TestRegisterBit(&Register.D, 3);
                case 0x62: return Op_TestRegisterBit(&Register.D, 4);
                case 0x6A: return Op_TestRegisterBit(&Register.D, 5);
                case 0x72: return Op_TestRegisterBit(&Register.D, 6);
                case 0x7A: return Op_TestRegisterBit(&Register.D, 7);
                case 0x43: return Op_TestRegisterBit(&Register.E, 0);
                case 0x4B: return Op_TestRegisterBit(&Register.E, 1);
                case 0x53: return Op_TestRegisterBit(&Register.E, 2);
                case 0x5B: return Op_TestRegisterBit(&Register.E, 3);
                case 0x63: return Op_TestRegisterBit(&Register.E, 4);
                case 0x6B: return Op_TestRegisterBit(&Register.E, 5);
                case 0x73: return Op_TestRegisterBit(&Register.E, 6);
                case 0x7B: return Op_TestRegisterBit(&Register.E, 7);
                case 0x44: return Op_TestRegisterBit(&Register.H, 0);
                case 0x4C: return Op_TestRegisterBit(&Register.H, 1);
                case 0x54: return Op_TestRegisterBit(&Register.H, 2);
                case 0x5C: return Op_TestRegisterBit(&Register.H, 3);
                case 0x64: return Op_TestRegisterBit(&Register.H, 4);
                case 0x6C: return Op_TestRegisterBit(&Register.H, 5);
                case 0x74: return Op_TestRegisterBit(&Register.H, 6);
                case 0x7C: return Op_TestRegisterBit(&Register.H, 7);
                case 0x45: return Op_TestRegisterBit(&Register.L, 0);
                case 0x4D: return Op_TestRegisterBit(&Register.L, 1);
                case 0x55: return Op_TestRegisterBit(&Register.L, 2);
                case 0x5D: return Op_TestRegisterBit(&Register.L, 3);
                case 0x65: return Op_TestRegisterBit(&Register.L, 4);
                case 0x6D: return Op_TestRegisterBit(&Register.L, 5);
                case 0x75: return Op_TestRegisterBit(&Register.L, 6);
                case 0x7D: return Op_TestRegisterBit(&Register.L, 7);
                case 0x46: return Op_TestAddrBit(Register.HL, 0);
                case 0x4E: return Op_TestAddrBit(Register.HL, 1);
                case 0x56: return Op_TestAddrBit(Register.HL, 2);
                case 0x5E: return Op_TestAddrBit(Register.HL, 3);
                case 0x66: return Op_TestAddrBit(Register.HL, 4);
                case 0x6E: return Op_TestAddrBit(Register.HL, 5);
                case 0x76: return Op_TestAddrBit(Register.HL, 6);
                case 0x7E: return Op_TestAddrBit(Register.HL, 7);

                //Reset Bit
                case 0x87: return Op_ResetRegisterBit(&Register.A, 0);
                case 0x8F: return Op_ResetRegisterBit(&Register.A, 1);
                case 0x97: return Op_ResetRegisterBit(&Register.A, 2);
                case 0x9F: return Op_ResetRegisterBit(&Register.A, 3);
                case 0xA7: return Op_ResetRegisterBit(&Register.A, 4);
                case 0xAF: return Op_ResetRegisterBit(&Register.A, 5);
                case 0xB7: return Op_ResetRegisterBit(&Register.A, 6);
                case 0xBF: return Op_ResetRegisterBit(&Register.A, 7);
                case 0x80: return Op_ResetRegisterBit(&Register.B, 0);
                case 0x88: return Op_ResetRegisterBit(&Register.B, 1);
                case 0x90: return Op_ResetRegisterBit(&Register.B, 2);
                case 0x98: return Op_ResetRegisterBit(&Register.B, 3);
                case 0xA0: return Op_ResetRegisterBit(&Register.B, 4);
                case 0xA8: return Op_ResetRegisterBit(&Register.B, 5);
                case 0xB0: return Op_ResetRegisterBit(&Register.B, 6);
                case 0xB8: return Op_ResetRegisterBit(&Register.B, 7);
                case 0x81: return Op_ResetRegisterBit(&Register.C, 0);
                case 0x89: return Op_ResetRegisterBit(&Register.C, 1);
                case 0x91: return Op_ResetRegisterBit(&Register.C, 2);
                case 0x99: return Op_ResetRegisterBit(&Register.C, 3);
                case 0xA1: return Op_ResetRegisterBit(&Register.C, 4);
                case 0xA9: return Op_ResetRegisterBit(&Register.C, 5);
                case 0xB1: return Op_ResetRegisterBit(&Register.C, 6);
                case 0xB9: return Op_ResetRegisterBit(&Register.C, 7);
                case 0x82: return Op_ResetRegisterBit(&Register.D, 0);
                case 0x8A: return Op_ResetRegisterBit(&Register.D, 1);
                case 0x92: return Op_ResetRegisterBit(&Register.D, 2);
                case 0x9A: return Op_ResetRegisterBit(&Register.D, 3);
                case 0xA2: return Op_ResetRegisterBit(&Register.D, 4);
                case 0xAA: return Op_ResetRegisterBit(&Register.D, 5);
                case 0xB2: return Op_ResetRegisterBit(&Register.D, 6);
                case 0xBA: return Op_ResetRegisterBit(&Register.D, 7);
                case 0x83: return Op_ResetRegisterBit(&Register.E, 0);
                case 0x8B: return Op_ResetRegisterBit(&Register.E, 1);
                case 0x93: return Op_ResetRegisterBit(&Register.E, 2);
                case 0x9B: return Op_ResetRegisterBit(&Register.E, 3);
                case 0xA3: return Op_ResetRegisterBit(&Register.E, 4);
                case 0xAB: return Op_ResetRegisterBit(&Register.E, 5);
                case 0xB3: return Op_ResetRegisterBit(&Register.E, 6);
                case 0xBB: return Op_ResetRegisterBit(&Register.E, 7);
                case 0x84: return Op_ResetRegisterBit(&Register.H, 0);
                case 0x8C: return Op_ResetRegisterBit(&Register.H, 1);
                case 0x94: return Op_ResetRegisterBit(&Register.H, 2);
                case 0x9C: return Op_ResetRegisterBit(&Register.H, 3);
                case 0xA4: return Op_ResetRegisterBit(&Register.H, 4);
                case 0xAC: return Op_ResetRegisterBit(&Register.H, 5);
                case 0xB4: return Op_ResetRegisterBit(&Register.H, 6);
                case 0xBC: return Op_ResetRegisterBit(&Register.H, 7);
                case 0x85: return Op_ResetRegisterBit(&Register.L, 0);
                case 0x8D: return Op_ResetRegisterBit(&Register.L, 1);
                case 0x95: return Op_ResetRegisterBit(&Register.L, 2);
                case 0x9D: return Op_ResetRegisterBit(&Register.L, 3);
                case 0xA5: return Op_ResetRegisterBit(&Register.L, 4);
                case 0xAD: return Op_ResetRegisterBit(&Register.L, 5);
                case 0xB5: return Op_ResetRegisterBit(&Register.L, 6);
                case 0xBD: return Op_ResetRegisterBit(&Register.L, 7);
                case 0x86: return Op_ResetAddrBit(Register.HL, 0);
                case 0x8E: return Op_ResetAddrBit(Register.HL, 1);
                case 0x96: return Op_ResetAddrBit(Register.HL, 2);
                case 0x9E: return Op_ResetAddrBit(Register.HL, 3);
                case 0xA6: return Op_ResetAddrBit(Register.HL, 4);
                case 0xAE: return Op_ResetAddrBit(Register.HL, 5);
                case 0xB6: return Op_ResetAddrBit(Register.HL, 6);
                case 0xBE: return Op_ResetAddrBit(Register.HL, 7);

                //Rotate Left
                case 0x07: return Op_RotateRegisterLeftWithCarry(&Register.A);
                case 0x00: return Op_RotateRegisterLeftWithCarry(&Register.B);
                case 0x01: return Op_RotateRegisterLeftWithCarry(&Register.C);
                case 0x02: return Op_RotateRegisterLeftWithCarry(&Register.D);
                case 0x03: return Op_RotateRegisterLeftWithCarry(&Register.E);
                case 0x04: return Op_RotateRegisterLeftWithCarry(&Register.H);
                case 0x05: return Op_RotateRegisterLeftWithCarry(&Register.L);
                case 0x06: return Op_RotateAddrLeftWithCarry(Register.HL);

                case 0x17: return Op_RotateRegisterLeftThroughCarry(&Register.A);
                case 0x10: return Op_RotateRegisterLeftThroughCarry(&Register.B);
                case 0x11: return Op_RotateRegisterLeftThroughCarry(&Register.C);
                case 0x12: return Op_RotateRegisterLeftThroughCarry(&Register.D);
                case 0x13: return Op_RotateRegisterLeftThroughCarry(&Register.E);
                case 0x14: return Op_RotateRegisterLeftThroughCarry(&Register.H);
                case 0x15: return Op_RotateRegisterLeftThroughCarry(&Register.L);
                case 0x16: return Op_RotateAddrLeftThroughCarry(Register.HL);

                //Rotate Right
                case 0x0F: return Op_RotateRegisterRightWithCarry(&Register.A);
                case 0x08: return Op_RotateRegisterRightWithCarry(&Register.B);
                case 0x09: return Op_RotateRegisterRightWithCarry(&Register.C);
                case 0x0A: return Op_RotateRegisterRightWithCarry(&Register.D);
                case 0x0B: return Op_RotateRegisterRightWithCarry(&Register.E);
                case 0x0C: return Op_RotateRegisterRightWithCarry(&Register.H);
                case 0x0D: return Op_RotateRegisterRightWithCarry(&Register.L);
                case 0x0E: return Op_RotateAddrRightWithCarry(Register.HL);
                
                case 0x1F: return Op_RotateRegisterRightThroughCarry(&Register.A);
                case 0x18: return Op_RotateRegisterRightThroughCarry(&Register.B);
                case 0x19: return Op_RotateRegisterRightThroughCarry(&Register.C);
                case 0x1A: return Op_RotateRegisterRightThroughCarry(&Register.D);
                case 0x1B: return Op_RotateRegisterRightThroughCarry(&Register.E);
                case 0x1C: return Op_RotateRegisterRightThroughCarry(&Register.H);
                case 0x1D: return Op_RotateRegisterRightThroughCarry(&Register.L);
                case 0x1E: return Op_RotateAddrRightThroughCarry(Register.HL);

                //Shift Left
                case 0x27: return Op_ShiftRegisterLeft(&Register.A);
                case 0x20: return Op_ShiftRegisterLeft(&Register.B);
                case 0x21: return Op_ShiftRegisterLeft(&Register.C);
                case 0x22: return Op_ShiftRegisterLeft(&Register.D);
                case 0x23: return Op_ShiftRegisterLeft(&Register.E);
                case 0x24: return Op_ShiftRegisterLeft(&Register.H);
                case 0x25: return Op_ShiftRegisterLeft(&Register.L);
                case 0x26: return Op_ShiftAddrLeft(Register.HL);

                //Shift Right
                case 0x2F: return Op_ShiftRegisterRight(&Register.A, false);
                case 0x28: return Op_ShiftRegisterRight(&Register.B, false);
                case 0x29: return Op_ShiftRegisterRight(&Register.C, false);
                case 0x2A: return Op_ShiftRegisterRight(&Register.D, false);
                case 0x2B: return Op_ShiftRegisterRight(&Register.E, false);
                case 0x2C: return Op_ShiftRegisterRight(&Register.H, false);
                case 0x2D: return Op_ShiftRegisterRight(&Register.L, false);
                case 0x2E: return Op_ShiftAddrRight(Register.HL, false);

                case 0x3F: return Op_ShiftRegisterRight(&Register.A, true);
                case 0x38: return Op_ShiftRegisterRight(&Register.B, true);
                case 0x39: return Op_ShiftRegisterRight(&Register.C, true);
                case 0x3A: return Op_ShiftRegisterRight(&Register.D, true);
                case 0x3B: return Op_ShiftRegisterRight(&Register.E, true);
                case 0x3C: return Op_ShiftRegisterRight(&Register.H, true);
                case 0x3D: return Op_ShiftRegisterRight(&Register.L, true);
                case 0x3E: return Op_ShiftAddrRight(Register.HL, false);

                default:
                    DebugPrint("Unhandled extended opcode 0x%02X!\n", extOpCode);
                    assert(0);
            }
        }
        break;

        case 0x00: return Op_NOP();
        case 0x76: return Op_Halt();

        default:
            DebugPrint("Unhandled opcode 0x%02X!\n", opCode);
            assert(0);
    }

    return 0;
}

void CheckInterrupts()
{
    if (!IME || *Register_IF == 0)
        return;

    for (int i = 0; i < NumInterrupts; ++i)
    {
        if (IsRegisterBitSet(Register_IE, i) && IsRegisterBitSet(Register_IF, i))
        {
            IME = false;
            CPURunning = true;
            UnsetRegisterBit(Register_IF, i);
            StackPush(Register.PC);
            Register.PC = InterruptOp[i];
            break;
        }
    }
}

void CPUSetInterrupt(int interruptIdx)
{
    SetRegisterBit(Register_IF, interruptIdx);
}

bool CPUInit(uint16_t startAddr, byte interruptOps[], int numInterrupts)
{
    Register.PC = startAddr;

    CPURunning = true;

    NumInterrupts = numInterrupts;
    for (int i = 0; i < NumInterrupts; ++i)
    {
        InterruptOp[i] = interruptOps[i];
    }

    return true;
}

cycles CPUTick()
{
    CheckInterrupts();

    if (CPURunning)
    {
        return HandleOpCode();
    }

    return 0;
}

#if DEBUG_ENABLED

const struct CPURegisters* DebugGetCPURegisters()
{
    return &Register;
}

#endif
