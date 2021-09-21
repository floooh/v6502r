// asmx.c - copyright 1998-2007 Bruce Tomlin

#include "asmx.h"
#include <assert.h>

#define VERSION_NAME "asmx multi-assembler"

//#define ENABLE_REP    // uncomment to enable REPEAT pseudo-op (still under development)
//#define DOLLAR_SYM    // allow symbols to start with '$' (incompatible with $ for hexadecimal constants!)
//#define TEMP_LBLAT    // enable use of '@' temporary labels (deprecated)

#ifndef VERSION // should be defined on the command line
#define VERSION "2.0"
#endif
#define COPYRIGHT "Copyright 1998-2007 Bruce Tomlin"
#define IHEX_SIZE   32          // max number of data bytes per line in hex object file
#define MAXSYMLEN   19          // max symbol length (only used in DumpSym())
static const int symTabCols = 3;    // number of columns for symbol table dump
#define MAXMACPARMS 30          // maximum macro parameters
#define MAX_INCLUDE 10          // maximum INCLUDE nesting level
//#define MAX_BYTSTR  1024        // size of bytStr[] (moved to asmx.h)
#define MAX_COND    256         // maximum nesting level of IF blocks
#define MAX_MACRO   10          // maximum nesting level of MACRO invocations

// --------------------------------------------------------------
asmx_Shared_t asmx_Shared;

#define errFlag asmx_Shared.errFlag
#define pass asmx_Shared.pass
#define linePtr asmx_Shared.linePtr
#define instrLen asmx_Shared.instrLen
#define curLine asmx_Shared.curLine
#define locPtr asmx_Shared.locPtr
#define bytStr asmx_Shared.bytStr
#define showAddr asmx_Shared.showAddr
#define curEndian asmx_Shared.curEndian
#define evalKnown asmx_Shared.evalKnown
#define curCPU asmx_Shared.curCPU
#define listLine asmx_Shared.listLine
#define hexSpaces asmx_Shared.hexSpaces
#define curListWid asmx_Shared.curListWid
#define cl_Binbase asmx_Shared.binBase
#define cl_Binend asmx_Shared.binEnd

#define DoLabelOp asmx_DoLabelOp
#define DefSym asmx_DefSym
#define Warning asmx_Warning
#define InstrBB asmx_InstrBB
#define InstrBBB asmx_InstrBBB
#define InstrBW asmx_InstrBW
#define InstrXW asmx_InstrXW
#define InstrXB asmx_InstrXB
#define InstrXBB asmx_InstrXBB
#define AddCPU asmx_AddCPU
#define Error asmx_Error
#define FindReg asmx_FindReg
#define InstrX asmx_InstrX
#define GetReg asmx_GetReg
#define IllegalOperand asmx_IllegalOperand
#define GetWord asmx_GetWord
#define Expect asmx_Expect
#define Comma asmx_Comma
#define RParen asmx_RParen
#define BadMode asmx_BadMode
#define Eval asmx_Eval
#define EvalBranch asmx_EvalBranch
#define EvalWBranch asmx_EvalWBranch
#define AddAsm asmx_AddAsm

struct SymRec
{
    struct SymRec   *next;      // pointer to next symtab entry
    uint32_t        value;      // symbol value
    bool            defined;    // TRUE if defined
    bool            multiDef;   // TRUE if multiply defined
    bool            isSet;      // TRUE if defined with SET pseudo
    bool            equ;        // TRUE if defined with EQU pseudo
    bool            known;      // TRUE if value is known
    char            name[1];    // symbol name, storage = 1 + length
} *symTab = 0;                  // pointer to first entry in symbol table
typedef struct SymRec *SymPtr;

struct MacroLine
{
    struct MacroLine    *next;      // pointer to next macro line
    char                text[1];    // macro line, storage = 1 + length
};
typedef struct MacroLine *MacroLinePtr;

struct MacroParm
{
    struct MacroParm    *next;      // pointer to next macro parameter name
    char                name[1];    // macro parameter name, storage = 1 + length
};
typedef struct MacroParm *MacroParmPtr;

struct MacroRec
{
    struct MacroRec     *next;      // pointer to next macro
    bool                def;        // TRUE after macro is defined in pass 2
    bool                toomany;    // TRUE if too many parameters in definition
    MacroLinePtr        text;       // macro text
    MacroParmPtr        parms;      // macro parms
    int                 nparms;     // number of macro parameters
    char                name[1];    // macro name, storage = 1 + length
} *macroTab = 0;                    // pointer to first entry in macro table
typedef struct MacroRec *MacroPtr;

struct SegRec
{
    struct SegRec       *next;      // pointer to next segment
//  bool                gen;        // FALSE to supress code output (not currently implemented)
    uint32_t            loc;        // locptr for this segment
    uint32_t            cod;        // codptr for this segment
    char                name[1];    // segment name, storage = 1 + length
} *segTab = 0;                      // pointer to first entry in macro table
typedef struct SegRec *SegPtr;

static int             macroCondLevel;     // current IF nesting level inside a macro definition
static int             macUniqueID;        // unique ID, incremented per macro invocation
static int             macLevel;           // current macro nesting level
static int             macCurrentID[MAX_MACRO]; // current unique ID
static MacroPtr        macPtr[MAX_MACRO];  // current macro in use
static MacroLinePtr    macLine[MAX_MACRO]; // current macro text pointer
static int             numMacParms[MAX_MACRO];  // number of macro parameters
static asmx_Str255     macParmsLine[MAX_MACRO]; // text of current macro parameters
static char            *macParms[MAXMACPARMS * MAX_MACRO]; // pointers to current macro parameters
#ifdef ENABLE_REP
static int             macRepeat[MAX_MACRO]; // repeat count for REP pseudo-op
#endif

struct AsmRec
{
    struct AsmRec   *next;          // next AsmRec
    int             (*DoCPUOpcode) (int typ, int parm);
    int             (*DoCPULabelOp) (int typ, int parm, char *labl);
    void            (*PassInit) (void);
    char            name[1];        // name of this assembler
};
typedef struct AsmRec *AsmPtr;

struct CpuRec
{
    struct CpuRec   *next;          // next CpuRec
    AsmPtr          as;             // assembler for CPU type
    int             index;          // CPU type index for assembler
    int             endian;         // endianness for this CPU
    int             addrWid;        // address bus width, ADDR_16 or ADDR_32
    int             listWid;        // listing hex area width, LIST_16 or LIST_24
    int             wordSize;       // addressing word size in bits
    asmx_OpcdPtr    opcdTab;        // opcdTab[] for this assembler
    int             opts;           // option flags
    char            name[1];        // all-uppercase name of CPU
};
typedef struct CpuRec *CpuPtr;

// --------------------------------------------------------------

static SegPtr          curSeg;             // current segment
static SegPtr          nullSeg;            // default null segment

static uint32_t        codPtr;             // Current program "real" address
static bool            warnFlag;           // TRUE if warning occurred this line
static int             errCount;           // Total number of errors

static bool            listLineFF;         // TRUE if an FF was in the current listing line
static bool            listFlag;           // FALSE to suppress listing source
static bool            listThisLine;       // TRUE to force listing this line
static bool            sourceEnd;          // TRUE when END pseudo encountered
static asmx_Str255     lastLabl;           // last label for '@' temp labels
static asmx_Str255     subrLabl;           // current SUBROUTINE label for '.' temp labels
static bool            listMacFlag;        // FALSE to suppress showing macro expansions
static bool            macLineFlag;        // TRUE if line came from a macro
static int             linenum;            // line number in main source file
static bool            expandHexFlag;      // TRUE to expand long hex data to multiple listing lines
static bool            symtabFlag;         // TRUE to show symbol table in listing
static bool            tempSymFlag;        // TRUE to show temp symbols in symbol table listing

static int             condLevel;          // current IF nesting level
static char            condState[MAX_COND]; // state of current nesting level
enum {
    condELSE = 1, // ELSE has already been countered at this level
    condTRUE = 2, // condition is currently true
    condFAIL = 4  // condition has failed (to handle ELSE after ELSIF)
};

static uint32_t        xferAddr;           // Transfer address from END pseudo
static bool            xferFound;          // TRUE if xfer addr defined w/ END

//  Command line parameters
static asmx_Str255     cl_SrcName;         // Source file name
static asmx_Str255     cl_ListName;        // Listing file name
static asmx_Str255     cl_ObjName;         // Object file name
static bool            cl_Err;             // TRUE for errors to screen
static bool            cl_Warn;            // TRUE for warnings to screen
static bool            cl_List;            // TRUE to generate listing file
static bool            cl_Obj;             // TRUE to generate object file
static int             cl_ObjType;         // type of object file to generate:
enum { OBJ_HEX, OBJ_BIN };  // values for cl_Obj
static bool            cl_AutoBinBaseEnd;  // if true, automatically adjust Binbase and Binend

static asmx_FILE       *source;            // source input file
static asmx_FILE       *object;            // object output file
static asmx_FILE       *listing;           // listing output file
static asmx_FILE       *incbin;            // binary include file
static asmx_FILE       *(include[MAX_INCLUDE]);    // include files
static asmx_Str255     incname[MAX_INCLUDE];       // include file names
static int             incline[MAX_INCLUDE];       // include line number
static int             nInclude;           // current include file index

static AsmPtr          asmTab;             // list of all assemblers
static CpuPtr          cpuTab;             // list of all CPU types
static AsmPtr          curAsm;             // current assembler

static int             addrWid;            // CPU address width: ADDR_16, ADDR_32
static int             opts;               // current CPU's option flags
static int             wordSize;           // current CPU's addressing size in bits
static int             wordDiv;            // scaling factor for current word size
static int             addrMax;            // maximum addrWid used
static asmx_OpcdPtr    opcdTab;            // current CPU's opcode table
static asmx_Str255     defCPU;             // default CPU name

// --------------------------------------------------------------

enum
{
//  0x00-0xFF = CPU-specific opcodes

//    o_Illegal = 0x100,  // opcode not found in FindOpcode

    o_DB = o_Illegal + 1,       // DB pseudo-op
    o_DW,       // DW pseudo-op
    o_DL,       // DL pseudo-op
    o_DWRE,     // reverse-endian DW
    o_DS,       // DS pseudo-op
    o_HEX,      // HEX pseudo-op
    o_FCC,      // FCC pseudo-op
    o_ZSCII,    // ZSCII pseudo-op
    o_ASCIIC,   // counted DB pseudo-op
    o_ASCIIZ,   // null-terminated DB pseudo-op
    o_ALIGN,    // ALIGN pseudo-op
    o_ALIGN_n,  // for EVEN pseudo-op

    o_END,      // END pseudo-op
    o_Include,  // INCLUDE pseudo-op

    o_ENDM,     // ENDM pseudo-op
#ifdef ENABLE_REP
    o_REPEND,   // REPEND pseudo-op
#endif
    o_MacName,  // Macro name
    o_Processor,// CPU selection pseudo-op

//    o_LabelOp = 0x1000,   // flag to handle opcode in DoLabelOp

//  0x1000-0x10FF = CPU-specific label-ops

    // the following pseudo-ops handle the label field specially
//    o_EQU = o_LabelOp + 0x100,      // EQU and SET pseudo-ops
    o_ORG = o_EQU + 1,      // ORG pseudo-op
    o_RORG,     // RORG pseudo-op
    o_REND,     // REND pseudo-op
    o_LIST,     // LIST pseudo-op
    o_OPT,      // OPT pseudo-op
    o_ERROR,    // ERROR pseudo-op
    o_ASSERT,   // ASSERT pseudo-op
    o_MACRO,    // MACRO pseudo-op
#ifdef ENABLE_REP
    o_REPEAT,   // REPEAT pseudo-op
#endif
    o_Incbin,   // INCBIN pseudo-op
    o_WORDSIZE, // WORDSIZE pseudo-op

    o_SEG,      // SEG pseudo-op
    o_SUBR,     // SUBROUTINE pseudo-op

    o_IF,       // IF <expr> pseudo-op
    o_ELSE,     // ELSE pseudo-op
    o_ELSIF,    // ELSIF <expr> pseudo-op
    o_ENDIF     // ENDIF pseudo-op
};

struct asmx_OpcdRec opcdTab2[] =
{
    {"DB",        o_DB,       0},
    {"FCB",       o_DB,       0},
    {"BYTE",      o_DB,       0},
    {"DC.B",      o_DB,       0},
    {"DFB",       o_DB,       0},
    {"DEFB",      o_DB,       0},

    {"DW",        o_DW,       0},
    {"FDB",       o_DW,       0},
    {"WORD",      o_DW,       0},
    {"DC.W",      o_DW,       0},
    {"DFW",       o_DW,       0},
    {"DA",        o_DW,       0},
    {"DEFW",      o_DW,       0},
    {"DRW",       o_DWRE,     0},

    {"LONG",      o_DL,       0},
    {"DL",        o_DL,       0},
    {"DC.L",      o_DL,       0},

    {"DS",        o_DS,       1},
    {"DS.B",      o_DS,       1},
    {"RMB",       o_DS,       1},
    {"BLKB",      o_DS,       1},
    {"DEFS",      o_DS,       1},
    {"DS.W",      o_DS,       2},
    {"BLKW",      o_DS,       2},
    {"DS.L",      o_DS,       4},
    {"HEX",       o_HEX,      0},
    {"FCC",       o_FCC,      0},
    {"ZSCII",     o_ZSCII,    0},
    {"ASCIIC",    o_ASCIIC,   0},
    {"ASCIZ",     o_ASCIIZ,   0},
    {"ASCIIZ",    o_ASCIIZ,   0},
    {"END",       o_END,      0},
    {"ENDM",      o_ENDM,     0},
    {"ALIGN",     o_ALIGN,    0},
    {"EVEN",      o_ALIGN_n,  2},
#ifdef ENABLE_REP
    {"REPEND",    o_REPEND,   0},
#endif
    {"INCLUDE",   o_Include,  0},
    {"INCBIN",    o_Incbin,   0},
    {"PROCESSOR", o_Processor,0},
    {"CPU",       o_Processor,0},

    {"=",         o_EQU,      0},
    {"EQU",       o_EQU,      0},
    {":=",        o_EQU,      1},
    {"SET",       o_EQU,      1},
    {"DEFL",      o_EQU,      1},
    {"ORG",       o_ORG,      0},
    {"AORG",      o_ORG,      0},
    {"RORG",      o_RORG,     0},
    {"REND",      o_REND,     0},
    {"LIST",      o_LIST,     0},
    {"OPT",       o_OPT,      0},
    {"ERROR",     o_ERROR,    0},
    {"ASSERT",    o_ASSERT,   0},
#ifdef ENABLE_REP
    {"REPEAT",    o_REPEAT,   0},
#endif
    {"MACRO",     o_MACRO,    0},
    {"SEG",       o_SEG,      1},
    {"RSEG",      o_SEG,      1},
    {"SEG.U",     o_SEG,      0},
    {"SUBR",      o_SUBR,     0},
    {"SUBROUTINE",o_SUBR,     0},
    {"IF",        o_IF,       0},
    {"ELSE",      o_ELSE,     0},
    {"ELSIF",     o_ELSIF,    0},
    {"ENDIF",     o_ENDIF,    0},
    {"WORDSIZE",  o_WORDSIZE, 0},

    {"",          o_Illegal, 0}
};


// --------------------------------------------------------------

#ifdef ENABLE_REP
void DoLine(void);          // forward declaration
#endif

// --------------------------------------------------------------

// multi-assembler call vectors

static int DoCPUOpcode(int typ, int parm)
{
    if (curAsm && curAsm -> DoCPUOpcode)
        return curAsm -> DoCPUOpcode(typ,parm);
    else return 0;
}


static int DoCPULabelOp(int typ, int parm, char *labl)
{
    if (curAsm && curAsm -> DoCPULabelOp)
        return curAsm -> DoCPULabelOp(typ,parm,labl);
    else return 0;
}


