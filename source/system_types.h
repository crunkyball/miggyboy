#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

enum Interrupt
{
    Interrupt_VBlank = 0,
    Interrupt_STAT = 1,
    Interrupt_Timer = 2,
    //Interrupt_Serial = 3,
    Interrupt_Joypad = 4
};

enum InterruptOp
{
    InterruptOp_VBlank = 0x40,
    InterruptOp_STAT = 0x48,
    InterruptOp_Timer = 0x50,
    //InterruptOp_Serial = 0x58,
    InterruptOp_Joypad = 0x60
};

enum DirectionInput
{
    Input_Right = 1 << 0,
    Input_Left = 1 << 1,
    Input_Up = 1 << 2,
    Input_Down = 1 << 3
};

enum ButtonInput
{
    Input_A = 1 << 0,
    Input_B = 1 << 1,
    Input_Select = 1 << 2,
    Input_Start = 1 << 3
};

typedef void(*DirectionInputCallbackFunc)(enum DirectionInput key, bool pressed);
typedef void(*ButtonInputCallbackFunc)(enum ButtonInput key, bool pressed);

#endif
