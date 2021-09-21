// asmx.h - copyright 1998-2009 Bruce Tomlin

#ifndef _ASMX_H_
#define _ASMX_H_

#define ASMX_MAX_BYTSTR  (1024)     // size of bytStr[]

#include <stdint.h>
#include <stdbool.h>

// redirected CRT wrapper functions
typedef uint32_t asmx_FILE;
typedef uint32_t asmx_size_t;
extern asmx_FILE* asmx_stderr;
enum {
    ASMX_SEEK_SET = 0,
    ASMX_SEEK_CUR = 1,
    ASMX_SEEK_END = 2
};
enum {
    ASMX_EOF = -1
};
extern asmx_FILE* asmx_fopen(const char* filename, const char* mode);
extern int asmx_fclose(asmx_FILE* stream);
extern asmx_size_t asmx_fwrite(const void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* stream);
extern asmx_size_t asmx_fread(void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* stream);
extern int asmx_fgetc(asmx_FILE* stream);
extern int asmx_ungetc(int character, asmx_FILE* stream);
extern long int asmx_ftell(asmx_FILE* stream);
extern int asmx_fseek(asmx_FILE* stream, long int offset, int origin);
extern int asmx_fputc(int character, asmx_FILE* stream);
extern int asmx_fprintf(asmx_FILE* stream, const char* format, ...);
extern void* asmx_malloc(asmx_size_t size);
extern void asmx_free(void* ptr);
extern asmx_size_t asmx_strlen(const char* str);
extern char* asmx_strcpy(char* destination, const char* source);
extern char* asmx_strncpy(char* destination, const char* source, asmx_size_t num);
extern int asmx_strcmp(const char* str1, const char* str2);
extern char* asmx_strchr(const char* str, int c);
extern char* asmx_strcat(char * destination, const char * source);
extern int asmx_toupper(int c);
extern int asmx_isdigit(int c);
extern int asmx_sprintf(char* str, const char* format, ...);
extern void* asmx_memcpy(void* dest, const void* src, asmx_size_t count);
extern int asmx_abs(int n);

// a few useful typedefs
enum { FALSE = 0, TRUE = 1 };
typedef char asmx_Str255[256];       // generic string type

#define ASMX_MAXOPCDLEN  11          // max opcode length (for building opcode table)
typedef char asmx_OpcdStr[ASMX_MAXOPCDLEN+1];
struct asmx_OpcdRec
{
    asmx_OpcdStr    name;       // opcode name
    short           typ;        // opcode type
    uint32_t        parm;       // opcode parameter
};
typedef struct asmx_OpcdRec *asmx_OpcdPtr;

// CPU option flags
enum
{
    ASMX_OPT_ATSYM     = 0x01,   // allow symbols to start with '@'
    ASMX_OPT_DOLLARSYM = 0x02,   // allow symbols to start with '$'
};

void *asmx_AddAsm(char *name,        // assembler name
    int (*DoCPUOpcode) (int typ, int parm),
    int (*DoCPULabelOp) (int typ, int parm, char *labl),
    void (*PassInit) (void) );
void asmx_AddCPU(void *as,           // assembler for this CPU
    char *name,         // uppercase name of this CPU
    int index,          // index number for this CPU
    int endian,         // assembler endian
    int addrWid,        // assembler 32-bit
    int listWid,        // listing width
    int wordSize,       // addressing word size in bits
    int opts,           // option flags
    struct asmx_OpcdRec opcdTab[]); // assembler opcode table

// assembler endian, address width, and listing hex width settings
// 0 = little endian, 1 = big endian, -1 = undefined endian
enum { ASMX_UNKNOWN_END = -1, ASMX_LITTLE_END, ASMX_BIG_END };
enum { ASMX_ADDR_16, ASMX_ADDR_24, ASMX_ADDR_32 };
enum { ASMX_LIST_16, ASMX_LIST_24 }; // Note: ADDR_24 and ADDR_32 should always use LIST_24

// special register numbers for FindReg/GetReg
enum
{
    asmx_reg_EOL = -2,   // -2
    asmx_reg_None,       // -1
};

// opcode constants
// it's stupid that "const int" isn't good enough for these:
#define o_Illegal 0x0100
#define o_LabelOp 0x1000
#define o_EQU (o_LabelOp + 0x100)

#if defined(ASMX_6502)
void asmx_Asm6502Init(void);
#elif defined(ASMX_Z80)
void asmx_AsmZ80Init(void);
#else
#error "ASMX: NO CPU DEFINED!"
#endif