static void PassInit(void)
{
    AsmPtr p;

    // for each assembler, call PassInit
    p = asmTab;
    while(p)
    {
        if (p -> PassInit)
            p -> PassInit();
        p = p -> next;
    }
}


void *AddAsm(char *name,        // assembler name
    int (*DoCPUOpcode) (int typ, int parm),
    int (*DoCPULabelOp) (int typ, int parm, char *labl),
    void (*PassInit) (void) )
{
    AsmPtr p;

    p = asmx_malloc(sizeof *p + asmx_strlen(name));

    asmx_strcpy(p -> name, name);
    p -> next     = asmTab;
    p -> DoCPUOpcode  = DoCPUOpcode;
    p -> DoCPULabelOp = DoCPULabelOp;
    p -> PassInit = PassInit;

    asmTab = p;

    return p;
}

void AddCPU(void *as,           // assembler for this CPU
    char *name,         // uppercase name of this CPU
    int index,          // index number for this CPU
    int endian,         // assembler endian
    int addrWid,        // assembler 32-bit
    int listWid,        // listing width
    int wordSize,       // addressing word size in bits
    int opts,           // option flags
    struct asmx_OpcdRec opcdTab[])  // assembler opcode table
{
    CpuPtr p;

    p = asmx_malloc(sizeof *p + asmx_strlen(name));

    asmx_strcpy(p -> name, name);
    p -> next  = cpuTab;
    p -> as    = (AsmPtr) as;
    p -> index = index;
    p -> endian   = endian;
    p -> addrWid  = addrWid;
    p -> listWid  = listWid;
    p -> wordSize = wordSize;
    p -> opts     = opts;
    p -> opcdTab  = opcdTab;

    cpuTab = p;
}

static CpuPtr FindCPU(char *cpuName)
{
    CpuPtr p;

    p = cpuTab;
    while (p)
    {
        if (asmx_strcmp(cpuName,p->name) == 0)
            return p;
        p = p -> next;
    }

    return 0;
}


static void SetWordSize(int wordSize)
{
    wordDiv = (wordSize + 7) / 8;
}


static void CodeFlush(void);
// sets up curAsm and curCpu based on cpuName, returns non-zero if success
static bool SetCPU(char *cpuName)
{
    CpuPtr p;

    p = FindCPU(cpuName);
    if (p)
    {
        curCPU     = p -> index;
        curAsm     = p -> as;
        curEndian  = p -> endian;
        addrWid    = p -> addrWid;
        curListWid = p -> listWid;
        wordSize   = p -> wordSize;
        opcdTab    = p -> opcdTab;
        opts       = p -> opts;
        SetWordSize(wordSize);

        CodeFlush();    // make a visual change in the hex object file

        return 1;
    }

    return 0;
}


static int isalphanum(char c);
static void Uprcase(char *s);


// MODIFIED v6502
static void AsmInit(void)
{
    char *p;

#define ASSEMBLER(name) extern void wasmx_Asm ## name ## Init(void); asmx_Asm ## name ## Init();

    p = AddAsm("None", 0, 0, 0);
    AddCPU(p, "NONE",  0, ASMX_UNKNOWN_END, ASMX_ADDR_32, ASMX_LIST_24, 8, 0, 0);
    #if defined(ASMX_6502)
        ASSEMBLER(6502);
        asmx_strcpy(defCPU,"6502");
    #elif defined(ASMX_Z80)
        ASSEMBLER(Z80);
        asmx_strcpy(defCPU, "Z80");
    #else
    #error "ASMX: NO CPU DEFINED!"
    #endif
}


// --------------------------------------------------------------
// error messages


/*
 *  Error
 */

void Error(char *message)
{
    char *name;
    int line_;

    errFlag = TRUE;
    errCount++;

    name = cl_SrcName;
    line_ = linenum;
    if (nInclude >= 0)
    {
        name = incname[nInclude];
        line_ = incline[nInclude];
    }

    if (pass == 2)
    {
        listThisLine = TRUE;
        if (cl_List)    asmx_fprintf(listing, "%s:%d: *** Error:  %s ***\n",name,line_,message);
        if (cl_Err)     asmx_fprintf(asmx_stderr,  "%s:%d: *** Error:  %s ***\n",name,line_,message);
    }
}


/*
 *  Warning
 */

void Warning(char *message)
{
    char *name;
    int line_;

    warnFlag = TRUE;

    name = cl_SrcName;
    line_ = linenum;
    if (nInclude >= 0)
    {
        name = incname[nInclude];
        line_ = incline[nInclude];
    }

    if (pass == 2 && cl_Warn)
    {
        listThisLine = TRUE;
        if (cl_List)    asmx_fprintf(listing, "%s:%d: *** Warning:  %s ***\n",name,line_,message);
        if (cl_Warn)    asmx_fprintf(asmx_stderr,  "%s:%d: *** Warning:  %s ***\n",name,line_,message);
    }
}


// --------------------------------------------------------------
// string utilities


/*
 *  Debleft deblanks the string s from the left side
 */
/*
static void Debleft(char *s)
{
    char *p = s;

    while (*p == 9 || *p == ' ')
        p++;

    if (p != s)
        while ((*s++ = *p++));
}
*/

/*
 *  Debright deblanks the string s from the right side
 */
static void Debright(char *s)
{
    char *p = s + asmx_strlen(s);

    while (p>s && *--p == ' ')
        *p = 0;
}

/*
 *  Deblank removes blanks from both ends of the string s
 */
/*
static void Deblank(char *s)
{
    Debleft(s);
    Debright(s);
}
*/


/*
 *  Uprcase converts string s to upper case
 */

static void Uprcase(char *s)
{
    char *p = s;

    while ((*p = asmx_toupper(*p)))
        p++;
}


static int ishex(char c)
{
    c = asmx_toupper(c);
    return asmx_isdigit(c) || ('A' <= c && c <= 'F');
}


static int isalphaul(char c)
{
    c = asmx_toupper(c);
    return ('A' <= c && c <= 'Z') || c == '_';
}


static int isalphanum(char c)
{
    c = asmx_toupper(c);
    return asmx_isdigit(c) || ('A' <= c && c <= 'Z') || c == '_';
}


static uint32_t EvalBin(char *binStr)
{
    uint32_t   binVal;
    int     evalErr;
    int     c;

    evalErr = FALSE;
    binVal  = 0;

    while ((c = *binStr++))
    {
        if (c < '0' || c > '1')
            evalErr = TRUE;
        else
            binVal = binVal * 2 + c - '0';
    }

    if (evalErr)
    {
      binVal = 0;
      Error("Invalid binary number");
    }

   return binVal;
}


static uint32_t EvalOct(char *octStr)
{
    uint32_t   octVal;
    int     evalErr;
    int     c;

    evalErr = FALSE;
    octVal  = 0;

    while ((c = *octStr++))
    {
        if (c < '0' || c > '7')
            evalErr = TRUE;
        else
            octVal = octVal * 8 + c - '0';
    }

    if (evalErr)
    {
      octVal = 0;
      Error("Invalid octal number");
    }

   return octVal;
}


static uint32_t EvalDec(char *decStr)
{
    uint32_t   decVal;
    int     evalErr;
    int     c;

    evalErr = FALSE;
    decVal  = 0;

    while ((c = *decStr++))
    {
        if (!asmx_isdigit(c))
            evalErr = TRUE;
        else
            decVal = decVal * 10 + c - '0';
    }

    if (evalErr)
    {
      decVal = 0;
      Error("Invalid decimal number");
    }

   return decVal;
}


static int Hex2Dec(int c)
{
    c = asmx_toupper(c);
    if (c > '9')
        return c - 'A' + 10;
    return c - '0';
}


static uint32_t EvalHex(char *hexStr)
{
    uint32_t   hexVal;
    int     evalErr;
    int     c;

    evalErr = FALSE;
    hexVal  = 0;

    while ((c = *hexStr++))
    {
        if (!ishex(c))
            evalErr = TRUE;
        else
            hexVal = hexVal * 16 + Hex2Dec(c);
    }

    if (evalErr)
    {
      hexVal = 0;
      Error("Invalid hexadecimal number");
    }

    return hexVal;
}


static uint32_t EvalNum(char *word)
{
    int val;

    // handle C-style 0xnnnn hexadecimal constants
    if(word[0] == '0')
    {
        if (asmx_toupper(word[1]) == 'X')
        {
            return EvalHex(word+2);
        }
        // return EvalOct(word);    // 0nnn octal constants are in less demand, though
    }

    val = asmx_strlen(word) - 1;
    switch(word[val])
    {
        case 'B':
            word[val] = 0;
            val = EvalBin(word);
            break;

        case 'O':
            word[val] = 0;
            val = EvalOct(word);
            break;

        case 'D':
            word[val] = 0;
            val = EvalDec(word);
            break;

        case 'H':
            word[val] = 0;
            val = EvalHex(word);
            break;

        default:
            val = EvalDec(word);
            break;
    }

    return val;
}


// --------------------------------------------------------------
// listing hex output routines

static char * ListStr(char *l, char *s)
{
    // copy string to line
    while (*s) *l++ = *s++;

    return l;
}


static char * ListByte(char *p, uint8_t b)
{
    char s[16]; // with extra space for just in case

    asmx_sprintf(s,"%.2X",b);
    return ListStr(p,s);
}


static char * ListWord(char *p, uint16_t w)
{
    char s[16]; // with extra space for just in case

    asmx_sprintf(s,"%.4X",w);
    return ListStr(p,s);
}


static char * ListLong24(char *p, uint32_t l)
{
    char s[16]; // with extra space for just in case

    asmx_sprintf(s,"%.6lX",l & 0xFFFFFF);
    return ListStr(p,s);
}


static char * ListLong(char *p, uint32_t l)
{
    char s[16]; // with extra space for just in case

    asmx_sprintf(s,"%.8lX",l);
    return ListStr(p,s);
}


static char * ListAddr(char *p,uint32_t addr)
{
    switch(addrWid)
    {
        default:
        case ASMX_ADDR_16:
            p = ListWord(p,addr);
            break;

        case ASMX_ADDR_24:
            p = ListLong24(p,addr);
            break;

        case ASMX_ADDR_32: // you need listWid == LIST_24 with this too
            p = ListLong(p,addr);
            break;
    };

    return p;
}

static char * ListLoc(uint32_t addr)
{
    char *p;

    p = ListAddr(listLine,addr);
    *p++ = ' ';
    if (curListWid == ASMX_LIST_24 && addrWid == ASMX_ADDR_16)
        *p++ = ' ';

    return p;
}

// --------------------------------------------------------------
// ZSCII conversion routines

static uint8_t zStr[ASMX_MAX_BYTSTR];   // output data buffer
static int     zLen;               // length of output data
static int     zOfs,zPos;          // current output offset (in bytes) and bit position
static int     zShift;             // current shift lock status (0, 1, 2)
static char    zSpecial[] = "0123456789.,!?_#'\"/\\<-:()"; // special chars table

static void InitZSCII(void)
{
    zOfs = 0;
    zPos = 0;
    zLen = 0;
    zShift = 0;
}


static void PutZSCII(char nib)
{
    nib = nib & 0x1F;

    // is it time to start a new word?
    if (zPos == 3)
    {
        // check for overflow
        if (zOfs >= ASMX_MAX_BYTSTR)
        {
            if (!errFlag) Error("ZSCII string length overflow");
            return;
        }
        zOfs = zOfs + 2;
        zPos = 0;
    }

    switch(zPos)
    {
        case 0:
            zStr[zOfs] = nib << 2;
            break;

        case 1:
            zStr[zOfs] |= nib >> 3;
            zStr[zOfs+1] = nib << 5;
            break;

        case 2:
            zStr[zOfs+1] |= nib;
            break;

        default:    // this shouldn't happen!
            break;
    }

    zPos++;
}


static void PutZSCIIShift(char shift, char nshift)
{
    int lock;

    lock = 0;                       // only do a temp shift if next shift is different
    if (shift == nshift) lock = 2;  // generate shift lock if next shift is same

    switch ((shift - zShift + 3) % 3)
    {
        case 0: // no shift
            break;

        case 1: // shift forward
            PutZSCII(0x02 + lock);
            break;

        case 2: // shift backwards
            PutZSCII(0x03 + lock);
            break;
    }

    if (lock) zShift = shift;       // update shift lock state
}


static void EndZSCII(void)
{
    // pad final word with shift nibbles
    while (zPos != 3)
        PutZSCII(0x05);

    // set high bit in final word as end-of-text marker
    zStr[zOfs] |= 0x80;

    zOfs = zOfs + 2;
}


static int GetZSCIIShift(char ch)
{
        if (ch == ' ')  return -2;
        if (ch == '\n') return -1;
        if ('a' <= ch && ch <= 'z') return 0;
        if ('A' <= ch && ch <= 'Z') return 1;
        return 2;
}


static void ConvertZSCII(void)
{
    //  input data is in bytStr[]
    //  input data length is instrLen

    int     i,inpos;            // input position
    char    ch;                 // current and next input byte
    char    *p;                 // pointer for looking up special chars
    int     shift,nshift;       // current and next shift states

    InitZSCII();

    inpos = 0;
    while (inpos < instrLen)
    {
        // get current char and shift
        ch = bytStr[inpos];
        shift = GetZSCIIShift(ch);
        nshift = shift;

        // determine next char's shift (skipping blanks and newlines, stopping at end of data)
        i = inpos + 1;
        while (i < instrLen && (nshift = GetZSCIIShift(bytStr[i])) < 0) i++;
        if (i >= instrLen) nshift = zShift;    // if end of data, use current shift as "next" shift

        switch(shift)
        {
            case 0: // alpha lower case
            case 1: // alpha upper case
                PutZSCIIShift(shift,nshift);
                PutZSCII(ch - 'A' + 6);
                break;

            case 2: // non-alpha
                if ((p = asmx_strchr(zSpecial,ch)))
                {   // numeric and special chars
                    PutZSCIIShift(shift,nshift);
                    PutZSCII((char)(p - zSpecial + 7));
                }
                else
                {   // extended char
                    PutZSCIIShift(shift,nshift);
                    PutZSCII(0x06);
                    PutZSCII(ch >> 5);
                    PutZSCII(ch);
                }
                break;

            default: // blank and newline
                PutZSCII(shift + 2);
                break;
        }

        inpos++;
    }

    EndZSCII();

    asmx_memcpy(bytStr,zStr,zOfs);
    instrLen = zOfs;
}


// --------------------------------------------------------------
// token handling

// returns 0 for end-of-line, -1 for alpha-numeric, else char value for non-alphanumeric
// converts the word to uppercase, too
int GetWord(char *word)
{
    uint8_t  c;

    word[0] = 0;

    // skip initial whitespace
    c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
        c = *++linePtr;

    // skip comments
    if (c == ';')
        while (c)
            c = *++linePtr;

    // test for end of line
    if (c)
    {
        // test for alphanumeric token
#if 1
        if (isalphanum(c) ||
            (
             (((opts & ASMX_OPT_DOLLARSYM) && c == '$') || ((opts & ASMX_OPT_ATSYM) && c == '@'))
             && ((isalphanum(linePtr[1]) ||
                 linePtr[1]=='$' ||
                 ((opts & ASMX_OPT_ATSYM) && linePtr[1]=='@'))
                )
           ))

#else

#ifdef DOLLAR_SYM
        if (isalphanum(c) || (c == '$' && (isalphanum(linePtr[1]) || linePtr[1]=='$')))
#else
        if (isalphanum(c))
#endif

#endif
        {
            while (isalphanum(c) || c == '$' || ((opts & ASMX_OPT_ATSYM) && c == '@'))
            {
                *word++ = asmx_toupper(c);
                c = *++linePtr;
            }
            *word = 0;
            return -1;
        }
        else
        {
            word[0] = c;
            word[1] = 0;
            linePtr++;
            return c;
        }
    }

    return 0;
}

