// asm6502.c - copyright 1998-2007 Bruce Tomlin

#define versionName "6502 assembler"
#include "asmx.h"

enum
{
    o_Implied,          // implied instructions
    o_Implied_65C02,    // implied instructions for 65C02/65C816
    o_Implied_6502U,    // implied instructions for undocumented 6502 only
    o_Branch,           // branch instructions
    o_Branch_65C02,     // branch instructions for 65C02/65C816
    o_Mode,             // instructions with multiple addressing modes
    o_Mode_65C02,       // o_Mode for 6502
    o_Mode_6502U,       // o_Mode for undocumented 6502
    o_RSMB,             // RMBn/SMBn instructions for 65C02 only
    o_BBRS,             // BBRn/BBSn instructions for 65C02 only
    // 65C816 instructions
    o_Implied_65C816,   // implied instructions for 65C816 only
    o_Mode_65C816,      // o_Mode for 65C816
    o_BranchW,          // 16-bit branch for 65C816
    o_BlockMove,        // MVN,MVP for 65C816
    o_COP,              // COP for 65C816

//  o_Foo = o_LabelOp,
};

enum cputype
{
    CPU_6502, CPU_65C02, CPU_6502U, CPU_65C816
};

enum addrMode
{
    a_None = -1,// -1 invalid addressing mode
    a_Imm,      //  0 Immediate        #byte
    a_Abs,      //  1 Absolute         word
    a_Zpg,      //  2 Z-Page           byte
    a_Acc,      //  3 Accumulator      A (or no operand)
    a_Inx,      //  4 Indirect X       (byte,X)
    a_Iny,      //  5 Indirect Y       (byte),Y
    a_Zpx,      //  6 Z-Page X         byte,X
    a_Abx,      //  7 Absolute X       word,X
    a_Aby,      //  8 Absolute Y       word,Y
    a_Ind,      //  9 Indirect         (word)
    a_Zpy,      // 10 Z-Page Y         byte,Y
    a_Zpi,      // 11 Z-Page Indirect  (byte) 65C02/65C816 only
    // 65C816 modes
    a_AbL,      // 12 Absolute Long        al
    a_ALX,      // 13 Absolute Long X      al,x
    a_DIL,      // 14 Direct Indirect Long [d]
    a_DIY,      // 15 Direct Indirect Y    [d],Y
    a_Stk,      // 16 Stack Relative       d,S
    a_SIY,      // 17 Stack Indirect Y     (d,S),Y
    a_Max       // 18
};

enum addrOps
{
    o_ORA,      //  0
    o_ASL,      //  1
    o_JSR,      //  2
    o_AND,      //  3
    o_BIT,      //  4
    o_ROL,      //  5
    o_EOR,      //  6
    o_LSR,      //  7
    o_JMP,      //  8
    o_ADC,      //  9
    o_ROR,      // 10
    o_STA,      // 11
    o_STY,      // 12
    o_STX,      // 13
    o_LDY,      // 14
    o_LDA,      // 15
    o_LDX,      // 16
    o_CPY,      // 17
    o_CMP,      // 18
    o_DEC,      // 19
    o_CPX,      // 20
    o_SBC,      // 21
    o_INC,      // 22

    o_Extra,    // 23

//  o_Mode_6502U

    o_LAX = o_Extra, // 23
    o_DCP,
    o_ISB,
    o_RLA,
    o_RRA,
    o_SAX,
    o_SLO,
    o_SRE,
    o_ANC,
    o_ARR,
    o_ASR,
    o_SBX,

//  o_Mode_65C02

    o_STZ = o_Extra, // 23
    o_TSB,      // 24
    o_TRB,      // 25

//  o_Mode_65C816

    o_JSL,      // 26
    o_JML,      // 27
    o_REP,      // 28
    o_SEP,      // 29
    o_PEI,      // 30
    o_PEA,      // 31

    o_Max       // 32
    
};


