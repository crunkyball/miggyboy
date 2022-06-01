#ifndef OPCODE_DEBUG_H
#define OPCODE_DEBUG_H

#include <assert.h>

#include "types.h"

#if DEBUG_ENABLED

static int CPUExtendedOpGetDebugInfo(byte* pOpPtr, const char** pOpStr)
{
    switch (*pOpPtr)
    {
        case 0x00: *pOpStr = "RLC B"; return 2;
        case 0x01: *pOpStr = "RLC C"; return 2;
        case 0x02: *pOpStr = "RLC D"; return 2;
        case 0x03: *pOpStr = "RLC E"; return 2;
        case 0x04: *pOpStr = "RLC H"; return 2;
        case 0x05: *pOpStr = "RLC L"; return 2;
        case 0x06: *pOpStr = "RLC (HL)"; return 2;
        case 0x07: *pOpStr = "RLC A"; return 2;
        case 0x08: *pOpStr = "RRC B"; return 2;
        case 0x09: *pOpStr = "RRC C"; return 2;
        case 0x0A: *pOpStr = "RRC D"; return 2;
        case 0x0B: *pOpStr = "RRC E"; return 2;
        case 0x0C: *pOpStr = "RRC H"; return 2;
        case 0x0D: *pOpStr = "RRC L"; return 2;
        case 0x0E: *pOpStr = "RRC (HL)"; return 2;
        case 0x0F: *pOpStr = "RRC A"; return 2;

        case 0x10: *pOpStr = "RL B"; return 2;
        case 0x11: *pOpStr = "RL C"; return 2;
        case 0x12: *pOpStr = "RL D"; return 2;
        case 0x13: *pOpStr = "RL E"; return 2;
        case 0x14: *pOpStr = "RL H"; return 2;
        case 0x15: *pOpStr = "RL L"; return 2;
        case 0x16: *pOpStr = "RL (HL)"; return 2;
        case 0x17: *pOpStr = "RL A"; return 2;
        case 0x18: *pOpStr = "RR B"; return 2;
        case 0x19: *pOpStr = "RR C"; return 2;
        case 0x1A: *pOpStr = "RR D"; return 2;
        case 0x1B: *pOpStr = "RR E"; return 2;
        case 0x1C: *pOpStr = "RR H"; return 2;
        case 0x1D: *pOpStr = "RR L"; return 2;
        case 0x1E: *pOpStr = "RR (HL)"; return 2;
        case 0x1F: *pOpStr = "RR A"; return 2;

        case 0x20: *pOpStr = "SLA B"; return 2;
        case 0x21: *pOpStr = "SLA C"; return 2;
        case 0x22: *pOpStr = "SLA D"; return 2;
        case 0x23: *pOpStr = "SLA E"; return 2;
        case 0x24: *pOpStr = "SLA H"; return 2;
        case 0x25: *pOpStr = "SLA L"; return 2;
        case 0x26: *pOpStr = "SLA (HL)"; return 2;
        case 0x27: *pOpStr = "SLA A"; return 2;
        case 0x28: *pOpStr = "SRA B"; return 2;
        case 0x29: *pOpStr = "SRA C"; return 2;
        case 0x2A: *pOpStr = "SRA D"; return 2;
        case 0x2B: *pOpStr = "SRA E"; return 2;
        case 0x2C: *pOpStr = "SRA H"; return 2;
        case 0x2D: *pOpStr = "SRA L"; return 2;
        case 0x2E: *pOpStr = "SRA (HL)"; return 2;
        case 0x2F: *pOpStr = "SRA A"; return 2;

        case 0x30: *pOpStr = "SWAP B"; return 2;
        case 0x31: *pOpStr = "SWAP C"; return 2;
        case 0x32: *pOpStr = "SWAP D"; return 2;
        case 0x33: *pOpStr = "SWAP E"; return 2;
        case 0x34: *pOpStr = "SWAP H"; return 2;
        case 0x35: *pOpStr = "SWAP L"; return 2;
        case 0x36: *pOpStr = "SWAP (HL)"; return 2;
        case 0x37: *pOpStr = "SWAP A"; return 2;
        case 0x38: *pOpStr = "SRL B"; return 2;
        case 0x39: *pOpStr = "SRL C"; return 2;
        case 0x3A: *pOpStr = "SRL D"; return 2;
        case 0x3B: *pOpStr = "SRL E"; return 2;
        case 0x3C: *pOpStr = "SRL H"; return 2;
        case 0x3D: *pOpStr = "SRL L"; return 2;
        case 0x3E: *pOpStr = "SRL (HL)"; return 2;
        case 0x3F: *pOpStr = "SRL A"; return 2;

        case 0x40: *pOpStr = "BIT 0,B"; return 2;
        case 0x41: *pOpStr = "BIT 0,C"; return 2;
        case 0x42: *pOpStr = "BIT 0,D"; return 2;
        case 0x43: *pOpStr = "BIT 0,E"; return 2;
        case 0x44: *pOpStr = "BIT 0,H"; return 2;
        case 0x45: *pOpStr = "BIT 0,L"; return 2;
        case 0x46: *pOpStr = "BIT 0,(HL)"; return 2;
        case 0x47: *pOpStr = "BIT 0,A"; return 2;
        case 0x48: *pOpStr = "BIT 1,B"; return 2;
        case 0x49: *pOpStr = "BIT 1,C"; return 2;
        case 0x4A: *pOpStr = "BIT 1,D"; return 2;
        case 0x4B: *pOpStr = "BIT 1,E"; return 2;
        case 0x4C: *pOpStr = "BIT 1,H"; return 2;
        case 0x4D: *pOpStr = "BIT 1,L"; return 2;
        case 0x4E: *pOpStr = "BIT 1,(HL)"; return 2;
        case 0x4F: *pOpStr = "BIT 1,A"; return 2;

        case 0x50: *pOpStr = "BIT 2,B"; return 2;
        case 0x51: *pOpStr = "BIT 2,C"; return 2;
        case 0x52: *pOpStr = "BIT 2,D"; return 2;
        case 0x53: *pOpStr = "BIT 2,E"; return 2;
        case 0x54: *pOpStr = "BIT 2,H"; return 2;
        case 0x55: *pOpStr = "BIT 2,L"; return 2;
        case 0x56: *pOpStr = "BIT 2,(HL)"; return 2;
        case 0x57: *pOpStr = "BIT 2,A"; return 2;
        case 0x58: *pOpStr = "BIT 3,B"; return 2;
        case 0x59: *pOpStr = "BIT 3,C"; return 2;
        case 0x5A: *pOpStr = "BIT 3,D"; return 2;
        case 0x5B: *pOpStr = "BIT 3,E"; return 2;
        case 0x5C: *pOpStr = "BIT 3,H"; return 2;
        case 0x5D: *pOpStr = "BIT 3,L"; return 2;
        case 0x5E: *pOpStr = "BIT 3,(HL)"; return 2;
        case 0x5F: *pOpStr = "BIT 3,A"; return 2;

        case 0x60: *pOpStr = "BIT 4,B"; return 2;
        case 0x61: *pOpStr = "BIT 4,C"; return 2;
        case 0x62: *pOpStr = "BIT 4,D"; return 2;
        case 0x63: *pOpStr = "BIT 4,E"; return 2;
        case 0x64: *pOpStr = "BIT 4,H"; return 2;
        case 0x65: *pOpStr = "BIT 4,L"; return 2;
        case 0x66: *pOpStr = "BIT 4,(HL)"; return 2;
        case 0x67: *pOpStr = "BIT 4,A"; return 2;
        case 0x68: *pOpStr = "BIT 5,B"; return 2;
        case 0x69: *pOpStr = "BIT 5,C"; return 2;
        case 0x6A: *pOpStr = "BIT 5,D"; return 2;
        case 0x6B: *pOpStr = "BIT 5,E"; return 2;
        case 0x6C: *pOpStr = "BIT 5,H"; return 2;
        case 0x6D: *pOpStr = "BIT 5,L"; return 2;
        case 0x6E: *pOpStr = "BIT 5,(HL)"; return 2;
        case 0x6F: *pOpStr = "BIT 5,A"; return 2;

        case 0x70: *pOpStr = "BIT 6,B"; return 2;
        case 0x71: *pOpStr = "BIT 6,C"; return 2;
        case 0x72: *pOpStr = "BIT 6,D"; return 2;
        case 0x73: *pOpStr = "BIT 6,E"; return 2;
        case 0x74: *pOpStr = "BIT 6,H"; return 2;
        case 0x75: *pOpStr = "BIT 6,L"; return 2;
        case 0x76: *pOpStr = "BIT 6,(HL)"; return 2;
        case 0x77: *pOpStr = "BIT 6,A"; return 2;
        case 0x78: *pOpStr = "BIT 7,B"; return 2;
        case 0x79: *pOpStr = "BIT 7,C"; return 2;
        case 0x7A: *pOpStr = "BIT 7,D"; return 2;
        case 0x7B: *pOpStr = "BIT 7,E"; return 2;
        case 0x7C: *pOpStr = "BIT 7,H"; return 2;
        case 0x7D: *pOpStr = "BIT 7,L"; return 2;
        case 0x7E: *pOpStr = "BIT 7,(HL)"; return 2;
        case 0x7F: *pOpStr = "BIT 7,A"; return 2;

        case 0x80: *pOpStr = "RES 0,B"; return 2;
        case 0x81: *pOpStr = "RES 0,C"; return 2;
        case 0x82: *pOpStr = "RES 0,D"; return 2;
        case 0x83: *pOpStr = "RES 0,E"; return 2;
        case 0x84: *pOpStr = "RES 0,H"; return 2;
        case 0x85: *pOpStr = "RES 0,L"; return 2;
        case 0x86: *pOpStr = "RES 0,(HL)"; return 2;
        case 0x87: *pOpStr = "RES 0,A"; return 2;
        case 0x88: *pOpStr = "RES 1,B"; return 2;
        case 0x89: *pOpStr = "RES 1,C"; return 2;
        case 0x8A: *pOpStr = "RES 1,D"; return 2;
        case 0x8B: *pOpStr = "RES 1,E"; return 2;
        case 0x8C: *pOpStr = "RES 1,H"; return 2;
        case 0x8D: *pOpStr = "RES 1,L"; return 2;
        case 0x8E: *pOpStr = "RES 1,(HL)"; return 2;
        case 0x8F: *pOpStr = "RES 1,A"; return 2;

        case 0x90: *pOpStr = "RES 2,B"; return 2;
        case 0x91: *pOpStr = "RES 2,C"; return 2;
        case 0x92: *pOpStr = "RES 2,D"; return 2;
        case 0x93: *pOpStr = "RES 2,E"; return 2;
        case 0x94: *pOpStr = "RES 2,H"; return 2;
        case 0x95: *pOpStr = "RES 2,L"; return 2;
        case 0x96: *pOpStr = "RES 2,(HL)"; return 2;
        case 0x97: *pOpStr = "RES 2,A"; return 2;
        case 0x98: *pOpStr = "RES 3,B"; return 2;
        case 0x99: *pOpStr = "RES 3,C"; return 2;
        case 0x9A: *pOpStr = "RES 3,D"; return 2;
        case 0x9B: *pOpStr = "RES 3,E"; return 2;
        case 0x9C: *pOpStr = "RES 3,H"; return 2;
        case 0x9D: *pOpStr = "RES 3,L"; return 2;
        case 0x9E: *pOpStr = "RES 3,(HL)"; return 2;
        case 0x9F: *pOpStr = "RES 3,A"; return 2;

        case 0xA0: *pOpStr = "RES 4,B"; return 2;
        case 0xA1: *pOpStr = "RES 4,C"; return 2;
        case 0xA2: *pOpStr = "RES 4,D"; return 2;
        case 0xA3: *pOpStr = "RES 4,E"; return 2;
        case 0xA4: *pOpStr = "RES 4,H"; return 2;
        case 0xA5: *pOpStr = "RES 4,L"; return 2;
        case 0xA6: *pOpStr = "RES 4,(HL)"; return 2;
        case 0xA7: *pOpStr = "RES 4,A"; return 2;
        case 0xA8: *pOpStr = "RES 5,B"; return 2;
        case 0xA9: *pOpStr = "RES 5,C"; return 2;
        case 0xAA: *pOpStr = "RES 5,D"; return 2;
        case 0xAB: *pOpStr = "RES 5,E"; return 2;
        case 0xAC: *pOpStr = "RES 5,H"; return 2;
        case 0xAD: *pOpStr = "RES 5,L"; return 2;
        case 0xAE: *pOpStr = "RES 5,(HL)"; return 2;
        case 0xAF: *pOpStr = "RES 5,A"; return 2;

        case 0xB0: *pOpStr = "RES 6,B"; return 2;
        case 0xB1: *pOpStr = "RES 6,C"; return 2;
        case 0xB2: *pOpStr = "RES 6,D"; return 2;
        case 0xB3: *pOpStr = "RES 6,E"; return 2;
        case 0xB4: *pOpStr = "RES 6,H"; return 2;
        case 0xB5: *pOpStr = "RES 6,L"; return 2;
        case 0xB6: *pOpStr = "RES 6,(HL)"; return 2;
        case 0xB7: *pOpStr = "RES 6,A"; return 2;
        case 0xB8: *pOpStr = "RES 7,B"; return 2;
        case 0xB9: *pOpStr = "RES 7,C"; return 2;
        case 0xBA: *pOpStr = "RES 7,D"; return 2;
        case 0xBB: *pOpStr = "RES 7,E"; return 2;
        case 0xBC: *pOpStr = "RES 7,H"; return 2;
        case 0xBD: *pOpStr = "RES 7,L"; return 2;
        case 0xBE: *pOpStr = "RES 7,(HL)"; return 2;
        case 0xBF: *pOpStr = "RES 7,A"; return 2;

        case 0xC0: *pOpStr = "SET 0,B"; return 2;
        case 0xC1: *pOpStr = "SET 0,C"; return 2;
        case 0xC2: *pOpStr = "SET 0,D"; return 2;
        case 0xC3: *pOpStr = "SET 0,E"; return 2;
        case 0xC4: *pOpStr = "SET 0,H"; return 2;
        case 0xC5: *pOpStr = "SET 0,L"; return 2;
        case 0xC6: *pOpStr = "SET 0,(HL)"; return 2;
        case 0xC7: *pOpStr = "SET 0,A"; return 2;
        case 0xC8: *pOpStr = "SET 1,B"; return 2;
        case 0xC9: *pOpStr = "SET 1,C"; return 2;
        case 0xCA: *pOpStr = "SET 1,D"; return 2;
        case 0xCB: *pOpStr = "SET 1,E"; return 2;
        case 0xCC: *pOpStr = "SET 1,H"; return 2;
        case 0xCD: *pOpStr = "SET 1,L"; return 2;
        case 0xCE: *pOpStr = "SET 1,(HL)"; return 2;
        case 0xCF: *pOpStr = "SET 1,A"; return 2;

        case 0xD0: *pOpStr = "SET 2,B"; return 2;
        case 0xD1: *pOpStr = "SET 2,C"; return 2;
        case 0xD2: *pOpStr = "SET 2,D"; return 2;
        case 0xD3: *pOpStr = "SET 2,E"; return 2;
        case 0xD4: *pOpStr = "SET 2,H"; return 2;
        case 0xD5: *pOpStr = "SET 2,L"; return 2;
        case 0xD6: *pOpStr = "SET 2,(HL)"; return 2;
        case 0xD7: *pOpStr = "SET 2,A"; return 2;
        case 0xD8: *pOpStr = "SET 3,B"; return 2;
        case 0xD9: *pOpStr = "SET 3,C"; return 2;
        case 0xDA: *pOpStr = "SET 3,D"; return 2;
        case 0xDB: *pOpStr = "SET 3,E"; return 2;
        case 0xDC: *pOpStr = "SET 3,H"; return 2;
        case 0xDD: *pOpStr = "SET 3,L"; return 2;
        case 0xDE: *pOpStr = "SET 3,(HL)"; return 2;
        case 0xDF: *pOpStr = "SET 3,A"; return 2;

        case 0xE0: *pOpStr = "SET 4,B"; return 2;
        case 0xE1: *pOpStr = "SET 4,C"; return 2;
        case 0xE2: *pOpStr = "SET 4,D"; return 2;
        case 0xE3: *pOpStr = "SET 4,E"; return 2;
        case 0xE4: *pOpStr = "SET 4,H"; return 2;
        case 0xE5: *pOpStr = "SET 4,L"; return 2;
        case 0xE6: *pOpStr = "SET 4,(HL)"; return 2;
        case 0xE7: *pOpStr = "SET 4,A"; return 2;
        case 0xE8: *pOpStr = "SET 5,B"; return 2;
        case 0xE9: *pOpStr = "SET 5,C"; return 2;
        case 0xEA: *pOpStr = "SET 5,D"; return 2;
        case 0xEB: *pOpStr = "SET 5,E"; return 2;
        case 0xEC: *pOpStr = "SET 5,H"; return 2;
        case 0xED: *pOpStr = "SET 5,L"; return 2;
        case 0xEE: *pOpStr = "SET 5,(HL)"; return 2;
        case 0xEF: *pOpStr = "SET 5,A"; return 2;

        case 0xF0: *pOpStr = "SET 6,B"; return 2;
        case 0xF1: *pOpStr = "SET 6,C"; return 2;
        case 0xF2: *pOpStr = "SET 6,D"; return 2;
        case 0xF3: *pOpStr = "SET 6,E"; return 2;
        case 0xF4: *pOpStr = "SET 6,H"; return 2;
        case 0xF5: *pOpStr = "SET 6,L"; return 2;
        case 0xF6: *pOpStr = "SET 6,(HL)"; return 2;
        case 0xF7: *pOpStr = "SET 6,A"; return 2;
        case 0xF8: *pOpStr = "SET 7,B"; return 2;
        case 0xF9: *pOpStr = "SET 7,C"; return 2;
        case 0xFA: *pOpStr = "SET 7,D"; return 2;
        case 0xFB: *pOpStr = "SET 7,E"; return 2;
        case 0xFC: *pOpStr = "SET 7,H"; return 2;
        case 0xFD: *pOpStr = "SET 7,L"; return 2;
        case 0xFE: *pOpStr = "SET 7,(HL)"; return 2;
        case 0xFF: *pOpStr = "SET 7,A"; return 2;
    }

    return 0;
}