// same as GetWord, except it allows '.' chars in alphanumerics and ":=" as a token
static int GetOpcode(char *word)
{
    uint8_t  c;

    word[0] = 0;

    // skip initial whitespace
    c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
        c = *++linePtr;

    // skip comments
    if (c == ';')
        while (c)
            c = *++linePtr;

    // test for ":="
    if (c == ':' && linePtr[1] == '=')
    {
        word[0] = ':';
        word[1] = '=';
        word[2] = 0;
        linePtr = linePtr + 2;
        return -1;
    }

    // test for end of line
    else if (c)
    {
        // test for alphanumeric token
        if (isalphanum(c) || c=='.')
        {
            while (isalphanum(c) || c=='.')
            {
                *word++ = asmx_toupper(c);
                c = *++linePtr;
            }
            *word = 0;
            return -1;
        }
        else
        {
            word[0] = c;
            word[1] = 0;
            linePtr++;
            return c;
        }
    }

    return 0;
}


static void GetFName(char *word)
{
    char            *oldLine;
    int             ch;
    uint8_t          quote;

    // skip leading whitespace
    while (*linePtr == ' ' || *linePtr == '\t')
        linePtr++;
    oldLine = word;

    // check for quote at start of file name
    quote = 0;
    if (*linePtr == '"' || *linePtr == 0x27)
        quote = *linePtr++;

    // continue reading until quote or whitespace or EOL
    while (*linePtr != 0 && *linePtr != quote && (quote || (*linePtr != ' ' && *linePtr != '\t')))
    {
        ch = *linePtr++;
        if (ch == '\\' && *linePtr != 0)
            ch = *linePtr++;
        *oldLine++ = ch;
    }
    *oldLine++ = 0;

    // if looking for quote, error on end quote
    if (quote)
    {
        if (*linePtr == quote)
            linePtr++;
        else
            Error("Missing close quote");
    }
}


bool Expect(char *expected)
{
    asmx_Str255 s;
    GetWord(s);
    if (asmx_strcmp(s,expected) != 0)
    {
        asmx_sprintf(s,"\"%s\" expected",expected);
        Error(s);
        return 1;
    }
    return 0;
}


bool Comma()
{
    return Expect(",");
}


bool RParen()
{
    return Expect(")");
}

static void EatIt()
{
    asmx_Str255  word;
    while (GetWord(word));      // eat junk at end of line
}


/*
 *  IllegalOperand
 */

void IllegalOperand()
{
    Error("Illegal operand");
    EatIt();
}


/*
 *  MissingOperand
 */

static void MissingOperand()
{
    Error("Missing operand");
    EatIt();
}


/*
 *  BadMode
 */
void BadMode()
{
    Error("Illegal addressing mode");
    EatIt();
}


// find a register name
// regList is a space-separated list of register names
// FindReg returns:
//       -2 (aka reg_EOL) if empty string
//      -1 (aka reg_None) if no register found
//      0 if regName is the first register in regList
//      1 if regName is the second register in regList
//      etc.
int FindReg(const char *regName, const char *regList)
{
    const char *p;
    int i;

    if (!regName[0]) return asmx_reg_EOL;

    i = 0;
    while (*regList)
    {
        p = regName;
        // compare words
        while (*p && *p == *regList)
        {
            regList++;
            p++;
        }

        // if not match, skip rest of word
        if (*p || (*regList != 0 && *regList != ' '))
        {
            // skip to next whitespace
            while (*regList && *regList != ' ')
                regList++;
            // skip to next word
            while (*regList == ' ')
                regList++;
            i++;
        }
        else return i;
    }

    return asmx_reg_None;
}


// get a word and find a register name
// regList is a space-separated list of register names
// GetReg returns:
//      -2 (aka reg_EOL) and reports a "missing operand" error if end of line
//      -1 (aka reg_None) if no register found
//      0 if regName is the first register in regList
//      1 if regName is the second register in regList
//      etc.
int GetReg(const char *regList)
{
    asmx_Str255  word;

    if (!GetWord(word))
    {
        MissingOperand();
        return asmx_reg_EOL;
    }

    return FindReg(word,regList);
}


// check if a register from FindReg/GetReg is valid
/*
static int CheckReg(int reg) // may want to add a maxreg parameter
{
    if (reg == asmx_reg_EOL)
    {
        MissingOperand();      // abort if not valid register
        return 1;
    }
//  if ((reg < 0) || (reg > maxReg))
    if (reg < 0)
    {
        IllegalOperand();      // abort if not valid register
        return 1;
    }
    return 0;
}
*/


static uint32_t GetBackslashChar(void)
{
    uint8_t      ch;
    asmx_Str255      s;

    if (*linePtr)
    {
        ch = *linePtr++;
            if (ch == '\\' && *linePtr != 0) // backslash
        {
            ch = *linePtr++;
            switch(ch)
            {
                case 'r':   ch = '\r';   break;
                case 'n':   ch = '\n';   break;
                case 't':   ch = '\t';   break;
                case 'x':
                    if (ishex(linePtr[0]) && ishex(linePtr[1]))
                    {
                        s[0] = linePtr[0];
                        s[1] = linePtr[1];
                        s[2] = 0;
                        linePtr = linePtr + 2;
                        ch = EvalHex(s);
                    }
                    break;
                default:   break;
            }
        }
    }
    else ch = -1;

    return ch;
}


// --------------------------------------------------------------
// macro handling


static MacroPtr FindMacro(char *name)
{
    MacroPtr p = macroTab;
    bool found = FALSE;

    while (p && !found)
    {
        found = (asmx_strcmp(p -> name, name) == 0);
        if (!found)
            p = p -> next;
    }

    return p;
}


static MacroPtr NewMacro(char *name)
{
    MacroPtr    p;

    p = asmx_malloc(sizeof *p + asmx_strlen(name));

    if (p)
    {
        asmx_strcpy(p -> name, name);
        p -> def     = FALSE;
        p -> toomany = FALSE;
        p -> text    = 0;
        p -> next    = macroTab;
        p -> parms   = 0;
        p -> nparms  = 0;
    }

    return p;
}
    

static MacroPtr AddMacro(char *name)
{
    MacroPtr    p;

    p = NewMacro(name);
    if (p)
        macroTab = p;

    return p;
}
    

static void AddMacroParm(MacroPtr macro, char *name)
{
    MacroParmPtr    parm;
    MacroParmPtr    p;

    parm = asmx_malloc(sizeof *parm + asmx_strlen(name));
    parm -> next = 0;
    asmx_strcpy(parm -> name, name);
    macro -> nparms++;

    p = macro -> parms;
    if (p)
    {
        while (p -> next)
            p = p -> next;
        p -> next = parm;
    }
    else macro -> parms = parm;
}


static void AddMacroLine(MacroPtr macro, char *line_)
{
    MacroLinePtr    m;
    MacroLinePtr    p;

    m = asmx_malloc(sizeof *m + asmx_strlen(line_));
    if (m)
    {
        m -> next = 0;
        asmx_strcpy(m -> text, line_);

        p = macro -> text;
        if (p)
        {
            while (p -> next)
                p = p -> next;
            p -> next = m;
        }
        else macro -> text = m;
    }
}


static void GetMacParms(MacroPtr macro)
{
    int     i;
    int     n;
    int     quote;
    char    c;
    char    *p;
    bool    done;

    macCurrentID[macLevel] = macUniqueID++;

    for (i=0; i<MAXMACPARMS; i++)
        macParms[i + macLevel * MAXMACPARMS] = 0;

    // skip initial whitespace
    c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
        c = *++linePtr;

    // copy rest of line for safekeeping
    asmx_strcpy(macParmsLine[macLevel], linePtr);

    n = 0;
    p = macParmsLine[macLevel];
    while (*p && *p != ';' && n<MAXMACPARMS)
    {
        // skip whitespace before current parameter
        c = *p;
        while (c == 12 || c == '\t' || c == ' ')
            c = *++p;

        // record start of parameter
        macParms[n + macLevel * MAXMACPARMS] = p;
        n++;

        quote = 0;
        done = FALSE;
        // skip to end of parameter
        while(!done)
        {
            c = *p++;
            switch(c)
            {
                case 0:
                    --p;
                    done = TRUE;
                    break;

                case ';':
                    if (quote==0)
                    {
                        *--p = 0;
                        done = TRUE;
                    }
                    break;

                case ',':
                    if (quote==0)
                    {
                        *(p-1) = 0;
                        done = TRUE;
                    }
                    break;

                case 0x27:  // quote
                case '"':
                    if (quote == 0)
                        quote = c;
                    else if (quote == c)
                        quote = 0;
            }
        }
    }

    numMacParms[macLevel] = n;

    // terminate last parameter and point remaining parameters to null strings
    *p = 0;
    for (i=n; i<MAXMACPARMS; i++)
        macParms[i + macLevel * MAXMACPARMS] = p;

    // remove whitespace from end of parameter
    for (i=0; i<MAXMACPARMS; i++)
        if (macParms[i + macLevel * MAXMACPARMS])
        {
            p = macParms[i + macLevel * MAXMACPARMS] + asmx_strlen(macParms[i + macLevel * MAXMACPARMS]) - 1;
            while (p>=macParms[i + macLevel * MAXMACPARMS] && (*p == ' ' || *p == 9))
                *p-- = 0;
        }

    if (n > macro -> nparms || n > MAXMACPARMS)
        Error("Too many macro parameters");
}


static void DoMacParms()
{
    int             i;
    asmx_Str255     word,word2;
    MacroParmPtr    parm;
    char            *p;     // pointer to start of word
    char            c;
    int             token;

    // start at beginning of line
    linePtr = curLine;

    // skip initial whitespace
    c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
        c = *++linePtr;

    // while not end of line
    p = linePtr;
    token = GetWord(word);
    while (token)
    {
        // if alphanumeric, search for macro parameter of the same name
        if (token == -1)
        {
            i = 0;
            parm = macPtr[macLevel] -> parms;
            while (parm && asmx_strcmp(parm -> name, word))
            {
                parm = parm -> next;
                i++;
            }

            // if macro parameter found, replace parameter name with parameter value
            if (parm)
            {
                // copy from linePtr to temp string
                asmx_strcpy(word, linePtr);
                // copy from corresponding parameter to p
                asmx_strcpy(p, macParms[i + macLevel * MAXMACPARMS]);
                // point p to end of appended text
                p = p + asmx_strlen(macParms[i + macLevel * MAXMACPARMS]);
                // copy from temp to p
                asmx_strcpy(p, word);
                // update linePtr
                linePtr = p;
            }
        }
        // handle '##' concatenation operator
        else if (token == '#' && *linePtr == '#')
        {
            p = linePtr + 1;    // skip second '#'
            linePtr--;          // skip first '#'
            // skip whitespace to the left
            while (linePtr > curLine && linePtr[-1] == ' ')
                linePtr--;
            // skip whitespace to the right
            while (*p == ' ') p++;
            // copy right side of chopped zone
            asmx_strcpy(word, p);
            // paste it at new linePtr
            asmx_strcpy(linePtr, word);
            // and linePtr now even points to where it should
        }
        // handle '\0' number of parameters operator
        else if (token == '\\' && *linePtr == '0')
        {
            p = linePtr + 1;    // skip '0'
            linePtr--;          // skip '\'
            // make string of number of parameters
            asmx_sprintf(word2, "%d", numMacParms[macLevel]);
            // copy right side of chopped zone
            asmx_strcpy(word, p);
            // paste number
            asmx_strcpy(linePtr, word2);
            linePtr = linePtr + asmx_strlen(word2);
            // paste right side at new linePtr
            asmx_strcpy(linePtr, word);
        }
        // handle '\n' parameter operator
        else if (token == '\\' && '1' <= *linePtr && *linePtr <= '9')
        {
            i = *linePtr - '1';
            p = linePtr + 1;    // skip 'n'
            linePtr--;          // skip '\'
            // copy right side of chopped zone
            asmx_strcpy(word, p);
            // paste parameter
            asmx_strcpy(linePtr, macParms[i + macLevel * MAXMACPARMS]);
            linePtr = linePtr + asmx_strlen(macParms[i + macLevel * MAXMACPARMS]);
            // paste right side at new linePtr
            asmx_strcpy(linePtr, word);
        }
        // handle '\?' unique ID operator
        else if (token == '\\' && *linePtr == '?')
        {
            p = linePtr + 1;    // skip '?'
            linePtr--;          // skip '\'
            // make string of number of parameters
            asmx_sprintf(word2, "%.5d", macCurrentID[macLevel]);
            // copy right side of chopped zone
            asmx_strcpy(word, p);
            // paste number
            asmx_strcpy(linePtr, word2);
            linePtr = linePtr + asmx_strlen(word2);
            // paste right side at new linePtr
            asmx_strcpy(linePtr, word);
        }
/* just use "\##" instead to avoid any confusion with \\ inside of DB pseudo-op
        // handle '\\' escape
        else if (token == '\\' && *linePtr == '\\')
        {
            p = linePtr + 1;    // skip second '\'
            // copy right side of chopped zone
            strcpy(word, p);
            // paste right side at new linePtr
            strcpy(linePtr, word);
        }
*/

        // skip initial whitespace
        c = *linePtr;
        while (c == 12 || c == '\t' || c == ' ')
            c = *++linePtr;

        p = linePtr;
        token = GetWord(word);
    }
}

/*
static void DumpMacro(MacroPtr p)
{
    MacroLinePtr    line_;
    MacroParmPtr    parm;

    if (cl_List)
    {
        asmx_fprintf(listing,"--- Macro '%s' ---", p -> name);
        asmx_fprintf(listing," def = %d, nparms = %d\n", p -> def, p -> nparms);

    //  dump parms here
        asmx_fprintf(listing,"Parms:");
        for (parm = p->parms; parm; parm = parm->next)
        {
            asmx_fprintf(listing," '%s'",parm->name);
        }
        asmx_fprintf(listing,"\n");

    //  dump text here
        for (line_ = p->text; line_; line_ = line_->next)
            asmx_fprintf(listing," '%s'\n",line_->text);
    }
}
*/

/*
static void DumpMacroTab(void)
{
    struct  MacroRec *p;

    p = macroTab;
    while (p)
    {
        DumpMacro(p);
        p = p -> next;
    }
}
*/


// --------------------------------------------------------------
// opcodes and symbol table


/*
 *  FindOpcodeTab - finds an entry in an opcode table
 */

// special compare for opcodes to allow "*" wildcard
static int opcode_strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == 0) return 0;
    if (*s1 == '*') return 0; // this is the magic
    return (*s1 - *(s2 - 1));
}


static asmx_OpcdPtr FindOpcodeTab(asmx_OpcdPtr p, char *name, int *typ, int *parm)
{
    bool found = FALSE;

//  while (p -> typ != o_Illegal && !found)
    while (*(p -> name) && !found)
    {
        found = (opcode_strcmp(p -> name, name) == 0);
        if (!found)
            p++;
        else
        {
            *typ  = p -> typ;
            *parm = p -> parm;
        }
    }

    if (!found) p = 0; // because this is an array, not a linked list
    return p;
}


/*
 *  FindOpcode - finds an opcode in either the generic or CPU-specific
 *               opcode tables, or as a macro name
 */

static asmx_OpcdPtr GetFindOpcode(char *opcode, int *typ, int *parm, MacroPtr *macro)
{
    *typ   = o_Illegal;
    *parm  = 0;
    *macro = 0;

    asmx_OpcdPtr p;
    int len;

    p = 0;
    if (GetOpcode(opcode))
    {
        if (opcdTab) p = FindOpcodeTab(opcdTab,  opcode, typ, parm);
        if (!p)
        {
            if (opcode[0] == '.') opcode++; // allow pseudo-ops to be invoked as ".OP"
            p = FindOpcodeTab((asmx_OpcdPtr) &opcdTab2, opcode, typ, parm);
        }
        if (p)
        {   // if wildcard was matched, back up linePtr
            // NOTE: if wildcard matched an empty string, linePtr will be
            //       unchanged and point to whitespace
            len = asmx_strlen(p->name);
            if (len && (p->name[len-1] == '*'))
            {
                linePtr = linePtr - (asmx_strlen(opcode) - len + 1);
            }
        }
        else
        {
            if ((*macro = FindMacro(opcode)))
            {
                *typ = o_MacName;
                p = opcdTab2; // return dummy non-null valid opcode pointer
            }
        }
    }
    /*
    if (pass == 2 && !strcmp(opcode,"FROB"))
    {
        printf("*** FROB typ=%d, parm=%d, macro=%.8X, p=%.8X\n",*typ,*parm,(int) macro,(int) p);
    }
    */

    return p;
}