const u_char mode2op6502[] = // [o_Max * a_Max] =
{//Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy=10 Zpi=11
    0x09, 0x0D, 0x05,    0, 0x01, 0x11, 0x15, 0x1D, 0x19,    0,    0,    0, 0,0,0,0,0,0, //  0 ORA
       0, 0x0E, 0x06, 0x0A,    0,    0, 0x16, 0x1E,    0,    0,    0,    0, 0,0,0,0,0,0, //  1 ASL
       0, 0x20,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, //  2 JSR
    0x29, 0x2D, 0x25,    0, 0x21, 0x31, 0x35, 0x3D, 0x39,    0,    0,    0, 0,0,0,0,0,0, //  3 AND
       0, 0x2C, 0x24,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, //  4 BIT
       0, 0x2E, 0x26, 0x2A,    0,    0, 0x36, 0x3E,    0,    0,    0,    0, 0,0,0,0,0,0, //  5 ROL
    0x49, 0x4D, 0x45,    0, 0x41, 0x51, 0x55, 0x5D, 0x59,    0,    0,    0, 0,0,0,0,0,0, //  6 EOR
       0, 0x4E, 0x46, 0x4A,    0,    0, 0x56, 0x5E,    0,    0,    0,    0, 0,0,0,0,0,0, //  7 LSR
       0, 0x4C,    0,    0,    0,    0,    0,    0,    0, 0x6C,    0,    0, 0,0,0,0,0,0, //  8 JMP
    0x69, 0x6D, 0x65,    0, 0x61, 0x71, 0x75, 0x7D, 0x79,    0,    0,    0, 0,0,0,0,0,0, //  9 ADC
       0, 0x6E, 0x66, 0x6A,    0,    0, 0x76, 0x7E,    0,    0,    0,    0, 0,0,0,0,0,0, // 10 ROR
       0, 0x8D, 0x85,    0, 0x81, 0x91, 0x95, 0x9D, 0x99,    0,    0,    0, 0,0,0,0,0,0, // 11 STA
       0, 0x8C, 0x84,    0,    0,    0, 0x94,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 12 STY
       0, 0x8E, 0x86,    0,    0,    0,    0,    0,    0,    0, 0x96,    0, 0,0,0,0,0,0, // 13 STX
    0xA0, 0xAC, 0xA4,    0,    0,    0, 0xB4, 0xBC,    0,    0,    0,    0, 0,0,0,0,0,0, // 14 LDY
    0xA9, 0xAD, 0xA5,    0, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9,    0,    0,    0, 0,0,0,0,0,0, // 15 LDA
    0xA2, 0xAE, 0xA6,    0,    0,    0,    0,    0, 0xBE,    0, 0xB6,    0, 0,0,0,0,0,0, // 16 LDX
    0xC0, 0xCC, 0xC4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 17 CPY
    0xC9, 0xCD, 0xC5,    0, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9,    0,    0,    0, 0,0,0,0,0,0, // 18 CMP
       0, 0xCE, 0xC6,    0,    0,    0, 0xD6, 0xDE,    0,    0,    0,    0, 0,0,0,0,0,0, // 19 DEC
    0xE0, 0xEC, 0xE4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 20 CPX
    0xE9, 0xED, 0xE5,    0, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9,    0,    0,    0, 0,0,0,0,0,0, // 21 SBC
       0, 0xEE, 0xE6,    0,    0,    0, 0xF6, 0xFE,    0,    0,    0,    0, 0,0,0,0,0,0, // 22 INC
// undocumented 6502 instructions start here
// Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy=10 Zpi=11
       0, 0xAF, 0xA7,    0, 0xA3, 0xB3,    0,    0, 0xBF,    0, 0xB7,    0, 0,0,0,0,0,0, // 23 LAX
       0, 0xCF, 0xC7,    0, 0xC3, 0xD3, 0xD7, 0xDF, 0xDB,    0,    0,    0, 0,0,0,0,0,0, // DCP
       0, 0xEF, 0xE7,    0, 0xE3, 0xF3, 0xF7, 0xFF, 0xFB,    0,    0,    0, 0,0,0,0,0,0, // ISB
       0, 0x2F, 0x27,    0, 0x23, 0x33, 0x37, 0x3F, 0x3B,    0,    0,    0, 0,0,0,0,0,0, // RLA
       0, 0x6F, 0x67,    0, 0x63, 0x73, 0x77, 0x7F, 0x7B,    0,    0,    0, 0,0,0,0,0,0, // RRA
       0, 0x8F, 0x87,    0, 0x83,    0,    0,    0,    0,    0, 0x97,    0, 0,0,0,0,0,0, // SAX
       0, 0x0F, 0x07,    0, 0x03, 0x13, 0x17, 0x1F, 0x1B,    0,    0,    0, 0,0,0,0,0,0, // SLO
       0, 0x4F, 0x47,    0, 0x43, 0x53, 0x57, 0x5F, 0x5B,    0,    0,    0, 0,0,0,0,0,0, // SRE
    0x2B,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // ANC
    0x6B,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // ARR
    0x4B,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // ASR
    0xCB,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0  // SBX
};