static int CPUOpGetDebugInfo(byte* pOpPtr, const char** pOpStr)
{
    *pOpStr = NULL;

    switch (*pOpPtr)
    {
        case 0x00: *pOpStr = "NOP"; return 1;
        case 0x01: *pOpStr = "LD BC,d16"; return 3;
        case 0x02: *pOpStr = "LD (BC),A"; return 1;
        case 0x03: *pOpStr = "INC BC"; return 1;
        case 0x04: *pOpStr = "INC B"; return 1;
        case 0x05: *pOpStr = "DEC B"; return 1;
        case 0x06: *pOpStr = "LD B,d8"; return 2;
        case 0x07: *pOpStr = "RLCA"; return 1;
        case 0x08: *pOpStr = "LD (a16),SP"; return 3;
        case 0x09: *pOpStr = "ADD HL,BC"; return 1;
        case 0x0A: *pOpStr = "LD A,(BC)"; return 1;
        case 0x0B: *pOpStr = "DEC BC"; return 1;
        case 0x0C: *pOpStr = "INC C"; return 1;
        case 0x0D: *pOpStr = "DEC C"; return 1;
        case 0x0E: *pOpStr = "LD C,d8"; return 2;
        case 0x0F: *pOpStr = "RRCA"; return 1;

        case 0x10: *pOpStr = "STOP d8"; return 2;
        case 0x11: *pOpStr = "LD DE,d16"; return 3;
        case 0x12: *pOpStr = "LD (DE),A"; return 1;
        case 0x13: *pOpStr = "INC DE"; return 1;
        case 0x14: *pOpStr = "INC D"; return 1;
        case 0x15: *pOpStr = "DEC D"; return 1;
        case 0x16: *pOpStr = "LD D,d8"; return 2;
        case 0x17: *pOpStr = "RLA"; return 1;
        case 0x18: *pOpStr = "JR r8"; return 2;
        case 0x19: *pOpStr = "ADD HL,DE"; return 1;
        case 0x1A: *pOpStr = "LD A,(DE)"; return 1;
        case 0x1B: *pOpStr = "DEC DE"; return 1;
        case 0x1C: *pOpStr = "INC E"; return 1;
        case 0x1D: *pOpStr = "DEC E"; return 1;
        case 0x1E: *pOpStr = "LD E,d8"; return 2;
        case 0x1F: *pOpStr = "RRA"; return 1;

        case 0x20: *pOpStr = "JR NZ,r8"; return 2;
        case 0x21: *pOpStr = "LD HL,d16"; return 3;
        case 0x22: *pOpStr = "LD (HL+),A"; return 1;
        case 0x23: *pOpStr = "INC HL"; return 1;
        case 0x24: *pOpStr = "INC H"; return 1;
        case 0x25: *pOpStr = "DEC H"; return 1;
        case 0x26: *pOpStr = "LD H,d8"; return 2;
        case 0x27: *pOpStr = "DAA"; return 1;
        case 0x28: *pOpStr = "JR Z,r8"; return 2;
        case 0x29: *pOpStr = "ADD HL,HL"; return 1;
        case 0x2A: *pOpStr = "LD A,(HL+)"; return 1;
        case 0x2B: *pOpStr = "DEC HL"; return 1;
        case 0x2C: *pOpStr = "INC L"; return 1;
        case 0x2D: *pOpStr = "DEC L"; return 1;
        case 0x2E: *pOpStr = "LD L,d8"; return 2;
        case 0x2F: *pOpStr = "CPL"; return 1;

        case 0x30: *pOpStr = "JR NC,r8"; return 2;
        case 0x31: *pOpStr = "LD SP,d16"; return 3;
        case 0x32: *pOpStr = "LD (HL-),A"; return 1;
        case 0x33: *pOpStr = "INC SP"; return 1;
        case 0x34: *pOpStr = "INC (HL)"; return 1;
        case 0x35: *pOpStr = "DEC (HL)"; return 1;
        case 0x36: *pOpStr = "LD (HL),d8"; return 2;
        case 0x37: *pOpStr = "SCF"; return 1;
        case 0x38: *pOpStr = "JR C,r8"; return 2;
        case 0x39: *pOpStr = "ADD HL,SP"; return 1;
        case 0x3A: *pOpStr = "LD A,(HL-)"; return 1;
        case 0x3B: *pOpStr = "DEC SP"; return 1;
        case 0x3C: *pOpStr = "INC A"; return 1;
        case 0x3D: *pOpStr = "DEC A"; return 1;
        case 0x3E: *pOpStr = "LD A,d8"; return 2;
        case 0x3F: *pOpStr = "CCF"; return 1;

        case 0x40: *pOpStr = "LD B,B"; return 1;
        case 0x41: *pOpStr = "LD B,C"; return 1;
        case 0x42: *pOpStr = "LD B,D"; return 1;
        case 0x43: *pOpStr = "LD B,E"; return 1;
        case 0x44: *pOpStr = "LD B,H"; return 1;
        case 0x45: *pOpStr = "LD B,L"; return 1;
        case 0x46: *pOpStr = "LD B,(HL)"; return 1;
        case 0x47: *pOpStr = "LD B,A"; return 1;
        case 0x48: *pOpStr = "LD C,B"; return 1;
        case 0x49: *pOpStr = "LD C,C"; return 1;
        case 0x4A: *pOpStr = "LD C,D"; return 1;
        case 0x4B: *pOpStr = "LD C,E"; return 1;
        case 0x4C: *pOpStr = "LD C,H"; return 1;
        case 0x4D: *pOpStr = "LD C,L"; return 1;
        case 0x4E: *pOpStr = "LD C(HL)"; return 1;
        case 0x4F: *pOpStr = "LD C,A"; return 1;

        case 0x50: *pOpStr = "LD D,B"; return 1;
        case 0x51: *pOpStr = "LD D,C"; return 1;
        case 0x52: *pOpStr = "LD D,D"; return 1;
        case 0x53: *pOpStr = "LD D,E"; return 1;
        case 0x54: *pOpStr = "LD D,H"; return 1;
        case 0x55: *pOpStr = "LD D,L"; return 1;
        case 0x56: *pOpStr = "LD D,(HL)"; return 1;
        case 0x57: *pOpStr = "LD D,A"; return 1;
        case 0x58: *pOpStr = "LD E,B"; return 1;
        case 0x59: *pOpStr = "LD E,C"; return 1;
        case 0x5A: *pOpStr = "LD E,D"; return 1;
        case 0x5B: *pOpStr = "LD E,E"; return 1;
        case 0x5C: *pOpStr = "LD E,H"; return 1;
        case 0x5D: *pOpStr = "LD E,L"; return 1;
        case 0x5E: *pOpStr = "LD E(HL)"; return 1;
        case 0x5F: *pOpStr = "LD E,A"; return 1;

        case 0x60: *pOpStr = "LD H,B"; return 1;
        case 0x61: *pOpStr = "LD H,C"; return 1;
        case 0x62: *pOpStr = "LD H,D"; return 1;
        case 0x63: *pOpStr = "LD H,E"; return 1;
        case 0x64: *pOpStr = "LD H,H"; return 1;
        case 0x65: *pOpStr = "LD H,L"; return 1;
        case 0x66: *pOpStr = "LD H,(HL)"; return 1;
        case 0x67: *pOpStr = "LD H,A"; return 1;
        case 0x68: *pOpStr = "LD L,B"; return 1;
        case 0x69: *pOpStr = "LD L,C"; return 1;
        case 0x6A: *pOpStr = "LD L,D"; return 1;
        case 0x6B: *pOpStr = "LD L,E"; return 1;
        case 0x6C: *pOpStr = "LD L,H"; return 1;
        case 0x6D: *pOpStr = "LD L,L"; return 1;
        case 0x6E: *pOpStr = "LD L(HL)"; return 1;
        case 0x6F: *pOpStr = "LD L,A"; return 1;

        case 0x70: *pOpStr = "LD (HL),B"; return 1;
        case 0x71: *pOpStr = "LD (HL),C"; return 1;
        case 0x72: *pOpStr = "LD (HL),D"; return 1;
        case 0x73: *pOpStr = "LD (HL),E"; return 1;
        case 0x74: *pOpStr = "LD (HL),H"; return 1;
        case 0x75: *pOpStr = "LD (HL),L"; return 1;
        case 0x76: *pOpStr = "HALT"; return 1;
        case 0x77: *pOpStr = "LD (HL),A"; return 1;
        case 0x78: *pOpStr = "LD A,B"; return 1;
        case 0x79: *pOpStr = "LD A,C"; return 1;
        case 0x7A: *pOpStr = "LD A,D"; return 1;
        case 0x7B: *pOpStr = "LD A,E"; return 1;
        case 0x7C: *pOpStr = "LD A,H"; return 1;
        case 0x7D: *pOpStr = "LD A,L"; return 1;
        case 0x7E: *pOpStr = "LD A(HL)"; return 1;
        case 0x7F: *pOpStr = "LD A,A"; return 1;

        case 0x80: *pOpStr = "ADD A,B"; return 1;
        case 0x81: *pOpStr = "ADD A,C"; return 1;
        case 0x82: *pOpStr = "ADD A,D"; return 1;
        case 0x83: *pOpStr = "ADD A,E"; return 1;
        case 0x84: *pOpStr = "ADD A,H"; return 1;
        case 0x85: *pOpStr = "ADD A,L"; return 1;
        case 0x86: *pOpStr = "ADD A,(HL)"; return 1;
        case 0x87: *pOpStr = "ADA A,A"; return 1;
        case 0x88: *pOpStr = "ADC A,B"; return 1;
        case 0x89: *pOpStr = "ADC A,C"; return 1;
        case 0x8A: *pOpStr = "ADC A,D"; return 1;
        case 0x8B: *pOpStr = "ADC A,E"; return 1;
        case 0x8C: *pOpStr = "ADC A,H"; return 1;
        case 0x8D: *pOpStr = "ADC A,L"; return 1;
        case 0x8E: *pOpStr = "ADC A,(HL)"; return 1;
        case 0x8F: *pOpStr = "ADC A,A"; return 1;

        case 0x90: *pOpStr = "SUB A,B"; return 1;
        case 0x91: *pOpStr = "SUB A,C"; return 1;
        case 0x92: *pOpStr = "SUB A,D"; return 1;
        case 0x93: *pOpStr = "SUB A,E"; return 1;
        case 0x94: *pOpStr = "SUB A,H"; return 1;
        case 0x95: *pOpStr = "SUB A,L"; return 1;
        case 0x96: *pOpStr = "SUB A,(HL)"; return 1;
        case 0x97: *pOpStr = "SBC A,A"; return 1;
        case 0x98: *pOpStr = "SBC A,B"; return 1;
        case 0x99: *pOpStr = "SBC A,C"; return 1;
        case 0x9A: *pOpStr = "SBC A,D"; return 1;
        case 0x9B: *pOpStr = "SBC A,E"; return 1;
        case 0x9C: *pOpStr = "SBC A,H"; return 1;
        case 0x9D: *pOpStr = "SBC A,L"; return 1;
        case 0x9E: *pOpStr = "SBC A,(HL)"; return 1;
        case 0x9F: *pOpStr = "SBC A,A"; return 1;

        case 0xA0: *pOpStr = "AND B"; return 1;
        case 0xA1: *pOpStr = "AND C"; return 1;
        case 0xA2: *pOpStr = "AND D"; return 1;
        case 0xA3: *pOpStr = "AND E"; return 1;
        case 0xA4: *pOpStr = "AND H"; return 1;
        case 0xA5: *pOpStr = "AND L"; return 1;
        case 0xA6: *pOpStr = "AND (HL)"; return 1;
        case 0xA7: *pOpStr = "AND A"; return 1;
        case 0xA8: *pOpStr = "XOR B"; return 1;
        case 0xA9: *pOpStr = "XOR C"; return 1;
        case 0xAA: *pOpStr = "XOR D"; return 1;
        case 0xAB: *pOpStr = "XOR E"; return 1;
        case 0xAC: *pOpStr = "XOR H"; return 1;
        case 0xAD: *pOpStr = "XOR L"; return 1;
        case 0xAE: *pOpStr = "XOR (HL)"; return 1;
        case 0xAF: *pOpStr = "XOR A"; return 1;

        case 0xB0: *pOpStr = "OR B"; return 1;
        case 0xB1: *pOpStr = "OR C"; return 1;
        case 0xB2: *pOpStr = "OR D"; return 1;
        case 0xB3: *pOpStr = "OR E"; return 1;
        case 0xB4: *pOpStr = "OR H"; return 1;
        case 0xB5: *pOpStr = "OR L"; return 1;
        case 0xB6: *pOpStr = "OR (HL)"; return 1;
        case 0xB7: *pOpStr = "OR A"; return 1;
        case 0xB8: *pOpStr = "CP B"; return 1;
        case 0xB9: *pOpStr = "CP C"; return 1;
        case 0xBA: *pOpStr = "CP D"; return 1;
        case 0xBB: *pOpStr = "CP E"; return 1;
        case 0xBC: *pOpStr = "CP H"; return 1;
        case 0xBD: *pOpStr = "CP L"; return 1;
        case 0xBE: *pOpStr = "CP (HL)"; return 1;
        case 0xBF: *pOpStr = "CP A"; return 1;

        case 0xC0: *pOpStr = "RET NZ"; return 1;
        case 0xC1: *pOpStr = "POP BC"; return 1;
        case 0xC2: *pOpStr = "JP NZ,a16"; return 3;
        case 0xC3: *pOpStr = "JP a16"; return 3;
        case 0xC4: *pOpStr = "CALL NZ,a16"; return 3;
        case 0xC5: *pOpStr = "PUSH BC"; return 1;
        case 0xC6: *pOpStr = "ADD A,d8"; return 2;
        case 0xC7: *pOpStr = "RST 00H"; return 1;
        case 0xC8: *pOpStr = "RET Z"; return 1;
        case 0xC9: *pOpStr = "RET"; return 1;
        case 0xCA: *pOpStr = "JP Z,a16"; return 3;

        case 0xCB: return CPUExtendedOpGetDebugInfo(pOpPtr+1, pOpStr);

        case 0xCC: *pOpStr = "CALL Z,a16"; return 3;
        case 0xCD: *pOpStr = "CALL a16"; return 3;
        case 0xCE: *pOpStr = "ADC A,d8"; return 2;
        case 0xCF: *pOpStr = "RST 08H"; return 1;

        case 0xD0: *pOpStr = "RET NC"; return 1;
        case 0xD1: *pOpStr = "POP DE"; return 1;
        case 0xD2: *pOpStr = "JP NC,a16"; return 3;
        //case 0xD3: *pOpStr = ""; return 0;
        case 0xD4: *pOpStr = "CALL NC,a16"; return 3;
        case 0xD5: *pOpStr = "PUSH DE"; return 1;
        case 0xD6: *pOpStr = "SUB d8"; return 2;
        case 0xD7: *pOpStr = "RST 10H"; return 1;
        case 0xD8: *pOpStr = "RET C"; return 1;
        case 0xD9: *pOpStr = "RETI"; return 1;
        case 0xDA: *pOpStr = "JP C,a16"; return 3;
        //case 0xDB: *pOpStr = ""; return 0;
        case 0xDC: *pOpStr = "CALL C,a16"; return 3;
        //case 0xDD: *pOpStr = ""; return 0;
        case 0xDE: *pOpStr = "SBC A,d8"; return 2;
        case 0xDF: *pOpStr = "RST 18H"; return 1;

        case 0xE0: *pOpStr = "LDH (a8),A"; return 2;
        case 0xE1: *pOpStr = "POP HL"; return 1;
        case 0xE2: *pOpStr = "LD (C),A"; return 1;
        //case 0xE3: *pOpStr = ""; return 0;
        //case 0xE4: *pOpStr = ""; return 0;
        case 0xE5: *pOpStr = "PUSH HL"; return 1;
        case 0xE6: *pOpStr = "AND d8"; return 2;
        case 0xE7: *pOpStr = "RST 20H"; return 1;
        case 0xE8: *pOpStr = "ADD SP,r8"; return 2;
        case 0xE9: *pOpStr = "JP HL"; return 1;
        case 0xEA: *pOpStr = "LD (a16),A"; return 3;
        //case 0xEB: *pOpStr = ""; return 0;
        //case 0xEC: *pOpStr = ""; return 0;
        //case 0xED: *pOpStr = ""; return 0;
        case 0xEE: *pOpStr = "XOR d8"; return 2;
        case 0xEF: *pOpStr = "RST 28H"; return 1;

        case 0xF0: *pOpStr = "LDH A,(a8)"; return 2;
        case 0xF1: *pOpStr = "POP AF"; return 1;
        case 0xF2: *pOpStr = "LD A,(C)"; return 1;
        case 0xF3: *pOpStr = "DI"; return 1;
        //case 0xF4: *pOpStr = ""; return 0;
        case 0xF5: *pOpStr = "PUSH AF"; return 1;
        case 0xF6: *pOpStr = "OR d8"; return 2;
        case 0xF7: *pOpStr = "RST 30H"; return 1;
        case 0xF8: *pOpStr = "LD HL,SP+r8"; return 2;
        case 0xF9: *pOpStr = "LD SP,HL"; return 1;
        case 0xFA: *pOpStr = "LD A,(a16)"; return 3;
        case 0xFB: *pOpStr = "EI"; return 1;
        //case 0xFC: *pOpStr = ""; return 0;
        //case 0xFD: *pOpStr = ""; return 0;
        case 0xFE: *pOpStr = "CP d8"; return 2;
        case 0xFF: *pOpStr = "RST 38H"; return 1;
    }

    *pOpStr = "???";    //Most likely a data block.
    return 1;
}

#endif

#endif