/*
 *  FindSym
 */

static SymPtr FindSym(char *symName)
{
    SymPtr p = symTab;
    bool found = FALSE;

    while (p && !found)
    {
        found = (asmx_strcmp(p -> name, symName) == 0);
        if (!found)
            p = p -> next;
    }

    return p;
}


/*
 *  AddSym
 */

static SymPtr AddSym(char *symName)
{
    SymPtr p;

    p = asmx_malloc(sizeof *p + asmx_strlen(symName));

    asmx_strcpy(p -> name, symName);
    p -> value    = 0;
    p -> next     = symTab;
    p -> defined  = FALSE;
    p -> multiDef = FALSE;
    p -> isSet    = FALSE;
    p -> equ      = FALSE;
    p -> known    = FALSE;

    symTab = p;

    return p;
}



/*
 *  RefSym
 */

static int RefSym(char *symName, bool *known)
{
    SymPtr p;
    int i;
    asmx_Str255 s;

    if ((p = FindSym(symName)))
    {
        if (!p -> defined)
        {
            asmx_sprintf(s, "Symbol '%s' undefined", symName);
            Error(s);
        }
        switch(pass)
        {
            case 1:
                if (!p -> defined) *known = FALSE;
                break;
            case 2:
                if (!p -> known) *known = FALSE;
                break;
        }
#if 0 // FIXME: possible fix that may be needed for 16-bit address
        if (addrWid == ADDR_16)
            return (short) p -> value;    // sign-extend from 16 bits
#endif
        return p -> value;
    }

    {   // check for 'FFH' style constants here

        i = asmx_strlen(symName)-1;
        if (asmx_toupper(symName[i]) != 'H')
            i = -1;
        else
            while (i>0 && ishex(symName[i-1]))
                i--;

        if (i == 0)
        {
            asmx_strncpy(s, symName, 255);
            s[asmx_strlen(s)-1] = 0;
            return EvalHex(s);
        }
        else
        {
            p = AddSym(symName);
            *known = FALSE;
//          sprintf(s, "Symbol '%s' undefined", symName);
//          Error(s);
        }
    }

    return 0;
}


/*
 *  DefSym
 */

void DefSym(char *symName, uint32_t val, bool setSym, bool equSym)
{
    SymPtr p;
    asmx_Str255 s;

    if (symName[0]) // ignore null string symName
    {
        p = FindSym(symName);
        if (p == 0)
            p = AddSym(symName);

        if (!p -> defined || (p -> isSet && setSym))
        {
            p -> value = val;
            p -> defined = TRUE;
            p -> isSet = setSym;
            p -> equ = equSym;
        }
        else if (p -> value != val)
        {
            p -> multiDef = TRUE;
            if (pass == 2 && !p -> known) {
                asmx_sprintf(s, "Phase error");
            }
            else {
                asmx_sprintf(s, "Symbol '%s' multiply defined",symName);
            }
            Error(s);
        }

        if (pass == 0 || pass == 2) p -> known = TRUE;
    }
}


static void DumpSym(SymPtr p, char *s, int *w)
{
//
// #######
//
    char *s2;
    int n,len,max;

    n = 0;
    *w = 1;
    max = MAXSYMLEN;

    s2 = p->name;
    len = asmx_strlen(s2);

    while (max-1 < len)
    {
        *w = *w + 1;
        max = max + MAXSYMLEN + 8; // 8 = number of extra chars between symbol names
    }

    while(*s2 && n < max)
    {
        *s++ = *s2++;
        n++;
    }

    while (n < max)
    {
        *s++ = ' ';
        n++;
    }

    switch(addrMax)
    {
        default:
        case ASMX_ADDR_16:
            asmx_sprintf(s, "%.4lX ", p->value & 0xFFFF);
            break;

        case ASMX_ADDR_24:
#if 0
            asmx_sprintf(s, "%.6lX ", p->value & 0xFFFFFF);
            break;
#endif
        case ASMX_ADDR_32:
            asmx_sprintf(s, "%.8lX ", p->value);
            break;
    }

    s = s + asmx_strlen(s);

    n = 0;
    if (!p -> defined)    {*s++ = 'U'; n++;}  // Undefined
    if ( p -> multiDef)   {*s++ = 'M'; n++;}  // Multiply defined
    if ( p -> isSet)      {*s++ = 'S'; n++;}  // Set
    if ( p -> equ)        {*s++ = 'E'; n++;}  // Equ
    while (n < 3)
    {
        *s++ = ' ';
        n++;
    }
    *s = 0;
}


static void DumpSymTab(void)
{
    struct  SymRec *p;
    int     i,w;
    asmx_Str255  s;

    i = 0;
    p = symTab;
    while (p)
    {
#ifdef TEMP_LBLAT
        if (tempSymFlag || !(strchr(p->name, '.') || strchr(p->name, '@')))
#else
        if (tempSymFlag || !asmx_strchr(p->name, '.'))
#endif
        {
            DumpSym(p,s,&w);
            p = p -> next;

            // force a newline if new symbol won't fit on current line
            if (i+w > symTabCols)
            {
                if (cl_List)
                    asmx_fprintf(listing, "\n");
                i = 0;
            }
            // if last symbol or if symbol fills line, deblank and print it
            if (p == 0 || i+w >= symTabCols)
            {
                Debright(s);
                if (cl_List)
                    asmx_fprintf(listing, "%s\n", s);
                i = 0;
            }
            // otherwise just print it and count its width
            else
            {
                if (cl_List)
                    asmx_fprintf(listing, "%s\n", s);
                i = i + w;
            }
        }
        else p = p -> next;
    }
}


static void SortSymTab()
{
    SymPtr          i,j;    // pointers to current elements
    SymPtr          ip,jp;  // pointers to previous elements
    SymPtr          t;      // temp for swapping

    // yes, it's a linked-list bubble sort

    if (symTab != 0)
    {
        ip = 0;  i = symTab;
        jp = i;     j = i -> next;

        while (j != 0)
        {
            while (j != 0)
            {
                if (asmx_strcmp(i->name,j->name) > 0)    // (i->name > j->name)
                {
                    if (ip != 0) ip -> next = j;
                               else symTab     = j;
                    if (i == jp)
                    {
                        i -> next = j -> next;
                        j -> next = i;
                    }
                    else
                    {
                        jp -> next = i;

                        t         = i -> next;
                        i -> next = j -> next;
                        j -> next = t;
                    }
                    t = i; i = j; j = t;
                }
                jp = j;     j = j -> next;
            }
            ip = i;     i = i -> next;
            jp = i;     j = i -> next;
        }
    }
}


// --------------------------------------------------------------
// expression evaluation


static int Eval0(void);        // forward declaration


static int Factor(void)
{
    asmx_Str255      word,s;
    int         token;
    int         val;
    char        *oldLine;
    SymPtr      p;

    token = GetWord(word);
    val = 0;

    switch(token)
    {
        case 0:
            MissingOperand();
            break;

        case '%':
            GetWord(word);
            val = EvalBin(word);
            break;

        case '$':
            if (ishex(*linePtr))
            {
                GetWord(word);
                val = EvalHex(word);
                break;
            }
            // fall-through...
        case '*':
            val = locPtr;
#if 0 // FIXME: possible fix that may be needed for 16-bit address
        if (addrWid == ADDR_16)
            val = (short) val;            // sign-extend from 16 bits
#endif
            val = val / wordDiv;
            break;

        case '+':
            val = Factor();
            break;

        case '-':
            val = -Factor();
            break;

        case '~':
            val = ~Factor();
            break;

        case '!':
            val = !Factor();
            break;

        case '<':
            val = Factor() & 0xFF;
            break;

        case '>':
            val = (Factor() >> 8) & 0xFF;
            break;

        case '(':
            val = Eval0();
            RParen();
            break;

        case '[':
            val = Eval0();
            Expect("]");
            break;

        case 0x27:  // single quote
#if 1 // enable multi-char single-quote constants
            val = 0;
            while (*linePtr != 0x27 && *linePtr != 0)
            {
                val = val * 256 + GetBackslashChar();
            }
            if (*linePtr == 0x27)
                linePtr++;
            else
                Error("Missing close quote");
#else
            if ((val = GetBackslashChar()) >= 0)
            {
                if (*linePtr && *linePtr != 0x27)
                    val = val * 256 + GetBackslashChar();
                if (*linePtr == 0x27)
                    linePtr++;
                else
                    Error("Missing close quote");
            }
            else MissingOperand();
#endif
            break;

        case '.':
            // check for ".."
            oldLine = linePtr;
            val = GetWord(word);
            if (val == '.')
            {
                GetWord(word);
                // check for "..DEF" operator
                if (asmx_strcmp(word,"DEF") == 0)
                {
                    val = 0;
                    if (GetWord(word) == -1)
                    {
                        p = FindSym(word);
                        val = (p && (p -> known || pass == 1));
                    }
                    else IllegalOperand();
                    break;
                }

                // check for "..UNDEF" operator
                if (asmx_strcmp(word,"UNDEF") == 0)
                {
                    val = 0;
                    if (GetWord(word) == -1)
                    {
                        p = FindSym(word);
                        val = !(p && (p -> known || pass == 1));
                    }
                    else IllegalOperand();
                    break;
                }

                // invalid ".." operator
                // rewind and return "current location"
                linePtr = oldLine;
                break;
            }

            // check for '.' as "current location"
            else if (val != -1)
            {
                linePtr = oldLine;
                val = locPtr;
#if 0 // FIXME: possible fix that may be needed for 16-bit address
                if (addrWid == ADDR_16)
                    val = (short) val;    // sign-extend from 16 bits
#endif
                val = val / wordDiv;
                break;
            }

            // now try it as a local ".symbol"
            linePtr = oldLine;
            // fall-through...
#ifdef TEMP_LBLAT
        case '@':
#endif
            GetWord(word);
            if (token == '.' && subrLabl[0])    asmx_strcpy(s,subrLabl);
                                        else    asmx_strcpy(s,lastLabl);
            s[asmx_strlen(s)+1] = 0;
            s[asmx_strlen(s)]   = token;
            asmx_strcat(s,word);
            val = RefSym(s, &evalKnown);
            break;

        case -1:
            if ((word[0] == 'H' || word[0] == 'L') && word[1] == 0 && *linePtr == '(')
            {   // handle H() and L() from vintage Atari 7800 source code
                // note: no whitespace allowed before the open paren!
                token = word[0];    // save 'H' or 'L'
                GetWord(word);      // eat left paren
                val = Eval0();      // evaluate sub-expression
                RParen();           // check for right paren
                if (token == 'H') val = (val >> 8) & 0xFF;
                if (token == 'L') val = val & 0xFF;
                break;
            }
            if (asmx_isdigit(word[0]))   val = EvalNum(word);
                            else    val = RefSym(word,&evalKnown);
            break;

        default:
            MissingOperand();
            break;
    }

    return val;
}