const u_char mode2op65C02[] = // [o_Max * a_Max] =
{//Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy=10 Zpi=11
    0x09, 0x0D, 0x05,    0, 0x01, 0x11, 0x15, 0x1D, 0x19,    0,    0, 0x12, 0,0,0,0,0,0, //  0 ORA
       0, 0x0E, 0x06, 0x0A,    0,    0, 0x16, 0x1E,    0,    0,    0,    0, 0,0,0,0,0,0, //  1 ASL
       0, 0x20,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, //  2 JSR
    0x29, 0x2D, 0x25,    0, 0x21, 0x31, 0x35, 0x3D, 0x39,    0,    0, 0x32, 0,0,0,0,0,0, //  3 AND
    0x89, 0x2C, 0x24,    0,    0,    0, 0x34, 0x3C,    0,    0,    0,    0, 0,0,0,0,0,0, //  4 BIT
       0, 0x2E, 0x26, 0x2A,    0,    0, 0x36, 0x3E,    0,    0,    0,    0, 0,0,0,0,0,0, //  5 ROL
    0x49, 0x4D, 0x45,    0, 0x41, 0x51, 0x55, 0x5D, 0x59,    0,    0, 0x52, 0,0,0,0,0,0, //  6 EOR
       0, 0x4E, 0x46, 0x4A,    0,    0, 0x56, 0x5E,    0,    0,    0,    0, 0,0,0,0,0,0, //  7 LSR
       0, 0x4C,    0,    0, 0x7C,    0,    0,    0,    0, 0x6C,    0,    0, 0,0,0,0,0,0, //  8 JMP note: 7C is (abs,x)
    0x69, 0x6D, 0x65,    0, 0x61, 0x71, 0x75, 0x7D, 0x79,    0,    0, 0x72, 0,0,0,0,0,0, //  9 ADC
       0, 0x6E, 0x66, 0x6A,    0,    0, 0x76, 0x7E,    0,    0,    0,    0, 0,0,0,0,0,0, // 10 ROR
       0, 0x8D, 0x85,    0, 0x81, 0x91, 0x95, 0x9D, 0x99,    0,    0, 0x92, 0,0,0,0,0,0, // 11 STA
       0, 0x8C, 0x84,    0,    0,    0, 0x94,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 12 STY
       0, 0x8E, 0x86,    0,    0,    0,    0,    0,    0,    0, 0x96,    0, 0,0,0,0,0,0, // 13 STX
    0xA0, 0xAC, 0xA4,    0,    0,    0, 0xB4, 0xBC,    0,    0,    0,    0, 0,0,0,0,0,0, // 14 LDY
    0xA9, 0xAD, 0xA5,    0, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9,    0,    0, 0xB2, 0,0,0,0,0,0, // 15 LDA
    0xA2, 0xAE, 0xA6,    0,    0,    0,    0,    0, 0xBE,    0, 0xB6,    0, 0,0,0,0,0,0, // 16 LDX
    0xC0, 0xCC, 0xC4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 17 CPY
    0xC9, 0xCD, 0xC5,    0, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9,    0,    0, 0xD2, 0,0,0,0,0,0, // 18 CMP
       0, 0xCE, 0xC6, 0x3A,    0,    0, 0xD6, 0xDE,    0,    0,    0,    0, 0,0,0,0,0,0, // 19 DEC
    0xE0, 0xEC, 0xE4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 20 CPX
    0xE9, 0xED, 0xE5,    0, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9,    0,    0, 0xF2, 0,0,0,0,0,0, // 21 SBC
       0, 0xEE, 0xE6, 0x1A,    0,    0, 0xF6, 0xFE,    0,    0,    0,    0, 0,0,0,0,0,0, // 22 INC
// 65C02-only instructions start here
       0, 0x9C, 0x64,    0,    0,    0, 0x74, 0x9E,    0,    0,    0,    0, 0,0,0,0,0,0, // 23 STZ
       0, 0x0C, 0x04,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 24 TSB
       0, 0x1C, 0x14,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0  // 25 TRB
};