void asmx_Error(char *message);
void asmx_Warning(char *message);
void asmx_DefSym(char *symName, uint32_t val, bool setSym, bool equSym);
int asmx_GetWord(char *word);
bool asmx_Expect(char *expected);
bool asmx_Comma();
bool asmx_RParen();
void asmx_EatIt();
void asmx_IllegalOperand();
void asmx_MissingOperand();
void asmx_BadMode();
int asmx_FindReg(const char *regName, const char *regList);
int asmx_GetReg(const char *regList);
int asmx_CheckReg(int reg);
int asmx_Eval(void);
void asmx_CheckByte(int val);
void asmx_CheckStrictByte(int val);
void asmx_CheckWord(int val);
void asmx_CheckStrictWord(int val);
int asmx_EvalByte(void);
int asmx_EvalBranch(int instrLen);
int asmx_EvalWBranch(int instrLen);
int asmx_EvalLBranch(int instrLen);
void asmx_DoLabelOp(int typ, int parm, char *labl);

void asmx_InstrClear(void);
void asmx_InstrAddB(uint8_t b);
void asmx_InstrAddX(uint32_t op);
void asmx_InstrAddW(uint16_t w);
void asmx_InstrAdd3(uint32_t l);
void asmx_InstrAddL(uint32_t l);

void asmx_InstrB(uint8_t b1);
void asmx_InstrBB(uint8_t b1, uint8_t b2);
void asmx_InstrBBB(uint8_t b1, uint8_t b2, uint8_t b3);
void asmx_InstrBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
void asmx_InstrBBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
void asmx_InstrBW(uint8_t b1, uint16_t w1);
void asmx_InstrBBW(uint8_t b1, uint8_t b2, uint16_t w1);
void asmx_InstrBBBW(uint8_t b1, uint8_t b2, uint8_t b3, uint16_t w1);
void asmx_InstrX(uint32_t op);
void asmx_InstrXB(uint32_t op, uint8_t b1);
void asmx_InstrXBB(uint32_t op, uint8_t b1, uint8_t b2);
void asmx_InstrXBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3);
void asmx_InstrXBBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
void asmx_InstrXW(uint32_t op, uint16_t w1);
void asmx_InstrXBW(uint32_t op, uint8_t b1, uint16_t w1);
void asmx_InstrXBWB(uint32_t op, uint8_t b1, uint16_t w1, uint8_t b2);
void asmx_InstrXWW(uint32_t op, uint16_t w1, uint16_t w2);
void asmx_InstrX3(uint32_t op, uint32_t l1);
void asmx_InstrW(uint16_t w1);
void asmx_InstrWW(uint16_t w1, uint16_t w2);
void asmx_InstrWL(uint16_t w1, uint32_t l1);
void asmx_InstrL(uint32_t l1);
void asmx_InstrLL(uint32_t l1, uint32_t l2);

//char * ListStr(char *l, char *s);
char * asmx_ListByte(char *p, uint8_t b);
//char * ListWord(char *p, uint16_t w);
//char * ListLong(char *p, uint32_t l);
//char * ListAddr(char *p,uint32_t addr);
//char * ListLoc(uint32_t addr);

// various internal variables used by the assemblers
typedef struct {
    bool            errFlag;            // TRUE if error occurred this line
    int             pass;               // Current assembler pass
    char           *linePtr;            // pointer into current line
    int             instrLen;           // Current instruction length (negative to display as long DB)
    asmx_Str255     curLine;            // Current line from input file
    uint32_t        locPtr;             // Current program address
    uint8_t         bytStr[ASMX_MAX_BYTSTR];    // Current instruction / buffer for long DB statements
    bool            showAddr;           // TRUE to show LocPtr on listing
    int             curEndian;          // 0 = little endian, 1 = big endian, -1 = undefined endian
    bool            evalKnown;          // TRUE if all operands in Eval were "known"
    int             curCPU;             // current CPU index for current assembler
    asmx_Str255     listLine;           // Current listing line
    int             hexSpaces;          // flags for spaces in hex output for instructions
    int             curListWid;         // listing width: LIST_16, LIST_24
    uint32_t        binBase;            // base address for OBJ_BIN
    uint32_t        binEnd;             // end address for OBJ_BIN
} asmx_Shared_t;
extern asmx_Shared_t asmx_Shared;

#define ASMX_OPTIONS_MAX_DEFINES (16)
typedef struct {
    const char* srcName;
    const char* objName;
    const char* lstName;
    const char* cpuType;
    bool showErrors;
    bool showWarnings;
    const char* defines[16];            // label=value
} asmx_Options;

bool asmx_Assemble(const asmx_Options* opts);

#endif // _ASMX_H_