static int Term(void)
{
    asmx_Str255  word;
    int     token;
    int     val,val2;
    char    *oldLine;

    val = Factor();

    oldLine = linePtr;
    token = GetWord(word);
    while (token == '*' || token == '/' || token == '%')
    {
        switch(token)
        {
            case '*':   val = val * Factor();   break;
            case '/':   val2 = Factor();
                        if (val2)
                            val = val / val2;
                        else
                        {
                            Warning("Division by zero");
                            val = 0;
                        }
                        break;
            case '%':   val2 = Factor();
                        if (val2)
                            val = val % val2;
                        else
                        {
                            Warning("Division by zero");
                            val = 0;
                        }
                        break;
        }
        oldLine = linePtr;
        token = GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


static int Eval2(void)
{
    asmx_Str255  word;
    int     token;
    int     val;
    char    *oldLine;

    val = Term();

    oldLine = linePtr;
    token = GetWord(word);
    while (token == '+' || token == '-')
    {
        switch(token)
        {
            case '+':   val = val + Term();     break;
            case '-':   val = val - Term();     break;
        }
        oldLine = linePtr;
        token = GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


static int Eval1(void)
{
    asmx_Str255  word;
    int     token;
    int     val;
    char    *oldLine;

    val = Eval2();

    oldLine = linePtr;
    token = GetWord(word);
    while ((token == '<' && *linePtr != token) ||
            (token == '>' && *linePtr != token) ||
            token == '=' ||
            (token == '!' && *linePtr == '='))
    {
        switch(token)
        {
            case '<':   if (*linePtr == '=')    {linePtr++; val = (val <= Eval2());}
                        else                    val = (val <  Eval2());
                        break;
            case '>':   if (*linePtr == '=')    {linePtr++; val = (val >= Eval2());}
                        else                    val = (val >  Eval2());
                        break;
            case '=':   if (*linePtr == '=') linePtr++; // allow either one or two '=' signs
                        val = (val == Eval2());     break;
            case '!':   linePtr++;  val = (val != Eval2());     break;
        }
        oldLine = linePtr;
        token = GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


static int Eval0(void)
{
    asmx_Str255  word;
    int     token;
    int     val;
    char    *oldLine;

    val = Eval1();

    oldLine = linePtr;
    token = GetWord(word);
    while (token == '&' || token == '|' || token == '^' ||
            (token == '<' && *linePtr == '<') ||
            (token == '>' && *linePtr == '>'))
    {
        switch(token)
        {
            case '&':   if (*linePtr == '&') {linePtr++; val = ((val & Eval1()) != 0);}
                                        else             val =   val & Eval1();
                        break;
            case '|':   if (*linePtr == '|') {linePtr++; val = ((val | Eval1()) != 0);}
                                        else             val =   val | Eval1();
                        break;
            case '^':   val = val ^ Eval1();
                        break;
            case '<':   linePtr++;  val = val << Eval1();   break;
            case '>':   linePtr++;  val = val >> Eval1();   break;
        }
        oldLine = linePtr;
        token = GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


int Eval(void)
{
    evalKnown = TRUE;

    return Eval0();
}


static void CheckByte(int val)
{
    if (!errFlag && (val < -128 || val > 255))
        Warning("Byte out of range");
}


/*
static void CheckStrictByte(int val)
{
    if (!errFlag && (val < -128 || val > 127))
        Warning("Byte out of range");
}


static void CheckWord(int val)
{
    if (!errFlag && (val < -32768 || val > 65535))
        Warning("Word out of range");
}


static void CheckStrictWord(int val)
{
    if (!errFlag && (val < -32768 || val > 32767))
        Warning("Word out of range");
}
*/

static int EvalByte(void)
{
    long val;

    val = Eval();
    CheckByte(val);

    return val & 0xFF;
}


int EvalBranch(int iLen)
{
    long val;

    val = Eval();
    val = val - locPtr - iLen;
    if (!errFlag && (val < -128 || val > 127))
        Error("Short branch out of range");

    return val & 0xFF;
}


int EvalWBranch(int iLen)
{
    long val;

    val = Eval();
    val = val - locPtr - iLen;
    if (!errFlag && (val < -32768 || val > 32767))
        Error("Word branch out of range");
    return val;
}

/*
static int EvalLBranch(int iLen)
{
    long val;

    val = Eval();
    val = val - locPtr - iLen;
    return val;
}
*/

// --------------------------------------------------------------
// object file generation

// record types -- note that 0 and 1 are used directly for .hex
enum
{
    REC_DATA = 0,   // data record
    REC_XFER = 1,   // transfer address record
    REC_HEDR = 2,   // header record (must be sent before start of data)
#ifdef CODE_COMMENTS
    REC_CMNT = 3    // comment record
#endif // CODE_COMMENTS
};
static uint8_t   hex_buf[IHEX_SIZE]; // buffer for current line of object data
static uint32_t  hex_len;            // current size of object data buffer
static uint32_t  hex_base;           // address of start of object data buffer
static uint32_t  hex_addr;           // address of next byte in object data buffer
static uint16_t  hex_page;           // high word of address for intel hex file
static uint32_t  bin_eof;            // current end of file when writing binary file

// Intel hex format:
//
// :aabbbbccdddd...ddee
//
// aa    = record data length (the number of dd bytes)
// bbbb  = address for this record
// cc    = record type
//       00 = data (data is in the dd bytes)
//       01 = end of file (bbbb is transfer address)
//       02 = extended segment address record
//            dddd (two bytes) represents the segment address
//       03 = Start segment address record
//            dddd (two bytes) represents the segment of the transfer address
//       04 = extended linear address record
//            dddd (two bytes) represents the high address word
//       05 = Start linear address record
//            dddd (two bytes) represents the high word of the transfer address
// dd... = data bytes if record type needs it
// ee    = checksum byte: add all bytes aa through dd
//                        and subtract from 256 (2's complement negate)

static void write_ihex(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    int i,chksum;

    if (rectype > REC_XFER) return;

    // if transfer record with long address, write extended address record
    if (rectype == REC_XFER && (addr & 0xFFFF0000))
        write_ihex(addr >> 16, buf, 0, 5);

    // if data record with long address, write extended address record
    if (rectype == REC_DATA && (addr >> 16) != hex_page)
    {
        write_ihex(addr >> 16, buf, 0, 4);
        hex_page = addr >> 16;
    }

    // compute initial checksum from length, address, and record type
    chksum = len + (addr >> 8) + addr + rectype;

    // print length, address, and record type
    asmx_fprintf(object,":%.2lX%.4lX%.2X", len, addr & 0xFFFF, rectype);

    // print data while updating checksum
    for (i=0; i<(int)len; i++)
    {
        asmx_fprintf(object, "%.2X", buf[i]);
        chksum = chksum + buf[i];
    }

    // print final checksum
    asmx_fprintf(object, "%.2X\n", (-chksum) & 0xFF);
}


static void write_bin(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    uint32_t i;

    if (rectype == REC_DATA)
    {
        // automatically adjust base end end?
        if (cl_AutoBinBaseEnd) {
            if (cl_Binbase > addr) {
                cl_Binbase = addr;
            }
            if (cl_Binend < (addr + len)) {
                cl_Binend = addr + len;
            }
        }

        // return if end of data less than base address
        if (addr + len <= cl_Binbase) return;

        // return if start of data greater than end address
        if (addr > cl_Binend) return;

        // if data crosses base address, adjust start of data
        if (addr < cl_Binbase)
        {
            buf = buf + cl_Binbase - addr;
            addr = cl_Binbase;
        }

        // if data crossses end address, adjust length of data
        if (addr+len-1 > cl_Binend)
            len = cl_Binend - addr + 1;

        // if addr is beyond current EOF, write (addr-bin_eof) bytes of 0xFF padding
        if (addr - cl_Binbase > bin_eof)
        {
            asmx_fseek(object, bin_eof, ASMX_SEEK_SET);
            for (i=0; i < addr - cl_Binbase - bin_eof; i++)
                asmx_fputc(0xFF, object);
        }

        // seek to addr and write buf
        asmx_fseek(object, addr - cl_Binbase, ASMX_SEEK_SET);
        asmx_fwrite(buf, 1, len, object);

        // update EOF of object file
        i = asmx_ftell(object);
        if (i > bin_eof)
            bin_eof = i;

        //fflush(object);
    }
}


// rectype 0 = code, rectype 1 = xfer
static void write_hex(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    if (cl_Obj)
    {
        switch(cl_ObjType)
        {
            default:
            case OBJ_HEX:    write_ihex  (addr, buf, len, rectype); break;
            case OBJ_BIN:    write_bin   (addr, buf, len, rectype); break;
        }
    }
}


static void CodeInit(void)
{
    hex_len  = 0;
    hex_base = 0;
    hex_addr = 0;
    hex_page = 0;
    bin_eof  = 0;
}


static void CodeFlush(void)
{
    if (hex_len)
    {
        write_hex(hex_base, hex_buf, hex_len, REC_DATA);
        hex_len  = 0;
        hex_base = hex_base + hex_len;
        hex_addr = hex_base;
    }
}


static void CodeOut(int byte)
{
    if (pass == 2)
    {
        if (codPtr != hex_addr)
        {
            CodeFlush();
            hex_base = codPtr;
            hex_addr = codPtr;
        }
        hex_buf[hex_len++] = byte;
        hex_addr++;
        if (hex_len == IHEX_SIZE)
            CodeFlush();
    }
    locPtr++;
    codPtr++;
}


static void CodeHeader(char *s)
{
    CodeFlush();

    write_hex(0, (uint8_t *) s, asmx_strlen(s), REC_HEDR);
}


#ifdef CODE_COMMENTS
static void CodeComment(char *s)
{
    CodeFlush();

    write_hex(0, (uint8_t *) s, strlen(s), REC_CMNT);
}
#endif // CODE_COMMENTS


static void CodeEnd(void)
{
    CodeFlush();

    if (pass == 2)
    {
        if (xferFound)
            write_hex(xferAddr, hex_buf, 0, REC_XFER);
    }
}


static void CodeAbsOrg(int addr)
{
    codPtr = addr;
    locPtr = addr;
}


static void CodeRelOrg(int addr)
{
    locPtr = addr;
}


static void CodeXfer(int addr)
{
    xferAddr  = addr;
    xferFound = TRUE;
}


static void AddLocPtr(int ofs)
{
    codPtr = codPtr + ofs;
    locPtr = locPtr + ofs;
}


// --------------------------------------------------------------
// instruction format calls

// clear the length of the instruction
static void InstrClear(void)
{
    instrLen = 0;
    hexSpaces = 0;
}
void asmx_InstrClear(void) {
    InstrClear();
}


// add a byte to the instruction
static void InstrAddB(uint8_t b)
{
    bytStr[instrLen++] = b;
    hexSpaces |= 1<<instrLen;
}
void asmx_InstrAddB(uint8_t b) {
    InstrAddB(b);
}


// add a byte/word opcode to the instruction
// a big-endian word is added if the opcode is > 255
// this is for opcodes with pre-bytes
static void InstrAddX(uint32_t op)
{
//  if ((op & 0xFFFFFF00) == 0) hexSpaces |= 1; // to indent single-byte instructions
//  if (op & 0xFF000000) bytStr[instrLen++] = op >> 24;
//  if (op & 0xFFFF0000) bytStr[instrLen++] = op >> 16;
    if (op & 0xFFFFFF00) bytStr[instrLen++] = op >>  8;
    bytStr[instrLen++] = op & 255;
    hexSpaces |= 1<<instrLen;
}

// add a word to the instruction in the CPU's endianness
static void InstrAddW(uint16_t w)
{
    if (curEndian == ASMX_LITTLE_END)
    {
        bytStr[instrLen++] = w & 255;
        bytStr[instrLen++] = w >> 8;
    }
    else if (curEndian == ASMX_BIG_END)
    {
        bytStr[instrLen++] = w >> 8;
        bytStr[instrLen++] = w & 255;
    }
    else Error("CPU endian not defined");
    hexSpaces |= 1<<instrLen;
}


// add a 3-byte word to the instruction in the CPU's endianness
static void InstrAdd3(uint32_t l)
{
    if (curEndian == ASMX_LITTLE_END)
    {
        bytStr[instrLen++] =  l        & 255;
        bytStr[instrLen++] = (l >>  8) & 255;
        bytStr[instrLen++] = (l >> 16) & 255;
    }
    else if (curEndian == ASMX_BIG_END)
    {
        bytStr[instrLen++] = (l >> 16) & 255;
        bytStr[instrLen++] = (l >>  8) & 255;
        bytStr[instrLen++] =  l        & 255;
    }
    else Error("CPU endian not defined");
    hexSpaces |= 1<<instrLen;
}
void asmx_InstrAdd3(uint32_t l) {
    InstrAdd3(l);
}


// add a longword to the instruction in the CPU's endianness
static void InstrAddL(uint32_t l)
{
    if (curEndian == ASMX_LITTLE_END)
    {
        bytStr[instrLen++] =  l        & 255;
        bytStr[instrLen++] = (l >>  8) & 255;
        bytStr[instrLen++] = (l >> 16) & 255;
        bytStr[instrLen++] = (l >> 24) & 255;
    }
    else if (curEndian == ASMX_BIG_END)
    {
        bytStr[instrLen++] = (l >> 24) & 255;
        bytStr[instrLen++] = (l >> 16) & 255;
        bytStr[instrLen++] = (l >>  8) & 255;
        bytStr[instrLen++] =  l        & 255;
    }
    else Error("CPU endian not defined");
    hexSpaces |= 1<<instrLen;
}

static void InstrB(uint8_t b1)
{
    InstrClear();
    InstrAddB(b1);
}
void asmx_InstrB(uint8_t b1) {
    InstrB(b1);
}


void InstrBB(uint8_t b1, uint8_t b2)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddB(b2);
}

void InstrBBB(uint8_t b1, uint8_t b2, uint8_t b3)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddB(b3);
}

void InstrBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddB(b3);
    InstrAddB(b4);
}


void InstrBBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddB(b3);
    InstrAddB(b4);
    InstrAddB(b5);
}

void InstrBW(uint8_t b1, uint16_t w1)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddW(w1);
}

void InstrBBW(uint8_t b1, uint8_t b2, uint16_t w1)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddW(w1);
}

void InstrBBBW(uint8_t b1, uint8_t b2, uint8_t b3, uint16_t w1)
{
    InstrClear();
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddB(b3);
    InstrAddW(w1);
}


void InstrX(uint32_t op)
{
    InstrClear();
    InstrAddX(op);
}


void InstrXB(uint32_t op, uint8_t b1)
{
    InstrClear();
    InstrAddX(op);
    InstrAddB(b1);
}


void InstrXBB(uint32_t op, uint8_t b1, uint8_t b2)
{
    InstrClear();
    InstrAddX(op);
    InstrAddB(b1);
    InstrAddB(b2);
}


void InstrXBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3)
{
    InstrClear();
    InstrAddX(op);
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddB(b3);
}


void InstrXBBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    InstrClear();
    InstrAddX(op);
    InstrAddB(b1);
    InstrAddB(b2);
    InstrAddB(b3);
    InstrAddB(b4);
}


void InstrXW(uint32_t op, uint16_t w1)
{
    InstrClear();
    InstrAddX(op);
    InstrAddW(w1);
}


void InstrXBW(uint32_t op, uint8_t b1, uint16_t w1)
{
    InstrClear();
    InstrAddX(op);
    InstrAddB(b1);
    InstrAddW(w1);
}


void InstrXBWB(uint32_t op, uint8_t b1, uint16_t w1, uint8_t b2)
{
    InstrClear();
    InstrAddX(op);
    InstrAddB(b1);
    InstrAddW(w1);
    InstrAddB(b2);
}


void InstrXWW(uint32_t op, uint16_t w1, uint16_t w2)
{
    InstrClear();
    InstrAddX(op);
    InstrAddW(w1);
    InstrAddW(w2);
}


void InstrX3(uint32_t op, uint32_t l1)
{
    InstrClear();
    InstrAddX(op);
    InstrAdd3(l1);
}


void InstrW(uint16_t w1)
{
    InstrClear();
    InstrAddW(w1);
}

void InstrWW(uint16_t w1, uint16_t w2)
{
    InstrClear();
    InstrAddW(w1);
    InstrAddW(w2);
}

void InstrWL(uint16_t w1, uint32_t l1)
{
    InstrClear();
    InstrAddW(w1);
    InstrAddL(l1);
}


void InstrL(uint32_t l1)
{
    InstrClear();
    InstrAddL(l1);
}


void InstrLL(uint32_t l1, uint32_t l2)
{
    InstrClear();
    InstrAddL(l1);
    InstrAddL(l2);
}

// --------------------------------------------------------------
// segment handling


static SegPtr FindSeg(char *name)
{
    SegPtr p = segTab;
    bool found = FALSE;

    while (p && !found)
    {
        found = (asmx_strcmp(p -> name, name) == 0);
        if (!found)
            p = p -> next;
    }

    return p;
}


static SegPtr AddSeg(char *name)
{
    SegPtr  p;

    p = asmx_malloc(sizeof *p + asmx_strlen(name));

    p -> next = segTab;
//  p -> gen = TRUE;
    p -> loc = 0;
    p -> cod = 0;
    asmx_strcpy(p -> name, name);

    segTab = p;

    return p;
}


static void SwitchSeg(SegPtr seg)
{
    CodeFlush();
    curSeg -> cod = codPtr;
    curSeg -> loc = locPtr;

    curSeg = seg;
    codPtr = curSeg -> cod;
    locPtr = curSeg -> loc;
}


// --------------------------------------------------------------
// text I/O


static int OpenInclude(char *fname)
{
    if (nInclude == MAX_INCLUDE - 1)
        return -1;

    nInclude++;
    include[nInclude] = 0;
    incline[nInclude] = 0;
    asmx_strcpy(incname[nInclude],fname);
    include[nInclude] = asmx_fopen(fname, "r");
    if (include[nInclude])
        return 1;

    nInclude--;
    return 0;
}


static void CloseInclude(void)
{
    if (nInclude < 0)
        return;

    asmx_fclose(include[nInclude]);
    include[nInclude] = 0;
    nInclude--;
}


static int ReadLine(asmx_FILE *file, char *line_, int max)
{
    int c = 0;
    int len = 0;

    macLineFlag = TRUE;

    // if at end of macro and inside a nested macro, pop the stack
    while (macLevel > 0 && macLine[macLevel] == 0)
    {
#ifdef ENABLE_REP
        if (macRepeat[macLevel] > 0)
        {
            if (macRepeat[macLevel]--)
                macLine = macPtr[macLevel] -> text;
            else
            {
                asmx_free(macPtr[macLevel]);
                macLevel--;
            }
        }
        else
#endif
        macLevel--;
    }

    // if there is still another macro line to process, get it
    if (macLine[macLevel] != 0)
    {
        asmx_strcpy(line_, macLine[macLevel] -> text);
        macLine[macLevel] = macLine[macLevel] -> next;
        DoMacParms();
    }
    else
    {   // else we weren't in a macro or we just ran out of macro
        macLineFlag = FALSE;

        if (nInclude >= 0)
            incline[nInclude]++;
        else
            linenum++;

        macPtr[macLevel] = 0;

        while (max > 1)
        {
            c = asmx_fgetc(file);
            *line_ = 0;
            switch(c)
            {
                case ASMX_EOF:
                    if (len == 0) return 0;
                case '\n':
                    return 1;
                case '\r':
                    c = asmx_fgetc(file);
                    if (c != '\n')
                    {
                        asmx_ungetc(c,file);
                        c = '\r';
                    }
                    return 1;
                default:
                    *line_++ = c;
                    max--;
                    len++;
                    break;
            }
        }
        while (c != ASMX_EOF && c != '\n')
            c = asmx_fgetc(file);
    }
    return 1;
}


static int ReadSourceLine(char *line_, int max)
{
    int i;

    while (nInclude >= 0)
    {
        i = ReadLine(include[nInclude], line_, max);
        if (i) return i;

        CloseInclude();
    }

    return ReadLine(source, line_, max);
}