const u_char mode2op65C816[] = // [o_Max * a_Max] =
{//Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy10 Zpi11 AbL12 ALX13 DIL14 DIY15 Stk16 SIY17
    0x09, 0x0D, 0x05,    0, 0x01, 0x11, 0x15, 0x1D, 0x19,    0,    0, 0x12, 0x0F, 0x1F, 0x07, 0x17, 0x03, 0x13, //  0 ORA
       0, 0x0E, 0x06, 0x0A,    0,    0, 0x16, 0x1E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  1 ASL
       0, 0x20,    0,    0, 0xFC,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  2 JSR note: FC is (abs,X)
    0x29, 0x2D, 0x25,    0, 0x21, 0x31, 0x35, 0x3D, 0x39,    0,    0, 0x32, 0x2F, 0x3F, 0x27, 0x37, 0x23, 0x33, //  3 AND
    0x89, 0x2C, 0x24,    0,    0,    0, 0x34, 0x3C,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  4 BIT
       0, 0x2E, 0x26, 0x2A,    0,    0, 0x36, 0x3E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  5 ROL
    0x49, 0x4D, 0x45,    0, 0x41, 0x51, 0x55, 0x5D, 0x59,    0,    0, 0x52, 0x4F, 0x5F, 0x47, 0x57, 0x43, 0x53, //  6 EOR
       0, 0x4E, 0x46, 0x4A,    0,    0, 0x56, 0x5E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  7 LSR
       0, 0x4C,    0,    0, 0x7C,    0,    0,    0,    0, 0x6C,    0,    0, 0x5C,    0,    0,    0,    0,    0, //  8 JMP note: 7C is (abs,x)
    0x69, 0x6D, 0x65,    0, 0x61, 0x71, 0x75, 0x7D, 0x79,    0,    0, 0x72, 0x6F, 0x7F, 0x67, 0x77, 0x63, 0x73, //  9 ADC
       0, 0x6E, 0x66, 0x6A,    0,    0, 0x76, 0x7E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 10 ROR
       0, 0x8D, 0x85,    0, 0x81, 0x91, 0x95, 0x9D, 0x99,    0,    0, 0x92, 0x8F, 0x9F, 0x87, 0x97, 0x83, 0x93, // 11 STA
       0, 0x8C, 0x84,    0,    0,    0, 0x94,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 12 STY
       0, 0x8E, 0x86,    0,    0,    0,    0,    0,    0,    0, 0x96,    0,    0,    0,    0,    0,    0,    0, // 13 STX
    0xA0, 0xAC, 0xA4,    0,    0,    0, 0xB4, 0xBC,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 14 LDY
    0xA9, 0xAD, 0xA5,    0, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9,    0,    0, 0xB2, 0xAF, 0xBF, 0xA7, 0xB7, 0xA3, 0xB3, // 15 LDA
    0xA2, 0xAE, 0xA6,    0,    0,    0,    0,    0, 0xBE,    0, 0xB6,    0,    0,    0,    0,    0,    0,    0, // 16 LDX
    0xC0, 0xCC, 0xC4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 17 CPY
    0xC9, 0xCD, 0xC5,    0, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9,    0,    0, 0xD2, 0xCF, 0xDF, 0xC7, 0xD7, 0xC3, 0xD3, // 18 CMP
       0, 0xCE, 0xC6, 0x3A,    0,    0, 0xD6, 0xDE,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 19 DEC
    0xE0, 0xEC, 0xE4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 20 CPX
    0xE9, 0xED, 0xE5,    0, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9,    0,    0, 0xF2, 0xEF, 0xFF, 0xE7, 0xF7, 0xE3, 0xF3, // 21 SBC
       0, 0xEE, 0xE6, 0x1A,    0,    0, 0xF6, 0xFE,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 22 INC
       0, 0x9C, 0x64,    0,    0,    0, 0x74, 0x9E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 23 STZ
       0, 0x0C, 0x04,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 24 TSB
       0, 0x1C, 0x14,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 25 TRB
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x22,    0,    0,    0,    0,    0, // 26 JSL
       0,    0,    0,    0,    0,    0,    0,    0,    0, 0xDC,    0,    0,    0,    0,    0,    0,    0,    0, // 27 JML
    0xC2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 28 REP
    0xE2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 29 SEP
       0,    0, 0xD4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 30 PEI
       0, 0xF4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0  // 31 PEA
};

// 26 22 JSL abs-long
// 27 DC JML (ind)
// 28 C2 REP imm
// 29 E2 SEP imm
// 30 D4 PEI zp
// 31 F4 PEA absolute

