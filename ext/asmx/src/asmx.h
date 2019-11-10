// asmx.h - copyright 1998-2009 Bruce Tomlin

#ifndef _ASMX_H_
#define _ASMX_H_

#define MAX_BYTSTR  1024    // size of bytStr[]

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#if 0
// these should already be defined in sys/types.h (included from stdio.h)
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;
#endif

// a few useful typedefs
typedef unsigned char  bool;    // define a bool type
enum { FALSE = 0, TRUE = 1 };
typedef char Str255[256];       // generic string type

#define maxOpcdLen  11          // max opcode length (for building opcode table)
typedef char OpcdStr[maxOpcdLen+1];
struct OpcdRec
{
    OpcdStr         name;       // opcode name
    short           typ;        // opcode type
    u_long          parm;       // opcode parameter
};
typedef struct OpcdRec *OpcdPtr;

// CPU option flags
enum
{
    OPT_ATSYM     = 0x01,   // allow symbols to start with '@'
    OPT_DOLLARSYM = 0x02,   // allow symbols to start with '$'
};

void *AddAsm(char *name,        // assembler name
              int (*DoCPUOpcode) (int typ, int parm),
              int (*DoCPULabelOp) (int typ, int parm, char *labl),
              void (*PassInit) (void) );
void AddCPU(void *as,           // assembler for this CPU
            char *name,         // uppercase name of this CPU
            int index,          // index number for this CPU
            int endian,         // assembler endian
            int addrWid,        // assembler 32-bit
            int listWid,        // listing width
            int wordSize,       // addressing word size in bits
            int opts,           // option flags
            struct OpcdRec opcdTab[]); // assembler opcode table

// assembler endian, address width, and listing hex width settings
// 0 = little endian, 1 = big endian, -1 = undefined endian
enum { UNKNOWN_END = -1, LITTLE_END, BIG_END };
enum { ADDR_16, ADDR_24, ADDR_32 };
enum { LIST_16, LIST_24 }; // Note: ADDR_24 and ADDR_32 should always use LIST_24

// special register numbers for FindReg/GetReg
enum
{
    reg_EOL = -2,   // -2
    reg_None,       // -1
};

// opcode constants
// it's stupid that "const int" isn't good enough for these:
#define o_Illegal 0x0100
#define o_LabelOp 0x1000
#define o_EQU (o_LabelOp + 0x100)

void Error(char *message);
void Warning(char *message);
void DefSym(char *symName, u_long val, bool setSym, bool equSym);
int GetWord(char *word);
bool Expect(char *expected);
bool Comma();
bool RParen();
void EatIt();
void IllegalOperand();
void MissingOperand();
void BadMode();
int FindReg(const char *regName, const char *regList);
int GetReg(const char *regList);
int CheckReg(int reg);
int Eval(void);
void CheckByte(int val);
void CheckStrictByte(int val);
void CheckWord(int val);
void CheckStrictWord(int val);
int EvalByte(void);
int EvalBranch(int instrLen);
int EvalWBranch(int instrLen);
int EvalLBranch(int instrLen);
void DoLabelOp(int typ, int parm, char *labl);

void InstrClear(void);
void InstrAddB(u_char b);
void InstrAddX(u_long op);
void InstrAddW(u_short w);
void InstrAdd3(u_long l);
void InstrAddL(u_long l);

void InstrB(u_char b1);
void InstrBB(u_char b1, u_char b2);
void InstrBBB(u_char b1, u_char b2, u_char b3);
void InstrBBBB(u_char b1, u_char b2, u_char b3, u_char b4);
void InstrBBBBB(u_char b1, u_char b2, u_char b3, u_char b4, u_char b5);
void InstrBW(u_char b1, u_short w1);
void InstrBBW(u_char b1, u_char b2, u_short w1);
void InstrBBBW(u_char b1, u_char b2, u_char b3, u_short w1);
void InstrX(u_long op);
void InstrXB(u_long op, u_char b1);
void InstrXBB(u_long op, u_char b1, u_char b2);
void InstrXBBB(u_long op, u_char b1, u_char b2, u_char b3);
void InstrXBBBB(u_long op, u_char b1, u_char b2, u_char b3, u_char b4);
void InstrXW(u_long op, u_short w1);
void InstrXBW(u_long op, u_char b1, u_short w1);
void InstrXBWB(u_long op, u_char b1, u_short w1, u_char b2);
void InstrXWW(u_long op, u_short w1, u_short w2);
void InstrX3(u_long op, u_long l1);
void InstrW(u_short w1);
void InstrWW(u_short w1, u_short w2);
void InstrWL(u_short w1, u_long l1);
void InstrL(u_long l1);
void InstrLL(u_long l1, u_long l2);

//char * ListStr(char *l, char *s);
char * ListByte(char *p, u_char b);
//char * ListWord(char *p, u_short w);
//char * ListLong(char *p, u_long l);
//char * ListAddr(char *p,u_long addr);
//char * ListLoc(u_long addr);

// various internal variables used by the assemblers
extern  bool            errFlag;            // TRUE if error occurred this line
extern  int             pass;               // Current assembler pass
extern  char           *linePtr;            // pointer into current line
extern  int             instrLen;           // Current instruction length (negative to display as long DB)
extern  Str255          line;               // Current line from input file
extern  char           *linePtr;            // pointer into current line
extern  u_long          locPtr;             // Current program address
extern  int             instrLen;           // Current instruction length (negative to display as long DB)
extern  u_char          bytStr[MAX_BYTSTR]; // Current instruction / buffer for long DB statements
extern  bool            showAddr;           // TRUE to show LocPtr on listing
extern  int             endian;             // 0 = little endian, 1 = big endian, -1 = undefined endian
extern  bool            evalKnown;          // TRUE if all operands in Eval were "known"
extern  int             curCPU;             // current CPU index for current assembler
extern  Str255          listLine;           // Current listing line
extern  int             hexSpaces;          // flags for spaces in hex output for instructions
extern  int             listWid;            // listing width: LIST_16, LIST_24

#endif // _ASMX_H_