static void ListOut(bool showStdErr)
{
/* uncomment this block if you want form feeds to be sent to the listing file
    if (listLineFF && cl_List)
        asmx_fputc(12,listing);
*/
    Debright(listLine);

    if (cl_List)
        asmx_fprintf(listing,"%s\n",listLine);

    if (pass == 2 && showStdErr && ((errFlag && cl_Err) || (warnFlag && cl_Warn)))
        asmx_fprintf(asmx_stderr,"%s\n",listLine);
}


static void CopyListLine(void)
{
    int n;
    char c,*p,*q;

    p = listLine;
    q = curLine;
    listLineFF = FALSE;

    // the old version
//  strcpy(listLine, "                ");       // 16 blanks
//  strncat(listLine, line, 255-16);

    if (curListWid == ASMX_LIST_24)
        for (n=24; n; n--) *p++ = ' ';  // 24 blanks at start of line
    else
        for (n=16; n; n--) *p++ = ' ';  // 16 blanks at start of line

    while (n < 255-16 && (c = *q++))    // copy rest of line, stripping out form feeds
    {
        if (c == 12) listLineFF = TRUE; // if a form feed was found, remember it for later
        else
        {
            *p++ = c;
            n++;
        }
    }
    *p = 0;   // string terminator
}


// --------------------------------------------------------------
// main assembler loops