struct OpcdRec M6502_opcdTab[] =
{
    {"BRK",  o_Implied, 0x00},
    {"PHP",  o_Implied, 0x08},
    {"CLC",  o_Implied, 0x18},
    {"PLP",  o_Implied, 0x28},
    {"SEC",  o_Implied, 0x38},
    {"RTI",  o_Implied, 0x40},
    {"PHA",  o_Implied, 0x48},
    {"CLI",  o_Implied, 0x58},
    {"RTS",  o_Implied, 0x60},
    {"PLA",  o_Implied, 0x68},
    {"SEI",  o_Implied, 0x78},
    {"DEY",  o_Implied, 0x88},
    {"TXA",  o_Implied, 0x8A},
    {"TYA",  o_Implied, 0x98},
    {"TXS",  o_Implied, 0x9A},
    {"TAY",  o_Implied, 0xA8},
    {"TAX",  o_Implied, 0xAA},
    {"CLV",  o_Implied, 0xB8},
    {"TSX",  o_Implied, 0xBA},
    {"INY",  o_Implied, 0xC8},
    {"DEX",  o_Implied, 0xCA},
    {"CLD",  o_Implied, 0xD8},
    {"INX",  o_Implied, 0xE8},
    {"NOP",  o_Implied, 0xEA},
    {"SED",  o_Implied, 0xF8},

    {"ASLA", o_Implied, 0x0A},
    {"ROLA", o_Implied, 0x2A},
    {"LSRA", o_Implied, 0x4A},
    {"RORA", o_Implied, 0x6A},

    {"BPL",  o_Branch, 0x10},
    {"BMI",  o_Branch, 0x30},
    {"BVC",  o_Branch, 0x50},
    {"BVS",  o_Branch, 0x70},
    {"BCC",  o_Branch, 0x90},
    {"BCS",  o_Branch, 0xB0},
    {"BNE",  o_Branch, 0xD0},
    {"BEQ",  o_Branch, 0xF0},

    {"ORA",  o_Mode, o_ORA},
    {"ASL",  o_Mode, o_ASL},
    {"JSR",  o_Mode, o_JSR},
    {"AND",  o_Mode, o_AND},
    {"BIT",  o_Mode, o_BIT},
    {"ROL",  o_Mode, o_ROL},
    {"EOR",  o_Mode, o_EOR},
    {"LSR",  o_Mode, o_LSR},
    {"JMP",  o_Mode, o_JMP},
    {"ADC",  o_Mode, o_ADC},
    {"ROR",  o_Mode, o_ROR},
    {"STA",  o_Mode, o_STA},
    {"STY",  o_Mode, o_STY},
    {"STX",  o_Mode, o_STX},
    {"LDY",  o_Mode, o_LDY},
    {"LDA",  o_Mode, o_LDA},
    {"LDX",  o_Mode, o_LDX},
    {"CPY",  o_Mode, o_CPY},
    {"CMP",  o_Mode, o_CMP},
    {"DEC",  o_Mode, o_DEC},
    {"CPX",  o_Mode, o_CPX},
    {"SBC",  o_Mode, o_SBC},
    {"INC",  o_Mode, o_INC},

    // 65C02 opcodes

    {"INCA", o_Implied_65C02, 0x1A},
    {"INA",  o_Implied_65C02, 0x1A},
    {"DECA", o_Implied_65C02, 0x3A},
    {"DEA",  o_Implied_65C02, 0x3A},
    {"PHY",  o_Implied_65C02, 0x5A},
    {"PLY",  o_Implied_65C02, 0x7A},
    {"PHX",  o_Implied_65C02, 0xDA},
    {"PLX",  o_Implied_65C02, 0xFA},

    {"BRA",  o_Branch_65C02, 0x80},

    {"STZ",  o_Mode_65C02, o_STZ},
    {"TSB",  o_Mode_65C02, o_TSB},
    {"TRB",  o_Mode_65C02, o_TRB},

    {"RMB0", o_RSMB, 0x07},
    {"RMB1", o_RSMB, 0x17},
    {"RMB2", o_RSMB, 0x27},
    {"RMB3", o_RSMB, 0x37},
    {"RMB4", o_RSMB, 0x47},
    {"RMB5", o_RSMB, 0x57},
    {"RMB6", o_RSMB, 0x67},
    {"RMB7", o_RSMB, 0x77},

    {"SMB0", o_RSMB, 0x87},
    {"SMB1", o_RSMB, 0x97},
    {"SMB2", o_RSMB, 0xA7},
    {"SMB3", o_RSMB, 0xB7},
    {"SMB4", o_RSMB, 0xC7},
    {"SMB5", o_RSMB, 0xD7},
    {"SMB6", o_RSMB, 0xE7},
    {"SMB7", o_RSMB, 0xF7},

    {"BBR0", o_BBRS, 0x0F},
    {"BBR1", o_BBRS, 0x1F},
    {"BBR2", o_BBRS, 0x2F},
    {"BBR3", o_BBRS, 0x3F},
    {"BBR4", o_BBRS, 0x4F},
    {"BBR5", o_BBRS, 0x5F},
    {"BBR6", o_BBRS, 0x6F},
    {"BBR7", o_BBRS, 0x7F},