static void DoOpcode(int typ, int parm)
{
    int             val;
    int             i,n;
    asmx_Str255     word,s;
    char            *oldLine;
    int             token;
    int             ch;
    uint8_t          quote;
    char            *p;

    if (DoCPUOpcode(typ, parm)) return;

    switch(typ)
    {
        case o_ZSCII:
        case o_ASCIIC:
        case o_ASCIIZ:
        case o_DB:
            instrLen = 0;
            oldLine = linePtr;
            token = GetWord(word);

            if (token == 0)
               MissingOperand();

            if (typ == o_ASCIIC)
                bytStr[instrLen++] = 0;

            while (token)
            {
                if ((token == '\'' && *linePtr && linePtr[1] != '\'') || token == '"')
                {
                    quote = token;
                    while (token == quote)
                    {
                        while (*linePtr != 0 && *linePtr != token)
                        {
                            ch = GetBackslashChar();
                            if ((ch >= 0) && (instrLen < ASMX_MAX_BYTSTR))
                                bytStr[instrLen++] = ch;
                        }
                        token = *linePtr;
                        if (token)  linePtr++;
                            else    Error("Missing close quote");
                        if (token == quote && *linePtr == quote)    // two quotes together
                        {
                            if (instrLen < ASMX_MAX_BYTSTR)
                                bytStr[instrLen++] = *linePtr++;
                        }
                        else
                            token = *linePtr;
                    }
                }
                else
                {
                    linePtr = oldLine;
                    val = EvalByte();
                    if (instrLen < ASMX_MAX_BYTSTR)
                        bytStr[instrLen++] = val;
                }

                token = GetWord(word);
                oldLine = linePtr;

                if (token == ',')
                {
                    token = GetWord(word);
                    if (token == 0)
                        MissingOperand();
                }
                else if (token)
                {
                    linePtr = oldLine;
                    Comma();
                    token = 0;
                }
                else if (errFlag)   // this is necessary to keep EvalByte() errors
                    token = 0;      // from causing phase errors
            }

            if (instrLen >= ASMX_MAX_BYTSTR || (typ == o_ASCIIC && instrLen > 256))
            {
                Error("String too long");
                instrLen = ASMX_MAX_BYTSTR;
            }

            switch(typ)
            {
                case o_ASCIIC:
                    if (instrLen > 255) bytStr[0] = 255;
                                   else bytStr[0] = instrLen-1;
                    break;

                case o_ZSCII:
                    ConvertZSCII();
                    break;

                case o_ASCIIZ:
                    if (instrLen < ASMX_MAX_BYTSTR)
                        bytStr[instrLen++] = 0;
                    break;

                default:
                    break;
            }

            instrLen = -instrLen;
            break;

        case o_DW:
        case o_DWRE:    // reverse-endian DW
            if (curEndian != ASMX_LITTLE_END && curEndian != ASMX_BIG_END)
            {
                Error("CPU endian not defined");
                break;
            }

            instrLen = 0;
            oldLine = linePtr;
            token = GetWord(word);

            if (token == 0)
               MissingOperand();

            while (token)
            {
#if 1 // enable padded string literals
                if ((token == '\'' && *linePtr && linePtr[1] != '\'') || token == '"')
                {
                    n = 0;
                    quote = token;
                    while (token == quote)
                    {
                        while (*linePtr != 0 && *linePtr != token)
                        {
                            ch = GetBackslashChar();
                            if ((ch >= 0) && (instrLen < ASMX_MAX_BYTSTR))
                            {
                                bytStr[instrLen++] = ch;
                                n++;
                            }
                        }
                        token = *linePtr;
                        if (token)  linePtr++;
                            else    Error("Missing close quote");
                        if (token == quote && *linePtr == quote)    // two quotes together
                        {
                            if (instrLen < ASMX_MAX_BYTSTR)
                            {
                                bytStr[instrLen++] = *linePtr++;
                                n++;
                            }
                        }
                        else
                            token = *linePtr;
                    }
                    // add padding nulls here
                    if ((n & 1) && instrLen < ASMX_MAX_BYTSTR)
                        bytStr[instrLen++] = 0;
                }
                else
#endif
                {
                    linePtr = oldLine;
                    val = Eval();
                    if ((curEndian == ASMX_LITTLE_END) ^ (typ == o_DWRE))
                    {   // little endian
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 8;
                    }
                    else
                    {   // big endian
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 8;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val;
                    }
                }

                token = GetWord(word);
                oldLine = linePtr;

                if (token == ',')
                {
                    token = GetWord(word);
                    if (token == 0)
                        MissingOperand();
                }
                else if (token)
                {
                    linePtr = oldLine;
                    Comma();
                    token = 0;
                }
            }

            if (instrLen >= ASMX_MAX_BYTSTR)
            {
                Error("String too long");
                instrLen = ASMX_MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case o_DL:
            if (curEndian != ASMX_LITTLE_END && curEndian != ASMX_BIG_END)
            {
                Error("CPU endian not defined");
                break;
            }

            instrLen = 0;
            oldLine = linePtr;
            token = GetWord(word);

            if (token == 0)
               MissingOperand();

            while (token)
            {
#if 1 // enable padded string literals
                if ((token == '\'' && *linePtr && linePtr[1] != '\'') || token == '"')
                {
                    n = 0;
                    quote = token;
                    while (token == quote)
                    {
                        while (*linePtr != 0 && *linePtr != token)
                        {
                            ch = GetBackslashChar();
                            if ((ch >= 0) && (instrLen < ASMX_MAX_BYTSTR))
                            {
                                bytStr[instrLen++] = ch;
                                n++;
                            }
                        }
                        token = *linePtr;
                        if (token)  linePtr++;
                            else    Error("Missing close quote");
                        if (token == quote && *linePtr == quote)    // two quotes together
                        {
                            if (instrLen < ASMX_MAX_BYTSTR)
                            {
                                bytStr[instrLen++] = *linePtr++;
                                n++;
                            }
                        }
                        else
                            token = *linePtr;
                    }
                    // add padding nulls here
                    while ((n & 3) && instrLen < ASMX_MAX_BYTSTR)
                    {
                        bytStr[instrLen++] = 0;
                        n++;
                    }
                }
                else
#endif
                {
                    linePtr = oldLine;
                    val = Eval();
                    if ((curEndian == ASMX_LITTLE_END) ^ (typ == o_DWRE))
                    {   // little endian
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 8;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 16;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 24;
                    }
                    else
                    {   // big endian
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 24;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 16;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val >> 8;
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val;
                    }
                }

                token = GetWord(word);
                oldLine = linePtr;

                if (token == ',')
                {
                    token = GetWord(word);
                    if (token == 0)
                        MissingOperand();
                }
                else if (token)
                {
                    linePtr = oldLine;
                    Comma();
                    token = 0;
                }
            }

            if (instrLen >= ASMX_MAX_BYTSTR)
            {
                Error("String too long");
                instrLen = ASMX_MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case o_DS:
            val = Eval();

            if (!evalKnown)
            {
                Error("Can't use DS pseudo-op with forward-declared length");
                break;
            }

            oldLine = linePtr;
            token = GetWord(word);
            if (token == ',')
            {
                if (parm == 1)
                    n = EvalByte();
                else
                    n = Eval();

                if (val*parm > ASMX_MAX_BYTSTR)
                {
                    asmx_sprintf(s,"String too long (max %d bytes)",ASMX_MAX_BYTSTR);
                    Error(s);
                    break;
                }

                if (parm == 1) // DS.B
                    for (i=0; i<val; i++)
                        bytStr[i] = n;
                else           // DS.W
                {
                    if (curEndian != ASMX_LITTLE_END && curEndian != ASMX_BIG_END)
                    {
                        Error("CPU endian not defined");
                        break;
                    }

                    for (i=0; i<val*parm; i+=parm)
                    {
                        if (curEndian == ASMX_BIG_END)
                        {
                            if (parm == 4)
                            {
                                bytStr[i]   = n >> 24;
                                bytStr[i+1] = n >> 16;
                                bytStr[i+2] = n >> 8;
                                bytStr[i+3] = n;
                            }
                            else
                            {
                                bytStr[i]   = n >> 8;
                                bytStr[i+1] = n;
                            }
                        }
                        else
                        {
                            bytStr[i]   = n;
                            bytStr[i+1] = n >> 8;
                            if (parm == 4)
                            {
                                bytStr[i+2] = n >> 16;
                                bytStr[i+3] = n >> 24;
                            }
                        }
                    }
                }
                instrLen = -val * parm;

                break;
            }
            else if (token)
            {
                linePtr = oldLine;
                Comma();
                token = 0;
            }

            if (pass == 2)
            {
                showAddr = FALSE;

                // "XXXX  (XXXX)"
                p = ListLoc(locPtr);
                *p++ = ' ';
                *p++ = '(';
                p = ListAddr(p,val);
                *p++ = ')';
            }

            AddLocPtr(val*parm);
            break;

        case o_HEX:
            instrLen = 0;
            while (!errFlag && GetWord(word))
            {
                n = asmx_strlen(word);
                for (i=0; i<n; i+=2)
                {
                    if (ishex(word[i]) && ishex(word[i+1]))
                    {
                        val = Hex2Dec(word[i]) * 16 + Hex2Dec(word[i+1]);
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val;
                    }
                    else
                    {
                        Error("Invalid HEX string");
                        n = i;
                    }
                }
            }

            if (instrLen >= ASMX_MAX_BYTSTR)
            {
                Error("String too long");
                instrLen = ASMX_MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case o_FCC:
            instrLen = 0;
            oldLine = linePtr;
            token = GetWord(word);

            if (token == 0)
               MissingOperand();
            else if (token == -1)
            {
                linePtr = oldLine;
                val = Eval();
                token = GetWord(word);
                if (val >= ASMX_MAX_BYTSTR)
                    Error("String too long");
                if (!errFlag && (token == ','))
                {
                    while (*linePtr >= 0x20 && instrLen < val)
                        bytStr[instrLen++] = *linePtr++;
                    while (instrLen < val)
                        bytStr[instrLen++] = ' ';
                }
                else IllegalOperand();
            }
            else
            {
                ch = token;
                while (token)
                {
                    if (token == ch)
                    {
                        while (token == ch)
                        {   while (*linePtr != 0 && *linePtr != token)
                            {
                                if (instrLen < ASMX_MAX_BYTSTR)
                                    bytStr[instrLen++] = *linePtr++;
                            }
                            if (*linePtr)   linePtr++;
                                    else    Error("FCC not terminated properly");
                            if (*linePtr == token)
                            {
                                if (instrLen < ASMX_MAX_BYTSTR)
                                    bytStr[instrLen++] = *linePtr++;
                            }
                            else
                                token = *linePtr;
                        }
                    }
                    else
                    {
                        linePtr = oldLine;
                        val = EvalByte();
                        if (instrLen < ASMX_MAX_BYTSTR)
                            bytStr[instrLen++] = val;
                    }

                    token = GetWord(word);
                    oldLine = linePtr;

                    if (token == ',')
                    {
                        token = GetWord(word);
                        if (token == 0)
                            MissingOperand();
                    }
                    else if (token)
                    {
                        linePtr = oldLine;
                        Comma();
                        token = 0;
                    }
                    else if (errFlag)   // this is necessary to keep EvalByte() errors
                        token = 0;      // from causing phase errors
                }
            }

            if (instrLen >= ASMX_MAX_BYTSTR)
            {
                Error("String too long");
                instrLen = ASMX_MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case o_ALIGN_n:
            val = parm;
        case o_ALIGN:
            if (typ == o_ALIGN)
                val = Eval();

            // val must be a power of two
            if (val <= 0 || val > 65535 || (val & (val - 1)) != 0)
            {
                IllegalOperand();
                val = 0;
            }
            else
            {
                i = locPtr & ~(val - 1);
                if (i != (int)locPtr)
                    val = val - (locPtr - i); // aka val = val + i - locPtr;
                else
                    val = 0;
            }

            if (pass == 2)
            {
                showAddr = FALSE;

                // "XXXX  (XXXX)"
                p = ListLoc(locPtr);
                *p++ = ' ';
                *p++ = '(';
                p = ListAddr(p,val);
                *p++ = ')';
            }

            AddLocPtr(val);
            break;

        case o_END:
            if (nInclude >= 0)
                CloseInclude();
            else
            {
                oldLine = linePtr;
                if (GetWord(word))
                {
                    linePtr = oldLine;
                    val = Eval();
                    CodeXfer(val);

                    // "XXXX  (XXXX)"
                    p = ListLoc(locPtr);
                    *p++ = ' ';
                    *p++ = '(';
                    p = ListAddr(p,val);
                    *p++ = ')';
                }
                sourceEnd = TRUE;
            }
            break;

        case o_Include:
            GetFName(word);

            switch(OpenInclude(word))
            {
                case -1:
                    Error("Too many nested INCLUDEs");
                    break;
                case 0:
                    asmx_sprintf(s,"Unable to open INCLUDE file '%s'",word);
                    Error(s);
                    break;
                default:
                    break;
            }
            break;

         case o_ENDM:
            Error("ENDM without MACRO");
            break;

#ifdef ENABLE_REP
        case o_REPEND:
            Error("REPEND without REPEAT");
            break;
#endif

        case o_Processor:
            if (!GetWord(word)) MissingOperand();
            else if (!SetCPU(word)) IllegalOperand();
            break;

        default:
            Error("Unknown opcode");
            break;
    }
}


void DoLabelOp(int typ, int parm, char *labl)
{
    int         val;
    int         i,n;
    asmx_Str255 word,s;
    int         token;
    asmx_Str255 opcode;
    MacroPtr    macro;
    MacroPtr    xmacro;
    int         nparms;
    SegPtr      seg;
    char        *oldLine;
//  struct MacroRec repList;        // repeat text list
//  MacroLinePtr    rep,rep2;       // pointer into repeat text list
    char        *p;

    if (DoCPULabelOp(typ,parm,labl)) return;

    switch(typ)
    {
        case o_EQU:
            if (labl[0] == 0)
                Error("Missing label");
            else
            {
                val = Eval();

                // "XXXX  (XXXX)"
                p = listLine;
                switch(addrWid)
                {
                    default:
                    case ASMX_ADDR_16:
                        if (curListWid == ASMX_LIST_24) n=5;
                                                else n=4;
                        break;

                    case ASMX_ADDR_24:
                        n=6;
                        break;

                    case ASMX_ADDR_32:
                        n=8;
                        break;
                }
                for (i = 0; i <= n; i++) *p++ = ' ';
                *p++ = '=';
                *p++ = ' ';
                p = ListAddr(p,val);
                DefSym(labl,val,parm==1,parm==0);
            }
            break;

        case o_ORG:
            CodeAbsOrg(Eval());
            if (!evalKnown)
                Warning("Undefined label used in ORG statement");
            DefSym(labl,locPtr,FALSE,FALSE);
            showAddr = TRUE;
            break;

        case o_RORG:
            val = Eval();
            CodeRelOrg(val);
            DefSym(labl,codPtr,FALSE,FALSE);

            if (pass == 2)
            {
                // "XXXX = XXXX"
                p = ListLoc(codPtr);
                *p++ = '=';
                *p++ = ' ';
                p = ListAddr(p,val);
            }
            break;

        case o_SEG:
            token = GetWord(word);  // get seg name
            if (token == 0 || token == -1)  // must be null or alphanumeric
            {
                seg = FindSeg(word);    // find segment storage and create if necessary
                if (!seg) seg = AddSeg(word);
//              seg -> gen = parm;      // copy gen flag from parameter
                SwitchSeg(seg);
                DefSym(labl,locPtr,FALSE,FALSE);
                showAddr = TRUE;
            }
            break;

        case o_SUBR:
            token = GetWord(word);  // get subroutine name
            asmx_strcpy(subrLabl,word);
            break;

        case o_REND:
            if (pass == 2)
            {
                // "XXXX = XXXX"
                p = ListLoc(locPtr);
            }

            DefSym(labl,locPtr,FALSE,FALSE);

            CodeAbsOrg(codPtr);

            break;

        case o_LIST:
            listThisLine = TRUE;    // always list this line

            if (labl[0])
                Error("Label not allowed");

            GetWord(word);

                 if (asmx_strcmp(word,"ON") == 0)        listFlag = TRUE;
            else if (asmx_strcmp(word,"OFF") == 0)       listFlag = FALSE;
            else if (asmx_strcmp(word,"MACRO") == 0)     listMacFlag = TRUE;
            else if (asmx_strcmp(word,"NOMACRO") == 0)   listMacFlag = FALSE;
            else if (asmx_strcmp(word,"EXPAND") == 0)    expandHexFlag = TRUE;
            else if (asmx_strcmp(word,"NOEXPAND") == 0)  expandHexFlag = FALSE;
            else if (asmx_strcmp(word,"SYM") == 0)       symtabFlag = TRUE;
            else if (asmx_strcmp(word,"NOSYM") == 0)     symtabFlag = FALSE;
            else if (asmx_strcmp(word,"TEMP") == 0)      tempSymFlag = TRUE;
            else if (asmx_strcmp(word,"NOTEMP") == 0)    tempSymFlag = FALSE;
            else                                         IllegalOperand();

            break;
/*
        case o_PAGE:
            listThisLine = TRUE;    // always list this line

            if (labl[0])
                Error("Label not allowed");

            listLineFF = TRUE;
            break;
*/
        case o_OPT:
            listThisLine = TRUE;    // always list this line

            if (labl[0])
                Error("Label not allowed");

            GetWord(word);

                 if (asmx_strcmp(word,"LIST") == 0)      listFlag = TRUE;
            else if (asmx_strcmp(word,"NOLIST") == 0)    listFlag = FALSE;
            else if (asmx_strcmp(word,"MACRO") == 0)     listMacFlag = TRUE;
            else if (asmx_strcmp(word,"NOMACRO") == 0)   listMacFlag = FALSE;
            else if (asmx_strcmp(word,"EXPAND") == 0)    expandHexFlag = TRUE;
            else if (asmx_strcmp(word,"NOEXPAND") == 0)  expandHexFlag = FALSE;
            else if (asmx_strcmp(word,"SYM") == 0)       symtabFlag = TRUE;
            else if (asmx_strcmp(word,"NOSYM") == 0)     symtabFlag = FALSE;
            else if (asmx_strcmp(word,"TEMP") == 0)      tempSymFlag = TRUE;
            else if (asmx_strcmp(word,"NOTEMP") == 0)    tempSymFlag = FALSE;
            else                                    Error("Illegal option");

            break;

        case o_ERROR:
            if (labl[0])
                Error("Label not allowed");
            while (*linePtr == ' ' || *linePtr == '\t')
                linePtr++;
            Error(linePtr);
            break;

        case o_ASSERT:
            if (labl[0])
                Error("Label not allowed");
            val = Eval();
            if (!val)
                Error("Assertion failed");
            break;

        case o_MACRO:
            // see if label already provided
            if (labl[0] == 0)
            {
                // get next word on line for macro name
                if (GetWord(labl) != -1)
                {
                    Error("Macro name requried");
                    break;
                }
                // optional comma after macro name
                oldLine = linePtr;
                if (GetWord(word) != ',')
                    linePtr = oldLine;
            }

            // don't allow temporary labels as macro names
#ifdef TEMP_LBLAT
            if (asmx_strchr(labl, '.') || (!(opts & OPT_ATSYM) && strchr(labl, '@')))
#else
            if (asmx_strchr(labl, '.'))
#endif
            {
                Error("Can not use temporary labels as macro names");
                break;
            }

            macro = FindMacro(labl);
            if (macro && macro -> def)
                Error("Macro multiply defined");
            else
            {
                if (macro == 0)
                {
                    macro = AddMacro(labl);
                    nparms = 0;

                    token = GetWord(word);
                    while (token == -1)
                    {
                        nparms++;
                        if (nparms > MAXMACPARMS)
                        {
                            Error("Too many macro parameters");
                            macro -> toomany = TRUE;
                            break;
                        }
                        AddMacroParm(macro,word);
                        token = GetWord(word);
                        if (token == ',')
                            token = GetWord(word);
                    }

                    if (word[0] && !errFlag)
                        Error("Illegal operand");
                }

                if (pass == 2)
                {
                    macro -> def = TRUE;
                    if (macro -> toomany)
                        Error("Too many macro parameters");
                }

                macroCondLevel = 0;
                i = ReadSourceLine(curLine, sizeof(curLine));
                while (i && typ != o_ENDM)
                {
                    if ((pass == 2) && (listFlag || errFlag))
                        ListOut(TRUE);
                    CopyListLine();

                    // skip initial formfeeds
                    linePtr = curLine;
                    while (*linePtr == 12)
                        linePtr++;

                    // get label
                    labl[0] = 0;
#ifdef TEMP_LBLAT
                    if (isalphanum(*linePtr) || *linePtr == '.' || *linePtr == '@')
#else
                    if (isalphanum(*linePtr) || *linePtr == '.' || ((opts & ASMX_OPT_ATSYM) && *linePtr == '@'))
#endif
                    {
                        token = GetWord(labl);
                        if (token)
                            showAddr = TRUE;
                            while (*linePtr == ' ' || *linePtr == '\t')
                                linePtr++;

                        if (labl[0])
                        {
#ifdef TEMP_LBLAT
                            if (token == '.' || token == '@')
#else
                            if (token == '.')
#endif
                            {
                                GetWord(word);
                                if (token == '.' && subrLabl[0])    asmx_strcpy(labl,subrLabl);
                                                            else    asmx_strcpy(labl,lastLabl);
                                labl[asmx_strlen(labl)+1] = 0;
                                labl[asmx_strlen(labl)]   = token;
                                asmx_strcat(labl,word);          // labl = lastLabl + "." + word;
                            }
                            else
                                asmx_strcpy(lastLabl,labl);
                        }

                        if (*linePtr == ':' && linePtr[1] != '=')
                            linePtr++;
                    }

                    typ = 0;
                    GetFindOpcode(opcode, &typ, &parm, &xmacro);

                    switch(typ)
                    {
                        case o_IF:
                            if (pass == 1)
                                AddMacroLine(macro,curLine);
                            macroCondLevel++;
                            break;

                        case o_ENDIF:
                            if (pass == 1)
                                AddMacroLine(macro,curLine);
                            if (macroCondLevel)
                                macroCondLevel--;
                            else
                                Error("ENDIF without IF in macro definition");
                            break;

                        case o_END:
                            Error("END not allowed inside a macro");
                            break;

                        case o_ENDM:
                            if (pass == 1 && labl[0])
                                AddMacroLine(macro,labl);
                            break;

                        default:
                            if (pass == 1)
                                AddMacroLine(macro,curLine);
                            break;
                    }
                    if (typ != o_ENDM)
                        i = ReadSourceLine(curLine, sizeof(curLine));
                }

                if (macroCondLevel)
                    Error("IF block without ENDIF in macro definition");

                if (typ != o_ENDM)
                    Error("Missing ENDM");
            }
            break;

        case o_IF:
            if (labl[0])
                Error("Label not allowed");


            if (condLevel >= MAX_COND)
                Error("IF statements nested too deeply");
            else
            {
                condLevel++;
                condState[condLevel] = 0; // this block false but level not permanently failed

                val = Eval();
                if (!errFlag && val != 0)
                    condState[condLevel] = condTRUE; // this block true
            }
            break;

        case o_ELSE:    // previous IF was true, so this section stays off
            if (labl[0])
                Error("Label not allowed");

            if (condLevel == 0)
                Error("ELSE outside of IF block");
            else
                if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                    Error("Multiple ELSE statements in an IF block");
                else
                {
                    condState[condLevel] = condELSE | condFAIL; // ELSE encountered, permanent fail
//                  condState[condLevel] |= condELSE; // ELSE encountered
//                  condState[condLevel] |= condFAIL; // this level permanently failed
//                  condState[condLevel] &= ~condTRUE; // this block false
                }
            break;

        case o_ELSIF:   // previous IF was true, so this section stays off
            if (labl[0])
                Error("Label not allowed");

            if (condLevel == 0)
                Error("ELSIF outside of IF block");
            else
                if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                    Error("Multiple ELSE statements in an IF block");
                else
                {
                    // i = Eval(); // evaluate condition and ignore result
                    EatIt(); // just ignore the conditional expression

                    condState[condLevel] |= condFAIL; // this level permanently failed
                    condState[condLevel] &= ~condTRUE; // this block false
                }
            break;

        case o_ENDIF:   // previous ELSE was true, now exiting it
            if (labl[0])
                Error("Label not allowed");

            if (condLevel == 0)
                Error("ENDIF outside of IF block");
            else
                condLevel--;
            break;

#ifdef ENABLE_REP
// still under construction
// notes:
//      REPEAT pseudo-op should act like an inline macro
//      1) collect all lines into new macro level
//      2) copy parameters from previous macro level (if any)
//      3) set repeat count for this macro level (repeat count is set to 0 for plain macro)
//      4) when macro ends, decrement repeat count and start over if count > 0
//      5) don't forget to dispose of the temp macro and its lines when done!
        case o_REPEAT:
            if (labl[0])
            {
                DefSym(labl,locPtr,FALSE,FALSE);
                showAddr = TRUE;
            }

            // get repeat count
            val = Eval();
            if (!errFlag)
                if (val < 0) IllegalOperand();

            if (!errFlag)
            {
                repList -> text = NULL;

// *** read line
// *** while line not REPEND
// ***      add line to repeat buffer
                macroCondLevel = 0;
                i = ReadSourceLine(line, sizeof(line));
                while (i && typ != o_REPEND)
                {
                    if ((pass == 2) && (listFlag || errFlag))
                        ListOut(TRUE);
                    CopyListLine();

                    // skip initial formfeeds
                    linePtr = line;
                    while (*linePtr == 12)
                        linePtr++;

                    // get label
                    labl[0] = 0;
#ifdef TEMP_LBLAT
                    if (isalphanum(*linePtr) || *linePtr == '.')
#else
                    if (isalphanum(*linePtr) || *linePtr == '.' || ((opts & OPT_ATSYM) && *linePtr == '@'))
#endif
                    {
                        token = GetWord(labl);
                        if (token)
                            showAddr = TRUE;
                            while (*linePtr == ' ' || *linePtr == '\t')
                                linePtr++;

                        if (labl[0])
                        {
#ifdef TEMP_LBLAT
                            if (token == '.' || token == '@'))
#else
                            if (token == '.')
#endif
                            {
                                GetWord(word);
                                if (token == '.' && subrLabl[0])    strcpy(labl,subrLabl);
                                                            else    strcpy(labl,lastLabl);
                                labl[strlen(labl)+1] = 0;
                                labl[strlen(labl)]   = token;
                                strcat(labl,word);          // labl = lastLabl + "." + word;
                            }
                            else
                                strcpy(lastLabl,labl);
                        }

                        if (*linePtr == ':' && linePtr[1] != '=')
                            linePtr++;
                    }

                    typ = 0;
                    GetFindOpcode(opcode, &typ, &parm, &xmacro);

                    switch(typ)
                    {
                        case o_IF:
                            if (pass == 1)
                                AddMacroLine(&replist,line);
                            macroCondLevel++;
                            break;

                        case o_ENDIF:
                            if (pass == 1)
                                AddMacroLine(&replist,line);
                            if (macroCondLevel)
                                macroCondLevel--;
                            else
                                Error("ENDIF without IF in REPEAT block");
                            break;

                        case o_END:
                            Error("END not allowed inside REPEAT block");
                            break;

                        case o_ENDM:
                            if (pass == 1 && labl[0])
                                AddMacroLine(&replist,labl);
                            break;

                        default:
                            if (pass == 1)
                                AddMacroLine(&replist,line);
                            break;
                    }
                    if (typ != o_ENDM)
                        i = ReadSourceLine(line, sizeof(line));
                }

                if (macroCondLevel)
                    Error("IF block without ENDIF in REPEAT block");

                if (typ != o_REPEND)
                    Error("Missing REPEND");

                if (!errFlag)
                {
// *** while (val--)
// ***      for each line
// ***            doline()
                }

                // free repeat line buffer
                while (replist)
                {
                    rep = replist->next;
                    asmx_free(replist);
                    replist = rep;
                }
            }
            break;
#endif

       case o_Incbin:
            DefSym(labl,locPtr,FALSE,FALSE);

            GetFName(word);

            val = 0;

            // open binary file
            incbin = asmx_fopen(word, "r");

            if (incbin)
            {
                // while not EOF
                do {
                    //   n = count of read up to MAX_BYTSTR bytes into bytStr
                    n = asmx_fread(bytStr, 1, ASMX_MAX_BYTSTR, incbin);
                    if (n>0)
                    {
                        // write data out to the object file
                        for (i=0; i<n; i++)
                            CodeOut(bytStr[i]);
                        val = val + n;
                    }
                } while (n>0);
                if (n<0)
                    asmx_sprintf(s,"Error reading INCBIN file '%s'",word);

                if (pass == 2)
                {
                    // "XXXX  (XXXX)"
                    p = ListLoc(locPtr-val);
                    *p++ = ' ';
                    *p++ = '(';
                    p = ListAddr(p,val);
                    *p++ = ')';
                }

            }
            else
            {
                asmx_sprintf(s,"Unable to open INCBIN file '%s'",word);
                Error(s);
            }

            // close binary file
            if (incbin) asmx_fclose(incbin);
            incbin = 0;

            break;

        case o_WORDSIZE:
            if (labl[0])
                Error("Label not allowed");

            val = Eval();
            if (evalKnown)
            {
                Error("Forward reference not allowed in WORDSIZE");
            }
            else if (!errFlag)
            {
                if (val == 0)
                    SetWordSize(wordSize);
                else if (val < 1 || val > 64)
                    Error("WORDSIZE must be in the range of 0..64");
                else SetWordSize(val);
            }
            break;

        default:
            Error("Unknown opcode");
            break;
    }
}


static void DoLine()
{
    asmx_Str255 labl;
    asmx_Str255 opcode;
    int         typ;
    int         parm;
    int         i;
    asmx_Str255 word;
    int         token;
    MacroPtr    macro;
    char        *oldLine;
    char        *p;
    int         numhex;
    bool        firstLine;

    errFlag      = FALSE;
    warnFlag     = FALSE;
    instrLen     = 0;
    showAddr     = FALSE;
    listThisLine = listFlag;
    firstLine    = TRUE;
    CopyListLine();

    // skip initial formfeeds
    linePtr = curLine;
    while (*linePtr == 12)
        linePtr++;

    // look for label at beginning of line
    labl[0] = 0;
#ifdef TEMP_LBLAT
    if (isalphaul(*linePtr) || *linePtr == '$' || *linePtr == '.' || *linePtr == '@')
#else
    if (isalphaul(*linePtr) || *linePtr == '$' || *linePtr == '.' || ((opts & ASMX_OPT_ATSYM) && *linePtr == '@'))
#endif
    {
        token = GetWord(labl);
        oldLine = linePtr;
        if (token)
            showAddr = TRUE;
        while (*linePtr == ' ' || *linePtr == '\t')
            linePtr++;

           if (labl[0])
        {
#ifdef TEMP_LBLAT
            if (token == '.' || token == '@')
#else
            if (token == '.')
#endif
            {
                GetWord(word);
                if (token == '.' && FindOpcodeTab((asmx_OpcdPtr) &opcdTab2, word, &typ, &parm) )
                    linePtr = oldLine;
                else if (token == '.' && FindCPU(word))
                    linePtr = curLine;
                else
                {
                    if (token == '.' && subrLabl[0])    asmx_strcpy(labl,subrLabl);
                                                else    asmx_strcpy(labl,lastLabl);
                    labl[asmx_strlen(labl)+1] = 0;
                    labl[asmx_strlen(labl)]   = token;
                    asmx_strcat(labl,word);          // labl = lastLabl + "." + word;
                }
            }
            else
                asmx_strcpy(lastLabl,labl);
        }

        if (*linePtr == ':' && linePtr[1] != '=')
            linePtr++;
    }

    if (!(condState[condLevel] & condTRUE))
    {
        listThisLine = FALSE;

        // inside failed IF statement
        if (GetFindOpcode(opcode, &typ, &parm, &macro))
        {
            switch(typ)
            {
                case o_IF: // nested IF inside failed IF should stay failed
                    if (condState[condLevel-1] & condTRUE)
                        listThisLine = listFlag;

                    if (condLevel >= MAX_COND)
                        Error("IF statements nested too deeply");
                    else
                    {
                        condLevel++;
                        condState[condLevel] = condFAIL; // this level false and permanently failed
                    }
                    break;

                case o_ELSE:
                    if (condState[condLevel-1] & condTRUE)
                        listThisLine = listFlag;

                    if (!(condState[condLevel] & (condTRUE | condFAIL)))
                    {   // previous IF was false
                        listThisLine = listFlag;

                        if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                            Error("Multiple ELSE statements in an IF block");
                        else
                            condState[condLevel] |= condTRUE;
                    }
                    condState[condLevel] |= condELSE;
                    break;

                case o_ELSIF:
                    if (condState[condLevel-1] & condTRUE)
                        listThisLine = listFlag;

                    if (!(condState[condLevel] & (condTRUE | condFAIL)))
                    {   // previous IF was false
                        listThisLine = listFlag;

                        if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                            Error("Multiple ELSE statements in an IF block");
                        else
                        {
                            i = Eval();
                            if (!errFlag && i != 0)
                                condState[condLevel] |= condTRUE;
                        }
                    }
                    break;

                case o_ENDIF:
                    if (condLevel == 0)
                        Error("ENDIF outside of IF block");
                    else
                    {
                        condLevel--;
                        if (condState[condLevel] & condTRUE)
                            listThisLine = listFlag;
                    }
                    break;

                default:    // ignore any other lines
                    break;
            }
        }

        if ((pass == 2) && listThisLine && (errFlag || listMacFlag || !macLineFlag))
            ListOut(TRUE);
    }
    else
    {
        if (!GetFindOpcode(opcode, &typ, &parm, &macro) && !opcode[0])
        {   // line with label only
            DefSym(labl,locPtr / wordDiv,FALSE,FALSE);
        }
        else
        {
            if (typ == o_Illegal)
            {
                if (opcode[0] == '.' && SetCPU(opcode+1))
                    /* successfully changed CPU type */;
                else
                {
                    asmx_sprintf(word,"Illegal opcode '%s'",opcode);
                    Error(word);
                }
            }
            else if (typ == o_MacName)
            {
                if (macPtr[macLevel] && macLevel >= MAX_MACRO)
                    Error("Macros nested too deeply");
#if 1
                else if (pass == 2 && !macro -> def)
                    Error("Macro has not been defined yet");
#endif
                else
                {
                    if (macPtr[macLevel])
                        macLevel++;

                    macPtr [macLevel] = macro;
                    macLine[macLevel] = macro -> text;
#ifdef ENABLE_REP
                    macRepeat[macLevel] = 0;
#endif

                    GetMacParms(macro);

                    showAddr = TRUE;
                    DefSym(labl,locPtr,FALSE,FALSE);
                }
            }
            else if (typ >= o_LabelOp)
            {
                showAddr = FALSE;
                DoLabelOp(typ,parm,labl);
            }
            else
            {
                showAddr = TRUE;
                DefSym(labl,locPtr,FALSE,FALSE);
                DoOpcode(typ, parm);
            }

            if (typ != o_Illegal && typ != o_MacName)
                if (!errFlag && GetWord(word))
                    Error("Too many operands");
        }

        if (pass == 1)
            AddLocPtr(asmx_abs(instrLen));
        else
        {
            p = listLine;
            if (showAddr) p = ListLoc(locPtr);
            else switch(addrWid)
            {
                default:
                case ASMX_ADDR_16:
                    p = listLine + 5;
                    break;

                case ASMX_ADDR_24:
                    p = listLine + 7;
                    break;

                case ASMX_ADDR_32:
                    p = listLine + 9;
                    break;
            }

            // determine width of hex data area
            if (curListWid == ASMX_LIST_16)
                numhex = 5;
            else // listWid == LIST_24
            {
                switch(addrWid)
                {
                    default:
                    case ASMX_ADDR_16:
                    case ASMX_ADDR_24:
                        numhex = 8;
                        break;

                    case ASMX_ADDR_32:
                        numhex = 6;
                        break;
                }
            }

            if (instrLen>0) // positive instrLen for CPU instruction formatting
            {
                // determine start of hex data area
                switch(addrWid)
                {
                    default:
                    case ASMX_ADDR_16:
                        if (curListWid == ASMX_LIST_24) p = listLine + 6;
                                                   else p = listLine + 5;
                        break;

                    case ASMX_ADDR_24:
                        p = listLine + 7;
                        break;

                    case ASMX_ADDR_32:
                        p = listLine + 9;
                        break;
                }

                // special case because 24-bit address usually can't fit
                // 8 bytes of code with operand spacing
                if (addrWid == ASMX_ADDR_24 && curListWid == ASMX_LIST_24)
                    numhex = 6;

                if (hexSpaces & 1) { *p++ = ' '; }
                for (i = 0; i < instrLen; i++)
                {
                    if (curListWid == ASMX_LIST_24)
                        if (hexSpaces & (1<<i)) *p++ = ' ';

                    if (i<numhex || expandHexFlag)
                    {
                        if (i > 0 && i % numhex == 0)
                        {
                            if (listThisLine && (errFlag || listMacFlag || !macLineFlag))
                            {
                                ListOut(firstLine);
                                firstLine = FALSE;
                            }
                            if (curListWid == ASMX_LIST_24)
                                asmx_strcpy(listLine, "                        ");   // 24 blanks
                            else
                                asmx_strcpy(listLine, "                ");           // 16 blanks
                            p = ListLoc(locPtr);
                        }

                        p = ListByte(p,bytStr[i]);
                    }
                    CodeOut(bytStr[i]);
                }
            }
            else if (instrLen<0) // negative instrLen for generic data formatting
            {
                instrLen = asmx_abs(instrLen);
                for (i = 0; i < instrLen; i++)
                {
                    if (i<numhex || expandHexFlag)
                    {
                        if (i > 0 && i % numhex == 0)
                        {
                            if (listThisLine && (errFlag || listMacFlag || !macLineFlag))
                            {
                                ListOut(firstLine);
                                firstLine = FALSE;
                            }
                            if (curListWid == ASMX_LIST_24)
                                asmx_strcpy(listLine, "                        ");   // 24 blanks
                            else
                                asmx_strcpy(listLine, "                ");           // 16 blanks
                            p = ListLoc(locPtr);
                        }
                        if (numhex == 6 && (i%numhex) == 2) *p++ = ' ';
                        if (numhex == 6 && (i%numhex) == 4) *p++ = ' ';
                        if (numhex == 8 && (i%numhex) == 4 && addrWid != ASMX_ADDR_24) *p++ = ' ';
                        p = ListByte(p,bytStr[i]);
                        if (i>=numhex) *p = 0;
                    }
                    CodeOut(bytStr[i]);
                }
            }

            if (listThisLine && (errFlag || listMacFlag || !macLineFlag))
                ListOut(firstLine);
        }
    }
}


static void DoPass()
{
    asmx_Str255 opcode;
    int         typ;
    int         parm;
    int         i;
    MacroPtr    macro;
    SegPtr      seg;

    asmx_fseek(source, 0, ASMX_SEEK_SET); // rewind source file
    sourceEnd = FALSE;
    lastLabl[0] = 0;
    subrLabl[0] = 0;

    asmx_fprintf(asmx_stderr,"Pass %d\n",pass);

    errCount      = 0;
    condLevel     = 0;
    condState[condLevel] = condTRUE; // top level always true
    listFlag      = TRUE;
    listMacFlag   = FALSE;
    expandHexFlag = TRUE;
    symtabFlag    = TRUE;
    tempSymFlag   = TRUE;
    linenum       = 0;
    macLevel      = 0;
    macUniqueID   = 0;
    macCurrentID[0] = 0;
    curAsm        = 0;
    curEndian     = ASMX_UNKNOWN_END;
    opcdTab       = 0;
    curListWid    = ASMX_LIST_24;
    addrWid       = ASMX_ADDR_32;
    wordSize      = 8;
    opts          = 0;
    SetWordSize(wordSize);
    SetCPU(defCPU);
    addrMax       = addrWid;

    // reset all code pointers
    CodeAbsOrg(0);
    seg = segTab;
    while (seg)
    {
        seg -> cod = 0;
        seg -> loc = 0;
        seg = seg -> next;
    }
    curSeg = nullSeg;

    if (pass == 2) CodeHeader(cl_SrcName);

    PassInit();
    i = ReadSourceLine(curLine, sizeof(curLine));
    while (i && !sourceEnd)
    {
        DoLine();
        i = ReadSourceLine(curLine, sizeof(curLine));
    }

    if (condLevel != 0)
        Error("IF block without ENDIF");

    if (pass == 2) CodeEnd();

    // Put the lines after the END statement into the listing file
    // while still checking for listing control statements.  Ignore
    // any lines which have invalid syntax, etc., because whatever
    // is found after an END statement should esentially be ignored.

    if (pass == 2)
    {
        while (i)
        {
            listThisLine = listFlag;
            CopyListLine();

            if (curLine[0]==' ' || curLine[0]=='\t')          // ignore labels (this isn't the right way)
            {
                GetFindOpcode(opcode, &typ, &parm, &macro);
                if (typ == o_LIST || typ == o_OPT)
                {
                    DoLabelOp(typ,parm,"");
                }
            }

            if (listThisLine)
                ListOut(TRUE);

            i = ReadSourceLine(curLine, sizeof(curLine));
        }
    }

}

// --------------------------------------------------------------
// initialization and parameters


static bool ParseOptions(const asmx_Options* opts)
{
    assert(opts);

    int     val;
    asmx_Str255 labl,word;
    bool    setSym;
    int     token;
    int     neg;


    if (opts->srcName) {
        asmx_strncpy(cl_SrcName, opts->srcName, 255);
    }
    else {
        asmx_strncpy(cl_SrcName, "src.asm", 255);
    }
    if (opts->objName) {
        asmx_strncpy(cl_ObjName, opts->objName, 255);
    }
    else {
        asmx_strncpy(cl_ObjName, "out.obj", 255);
    }
    if (opts->lstName) {
        asmx_strncpy(cl_ListName, opts->lstName, 255);
    }
    else {
        asmx_strncpy(cl_ListName, "list.lst", 255);
    }
    if (opts->cpuType) {
        asmx_strncpy(word, opts->cpuType, 255);
        Uprcase(word);
        if (!FindCPU(word))
        {
            asmx_fprintf(asmx_stderr,"CPU type '%s' unknown\n",word);
            return false;
        }
        asmx_strcpy(defCPU, word);
    }
    cl_Err = opts->showErrors;
    cl_Warn = opts->showWarnings;
    cl_ObjType = OBJ_BIN;
    cl_Obj = TRUE;
    cl_List = TRUE;
    cl_Binbase = 0x10000;
    cl_Binend = 0;
    cl_AutoBinBaseEnd = true;
    for (int i = 0; i < ASMX_OPTIONS_MAX_DEFINES; i++) {
        if (opts->defines[i]) {
            asmx_strncpy(curLine, opts->defines[i], 255);
            linePtr = curLine;
            GetWord(labl);
            val = 0;
            setSym = FALSE;
            token = GetWord(word);
            if (token == ':')
            {
                setSym = TRUE;
                token = GetWord(word);
            }
            if (token == '=')
            {
                neg = 1;
                if (GetWord(word) == '-')
                {
                    neg = -1;
                    GetWord(word);
                }
                val = neg * EvalNum(word);
                if (errFlag)
                {
                    asmx_fprintf(asmx_stderr, "Invalid number '%s' in -d option\n",word);
                    return false;
                }
            }
            DefSym(labl,val,setSym,!setSym);
        }
    }
    return true;
}


bool asmx_Assemble(const asmx_Options* opts)
{
    int i;

    // initialize and get parms

    //progname   = argv[0];
    pass       = 0;
    symTab     = 0;
    xferAddr   = 0;
    xferFound  = FALSE;

    macroTab   = 0;
    macPtr[0]  = 0;
    macLine[0] = 0;
    segTab     = 0;
    nullSeg    = AddSeg("");
    curSeg     = nullSeg;

    cl_Err     = FALSE;
    cl_Warn    = FALSE;
    cl_List    = FALSE;
    cl_Obj     = FALSE;
    cl_ObjType = OBJ_HEX;

    asmTab     = 0;
    cpuTab     = 0;
    defCPU[0]  = 0;

    nInclude  = -1;
    for (i=0; i<MAX_INCLUDE; i++)
        include[i] = 0;

    cl_SrcName [0] = 0;     source  = 0;
    cl_ListName[0] = 0;     listing = 0;
    cl_ObjName [0] = 0;     object  = 0;
    incbin = 0;

    AsmInit();

    if (!ParseOptions(opts)) {
        return false;
    }

    // open files

    source = asmx_fopen(cl_SrcName, "r");
    if (source == 0)
    {
        asmx_fprintf(asmx_stderr,"Unable to open source input file '%s'!\n",cl_SrcName);
        return false;
    }

    if (cl_List)
    {
        listing = asmx_fopen(cl_ListName, "w");
        if (listing == 0)
        {
            asmx_fprintf(asmx_stderr,"Unable to create listing output file '%s'!\n",cl_ListName);
            if (source)
                asmx_fclose(source);
            return false;
        }
    }

    if (cl_Obj)
    {
        object = asmx_fopen(cl_ObjName, "w");
        if (object == 0)
        {
            asmx_fprintf(asmx_stderr,"Unable to create object output file '%s'!\n",cl_ObjName);
            if (source)
                asmx_fclose(source);
            if (listing)
                asmx_fclose(listing);
            return false;
        }
    }

    CodeInit();

    pass = 1;
    DoPass();

    pass = 2;
    DoPass();

    if (cl_List)    asmx_fprintf(listing, "\n%.5d Total Error(s)\n\n", errCount);
    if (cl_Err)     asmx_fprintf(asmx_stderr,  "\n%.5d Total Error(s)\n\n", errCount);

    if (symtabFlag)
    {
        SortSymTab();
        DumpSymTab();
    }
//  DumpMacroTab();

    if (source)
        asmx_fclose(source);
    if (listing)
        asmx_fclose(listing);
    if (object)
        asmx_fclose(object);

    return (errCount != 0);
}