    {"BBS0", o_BBRS, 0x8F},
    {"BBS1", o_BBRS, 0x9F},
    {"BBS2", o_BBRS, 0xAF},
    {"BBS3", o_BBRS, 0xBF},
    {"BBS4", o_BBRS, 0xCF},
    {"BBS5", o_BBRS, 0xDF},
    {"BBS6", o_BBRS, 0xEF},
    {"BBS7", o_BBRS, 0xFF},

    // 65C816 opcodes

    {"PHD",  o_Implied_65C816, 0x0B},
    {"TCS",  o_Implied_65C816, 0x1B},
    {"PLD",  o_Implied_65C816, 0x2B},
    {"TSC",  o_Implied_65C816, 0x3B},
    {"WDM",  o_Implied_65C816, 0x42},
    {"PHK",  o_Implied_65C816, 0x4B},
    {"TCD",  o_Implied_65C816, 0x5B},
    {"RTL",  o_Implied_65C816, 0x6B},
    {"TDC",  o_Implied_65C816, 0x7B},
    {"PHB",  o_Implied_65C816, 0x8B},
    {"TXY",  o_Implied_65C816, 0x9B},
    {"PLB",  o_Implied_65C816, 0xAB},
    {"TYX",  o_Implied_65C816, 0xBB},
    {"WAI",  o_Implied_65C816, 0xCB},
    {"STP",  o_Implied_65C816, 0xDB},
    {"XBA",  o_Implied_65C816, 0xEB},
    {"XCE",  o_Implied_65C816, 0xFB},
    {"PER",  o_BranchW,        0x62},
    {"BRL",  o_BranchW,        0x82},
    {"MVP",  o_BlockMove,      0x44},
    {"MVN",  o_BlockMove,      0x54},
    {"COP",  o_COP,            0x02},
    {"JSL",  o_Mode_65C816,    o_JSL},
    {"REP",  o_Mode_65C816,    o_REP},
    {"PEI",  o_Mode_65C816,    o_PEI},
    {"JML",  o_Mode_65C816,    o_JML},
    {"SEP",  o_Mode_65C816,    o_SEP},
    {"PEA",  o_Mode_65C816,    o_PEA},

//  undocumented instructions for original 6502 only
//  see http://www.s-direktnet.de/homepages/k_nadj/opcodes.html
    {"NOP3", o_Implied_6502U, 0x0400},  // 3-cycle NOP
    {"LAX",  o_Mode_6502U,    o_LAX},   // LDA + LDX
    {"DCP",  o_Mode_6502U,    o_DCP},   // DEC + CMP (also called DCM)
    {"ISB",  o_Mode_6502U,    o_ISB},   // INC + SBC (also called INS and ISC)
    {"RLA",  o_Mode_6502U,    o_RLA},   // ROL + AND
    {"RRA",  o_Mode_6502U,    o_RRA},   // ROR + ADC
    {"SAX",  o_Mode_6502U,    o_SAX},   // store (A & X) (also called AXS)
    {"SLO",  o_Mode_6502U,    o_SLO},   // ASL + ORA (also called ASO)
    {"SRE",  o_Mode_6502U,    o_SRE},   // LSR + EOR (also called LSE)
    {"ANC",  o_Mode_6502U,    o_ANC},   // AND# with bit 7 copied to C flag (also called AAC)
    {"ARR",  o_Mode_6502U,    o_ARR},   // AND# + RORA with strange flag results
    {"ASR",  o_Mode_6502U,    o_ASR},   // AND# + LSRA (also called ALR)
    {"SBX",  o_Mode_6502U,    o_SBX},   // X = (A & X) - #nn (also called ASX and SAX)

    {"",     o_Illegal, 0}
};


// --------------------------------------------------------------


void InstrB3(u_char b, u_long l)
{
    InstrClear();
    InstrAddB(b);
    InstrAdd3(l);
}


int M6502_DoCPUOpcode(int typ, int parm)
{
    int     val;
    int     i;
    Str255  word;
    char    *oldLine;
    int     token;
//  char    ch;
//  bool    done;
    bool    forceabs;
    int     opcode;
    int     mode;
    const u_char *modes;     // pointer to current o_Mode instruction's opcodes

    switch(typ)
    {
        case o_Implied_65C816:
            if (curCPU != CPU_65C816) return 0;
        case o_Implied_65C02:
            if (curCPU != CPU_65C02 && curCPU != CPU_65C816) return 0;
        case o_Implied_6502U:
            if (typ == o_Implied_6502U && curCPU != CPU_6502U) return 0;
        case o_Implied:
            if (parm > 256) InstrBB(parm >> 8, parm & 255);
                    else    InstrB (parm);
            break;

        case o_Branch_65C02:
            if (curCPU == CPU_6502) return 0;
        case o_Branch:
            val = EvalBranch(2);
            InstrBB(parm,val);
            break;

        case o_Mode_65C816:
            if (curCPU != CPU_65C816) return 0;
        case o_Mode_65C02:
            if (curCPU != CPU_65C02 && curCPU != CPU_65C816) return 0;
        case o_Mode_6502U:
            if (typ == o_Mode_6502U && curCPU != CPU_6502U) return 0;
        case o_Mode:
            instrLen = 0;
            oldLine = linePtr;
            token = GetWord(word);

                 if (curCPU == CPU_65C02)  modes = mode2op65C02;
            else if (curCPU == CPU_65C816) modes = mode2op65C816;
                                      else modes = mode2op6502;
            modes = modes + parm * a_Max;

            mode = a_None;
            val  = 0;

            if (!token)
               mode = a_Acc;    // accumulator
            else
            {
                switch(token)
                {
                    case '#':   // immediate
                        val  = Eval();
                        mode = a_Imm;
                        break;

                    case '(':   // indirect X,Y
                        val   = Eval();
                        token = GetWord(word);
                        switch (token)
                        {
                            case ',':   // (val,X)
                                token = GetWord(word);
                                if (!word[1]) token = word[0];
                                switch(token)
                                {
                                    case 'X':
                                        RParen();
                                        mode = a_Inx;
                                        break;

                                    case 'S':
                                        if (curCPU == CPU_65C816)
                                        {
                                            if (RParen()) break;
                                            if (Comma()) break;
                                            if (Expect("Y")) break;
                                            mode = a_SIY;
                                        }
                                    default:
                                        ; // fall through to report bad mode
                                }
                                break;

                            case ')':   // (val) -and- (val),Y
                                token = GetWord(word);
                                switch(token)
                                {
                                    case 0:
                                        mode = a_Ind;
                                        if (val >= 0 && val < 256 && evalKnown &&
                                            (modes[a_Zpi] != 0))
                                                    mode = a_Zpi;
                                            else    mode = a_Ind;
                                        break;
                                    case ',':
                                        Expect("Y");
                                        mode = a_Iny;
                                        break;
                                }
                                break;

                            default:
                                mode = a_None;
                                break;
                        }
                        break;

                    case '[':   // a_DIL [d] and a_DIY [d],Y
                        if (curCPU == CPU_65C816)
                        {
                            val = Eval();
                            Expect("]");
                            mode = a_DIL;
                            oldLine = linePtr;
                            switch(GetWord(word))
                            {
                                default:
                                    linePtr = oldLine;
                                    // fall through to let garbage be handled as too many operands
                                case 0:
                                    break;

                                case ',':
                                    GetWord(word);
                                    if ((toupper(word[0]) == 'Y') && (!word[1]))
                                        mode = a_DIY;
                                    else mode = a_None;
                            }
                            break;
                        }
                        // else fall through letting eval() treat the '[' as an open paren

                    default:    // everything else
                        if ((!word[1]) && (toupper(word[0]) == 'A'))
                        {       // accumulator
                            token = GetWord(word);
                            if (token == 0)     // mustn't have anything after the 'A'
                                mode = a_Acc;
                        }
                        else    // absolute and absolute-indexed
                        {
                            // allow '>' in front of address to force absolute addressing
                            // modes instead of zero-page addressing modes
                            forceabs = FALSE;
                            if (token == '>')   forceabs = TRUE;
                                        else    linePtr = oldLine;

                            val   = Eval();
                            token = GetWord(word);

                            switch(token)
                            {
                                case 0:     // abs or zpg
                                    if (val >= 0 && val < 256 && !forceabs &&
                                        evalKnown && (modes[a_Zpg] != 0))
                                                mode = a_Zpg;
                                        else    mode = a_Abs;
                                        if (evalKnown && modes[a_AbL] != 0 && (val & 0xFF0000) != 0)
                                            mode = a_AbL;
                                    break;

                                case ',':   // indexed ,x or ,y or ,s
                                    token = GetWord(word);

                                    if ((toupper(word[0]) == 'X') && (word[1] == 0))
                                    {
                                        if (val >= 0 && val < 256 && !forceabs &&
                                            (evalKnown || modes[a_Abx] == 0))
                                                    mode = a_Zpx;
                                            else    mode = a_Abx;
                                        if (evalKnown && modes[a_ALX] != 0 && (val & 0xFF0000) != 0)
                                            mode = a_ALX;
                                    }
                                    else if ((toupper(word[0]) == 'Y') && (word[1] == 0))
                                    {
                                        if (val >= 0 && val < 256 && !forceabs &&
                                            (evalKnown || modes[a_Aby] == 0)
                                                && modes[a_Zpy] != 0)
                                                    mode = a_Zpy;
                                            else    mode = a_Aby;
                                    }
                                    else if (curCPU == CPU_65C816 && // 65C816 ofs,S
                                             (toupper(word[0]) == 'S') && (word[1] == 0))
                                    {
                                        if (forceabs) BadMode();
                                        else mode = a_Stk;
                                    }
                            }
                        }
                }
            }

            opcode = 0;
            if (mode != a_None)
            {
                opcode = modes[mode];
                if (opcode == 0)
                    mode = a_None;
            }

            instrLen = 0;
            switch(mode)
            {
                case a_None:  
                    Error("Invalid addressing mode");
                    break;

                case a_Acc:
                    InstrB(opcode);
                    break;

                case a_Inx:
                    if (opcode == 0x7C || opcode == 0xFC) // 65C02 JMP (abs,X) / 65C816 JSR (abs,X)
                    {
                        InstrBW(opcode, val);
                        break;
                    }
                    // else fall through

                case a_Zpg:
                case a_Iny:
                case a_Zpx:
                case a_Zpy:
                case a_Zpi:
                case a_Stk:
                case a_DIL:
                case a_DIY:
                case a_SIY:
                    val = (short) val;
                    if (!errFlag && (val < 0 || val > 255))
                        Error("Byte out of range");
                    InstrBB(opcode, val & 0xFF);
                    break;

                case a_Imm:
                    val = (short) val;
                    if (!errFlag && (val < -128 || val > 255))
                        Error("Byte out of range");
                    InstrBB(opcode, val & 0xFF);
                    break;

                case a_Abs:
                case a_Abx:
                case a_Aby:
                case a_Ind:
                    InstrBW(opcode, val);
                    break;

                case a_AbL:
                case a_ALX:
                    InstrB3(opcode, val);
            }
            break;

        case o_RSMB:        //    RMBn/SMBn instructions
            if (curCPU != CPU_65C02) return 0;

            // RMB/SMB zpg
            val = Eval();
            InstrBB(parm,val);
            break;

        case o_BBRS:        //    BBRn/BBSn instructions
            if (curCPU != CPU_65C02) return 0;

            i = Eval();
            Expect(",");
            val = EvalBranch(3);
            InstrBBB(parm,i,val);
            break;

        case o_BranchW:
            if (curCPU != CPU_65C816) return 0;

            val = EvalWBranch(3);
            InstrBW(parm,val);
            break;

        case o_BlockMove:
            if (curCPU != CPU_65C816) return 0;

            val = Eval();
            if (!errFlag && (val < 0 || val > 255))
                Error("Byte out of range");

            if (Comma()) break;

            i = Eval();
            if (!errFlag && (val < 0 || val > 255))
                Error("Byte out of range");

            InstrBBB(parm,i,val);

            break;

        case o_COP:
            if (curCPU != CPU_65C816) return 0;

            val = Eval();
            if (!errFlag && (val < 0 || val > 255))
                Error("Byte out of range");

            InstrBB(parm,val);

            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void Asm6502Init(void)
{
    char *p;

    p = AddAsm(versionName, &M6502_DoCPUOpcode, NULL, NULL);
    AddCPU(p, "6502",   CPU_6502,   LITTLE_END, ADDR_16, LIST_24, 8, 0, M6502_opcdTab);
    AddCPU(p, "65C02",  CPU_65C02,  LITTLE_END, ADDR_16, LIST_24, 8, 0, M6502_opcdTab);
    AddCPU(p, "6502U",  CPU_6502U,  LITTLE_END, ADDR_16, LIST_24, 8, 0, M6502_opcdTab);
    AddCPU(p, "65C816", CPU_65C816, LITTLE_END, ADDR_24, LIST_24, 8, 0, M6502_opcdTab);
    AddCPU(p, "65C816", CPU_65C816, LITTLE_END, ADDR_24, LIST_24, 8, 0, M6502_opcdTab);
}