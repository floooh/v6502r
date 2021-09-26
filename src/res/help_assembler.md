# The Assembler

NOTE: the original asmx documentation is [here](http://svn.xi6.com/svn/asmx/branches/2.x/asmx-doc.html).

The [integrated assembler](ui://assembler) is a "librarised" version of [asmx](http://xi6.com/projects/asmx/) with all CPU targets except the 6502 stripped out.

The [assembler window](ui://assembler) can be opened with 'Alt+A'. The source code is assembled automatically on each key stroke or when the source changes otherwise. Errors and warnings are displayed immediately.

External source code can be loaded into the editor window via 'Ctrl-Shift-O' (on Mac 'Cmd-Shift-O'), or via copy-paste ('Ctrl-V' or 'Cmd-V').

The edited source can be saved back to the host via 'Ctrl-Shift-S' or with copy-paste.

An assembler run produces two outputs:

- the actual binary code, which is immediately copied into the 6502's memory
- an 'output listing' which can be inspected in the [Listing window](ui://listing)

The output listing can be saved to the host throught the menu item 'File => Save Listing'.

The generated binary code can be saved to the host through the 'File => Save BIN/PRG' menu item. The file format is very simple: 2 bytes start address in little endian format, followed by the actual program bytes.

# 6502 Addressing Modes

The 6502 addressing modes have the following syntax (using the LDA instruction as example)

- Immediate:            LDA #10
- Zero Page:            LDA $20
- Zero Page,X:          LDA $20,X
- Zero Page,Y:          LDA $20,Y
- Absolute:             LDA $1000
- Absolute,X:           LDA $1000,X
- Absolute,Y:           LDA $1000,Y
- Indexed Indirect:     LDA ($40,X)
- Indirect Indexed:     LDA ($40),Y

- Relative:             BNE LABEL, BNE +4, BNE $-20

- Indirect:             JMP ($1000)

# Expressions

Whenever a value is needed, it goes through the expression evaluator. The expression evaluator will attempt to do the arithmetic needed to get a result.

Unary operations take a single value and do something with it. The supported unary operations are:

    +val            positive of val
    -val            negative of val
    ~val            bitwise NOT of val
    !val            logical NOT of val (returns 1 if val is zero, else 0)
    <val            low 8 bits of val
    >val            high 8 bits of val
    ..DEF sym       returns 1 if symbol 'sym' has already been defined
    ..UNDEF sym     returns 1 if symbol 'sym' has not been defined yet
    ( expr )        parentheses for grouping sub-expressions
    [ expr ]        square brackets can be used as parentheses when necessary
    'c'             one or two character constants, equal to the ASCII value
    'cc'            ...in the two-byte case, the first character is the high byte
    H(val)          high 8 bits of val; whitespace not allowed before '('
    L(val)          low 8 bits of val; whitespace not allowed before '('

Binary operations take two values and do something with them. The supported binary operations are:

    x * y           x multipled by y
    x / y           x divided by y
    x % y           x modulo y
    x + y           x plus y
    x - y           x minus y
    x << y          x shifted left by y bits
    x >> y          x shifted right by y bits
    x & y           bitwise x AND y
    x | y           bitwise x OR y
    x ^ y           bitwise x XOR y
    x = y           comparison operators, return 1 if condition is true
    x == y          (note that = and == are the same)
    x < y
    x <= y
    x > y
    x >= y
    x && y          logical AND of x and y (returns 1 if x !=0 and y != 0)
    x || y          logical OR of x and y (returns 1 if x != 0 or y != 0)

Numbers:

    ., *, $                 current program counter
    $nnnn, nnnnH, 0xnnnn    hexadecimal constant
    nnnn, nnnnD             decimal constant
    nnnnO                   octal constant
    %nnnn, nnnnB            binary constant

 Hexadecimal constants of the form "nnnnH" don't need a leading zero if there is no label defined with that name.

Operator precedence:

( ) [ ]
unary operators: + - ~ ! < > ..DEF ..UNDEF
* / %
+ -
< <= > >= = == !=
& && | || ^ << >>

WARNING:
Shifts and AND, OR, and XOR have a lower precedence than the comparison operators! You must use parentheses when combining them with comparison operators!

Example:
Use "(OPTIONS & 3) = 2", not "OPTIONS & 3 = 2". The former checks the lowest two bits of the label OPTIONS, the latter compares "3 = 2" first, which always results in zero.

Also, unary operators have higher precedence, so if X = 255, "<X + 1" is 256, but "<(X + 1)" is 0.

NOTE: ..def and ..undef do not work with local labels. (the ones that start with '@' or '.')

# Labels and Comments

Labels must consist of alphanumeric characters or underscores, and must not begin with a digit. Examples are "FOO", "_BAR", and "BAZ_99". Labels are limited to 255 characters. Labels may also contain '$' characters, but must not start with one.

Labels must begin in the first column of the source file when they are declared, and may optionally have a ":" following them. Opcodes with no label must have at least one blank character before them.

Local labels are defined starting with "@" or ".". This glues whatever is after the "@" or "." to the last non-temporary code label defined so far, making a unique label. Example: "@1", "@99", ".TEMP", and "@LOOP". These can be used until the next non-local label, by using this short form. They appear in the symbol table with a long form of "LABEL@1" or "LABEL.1", but can not be referenced by this full name. Local labels starting with a "." can also be defined as subroutine local, by using the SUBROUTINE pseudo-op.

Comments may either be started with a "*" as the first non-blank character of a line, or with a ";" in the middle of the line.

Lines after the END pseudo-op are ignored as though they were comments, except for LIST and OPT lines.

# Pseudo-Ops

These are all the opcodes that have nothing to do with the instruction set of the CPU. All pseudo-ops can be preceeded with a "." (example: ".BYTE" works the same as "BYTE")

# ASSERT expr
Generates an error if expr is false (equals zero).

# ALIGN
This ensures that the next instruction or data will be located on a power-of-two boundary. The parameter must be a power of two (2, 4, 8, 16, etc.)

# DB / BYTE / DC.B / FCB
Defines one or more constant bytes in the code. You can use as many comma-separated values as you like. Strings use either single or double quotes. Doubled quotes inside a string assemble to a quote character. The backslash ("\") can escape a quote, or it can represent a tab ("\t"), linefeed ("\n"), or carriage return ("\r") character. Hex escapes ("\xFF") are also supported.

# DW / WORD / DC.W / FDB
Defines one or more constant 16-bit words in the code, using the native endian-ness of the CPU. The low word comes first. Quoted text strings are padded to a multiple of two bytes. The data is not aligned to a 2-byte address.

# DL / LONG / DC.L
Defines one or more constant 32-bit words in the code, using the native endian-ness of the CPU. The low word comes first. Quoted text strings are padded to a multiple of four bytes. The data is not aligned to a 4-byte address.

# DRW
Define Reverse Word - just like DW, except the bytes are reversed from the current endian setting.

# DS / RMB / BLKB
Skips a number of bytes, optionally initialized.
Examples:
    DS 5     ; skip 5 bytes (generates no object code)
    DS 6,'*' ; assemble 6 asterisks
Note that no forward-reference values are allowed for the length because this would cause phase errors.

# ERROR message
This prints a custom error message.

# EVEN
This is an alias for ALIGN 2.

# END
This marks the end of code. After the END statement, all input is ignored except for LIST and OPT lines.

# EQU / = / SET / :=
Sets a label to a value. The difference between EQU and SET is that a SET label is allowed to be redefined later in the source code. EQU and '=' are equivalent, and SET and ':=' are equivalent.

# HEX
Defines raw hexadecimal data. Individual hex bytes may be separated by spaces.
Examples:
    HEX 123456     ; assembles to hex bytes 12, 34, and 56
    HEX 78 9ABC DE ; assembles to hex bytes 78, 9A, BC and DE
    HEX 1 2 3 4    ; Error: hexadecimal digits must be in pairs

# IF expr / ELSE / ELSIF expr / ENDIF
Conditional assembly. Depending on the value in the IF statement, code between it and the next ELSE / ELSIF / ENDIF, and code between an ELSE and an ENDIF, may or may not be assembled.
Example:
    IF .undef mode
        ERROR mode not defined!
    ELSIF mode = 1
        JSR mode1
    ELSIF mode = 2
        JSR mode2
    ELSE
        ERROR Invalid value of mode!
    ENDI
IF statements inside a macro only work inside that macro. When a macro is defined, IF statements are checked for matching ENDIF statements.

# LIST / OPT
These set assembler options. Currently, the options are:
    LIST ON / OPT LIST              Turn on listing
    LIST OFF / OPT NOLIST           Turn off listing
    LIST MACRO / OPT MACRO          Turn on macro expansion in listing
    LIST NOMACRO / OPT NOMACRO      Turn off macro expansion in listing
    LIST EXPAND / OPT EXPAND        Turn on data expansion in listing
    LIST NOEXPAND / OPT NOEXPAND    Turn off data expansion in listing
    LIST SYM / OPT SYM              Turn on symbol table in listing
    LIST NOSYM / OPT NOSYM          Turn off symbol table in listing
    LIST TEMP / OPT TEMP            Turn on temp symbols in symbol table listing
    LIST NOTEMP / OPT NOTEMP        Turn off temp symbols in symbol table listing
The default is listing on, macro expansion off, data expansion on, symbol table on.

# MACRO / ENDM
Defines a macro. This macro is used whenver the macro name is used as an opcode. Parameters are defined on the MACRO line, and replace values used inside the macro.

Macro calls can be nested to a maximum of 10 levels. (This can be changed in asmx.c if you really need it bigger.)

Example:
    TWOBYTES    MACRO parm1, parm2     ; start recording the macro
            DB parm1, parm2
            ENDM                   ; stop recording the macro
            TWOBYTES  1, 2         ; use the macro - expands to "DB 1, 2"

    An alternate form with the macro name after MACRO, instead of as a label, is also accepted. A comma after the macro name is optional.
            MACRO plusfive parm
            DB (parm)+5
            ENDM

When a macro is invoked with insufficient parameters, the remaining parameters are replaced with a null string. It is an error to invoke a macro with too many parameters.

Macro parameters can be inserted without surrounding whitespace by using the '##' concatenation operator.

    TEST        MACRO labl
    labl ## 1   DB 1
    labl ## 2   DB 2
            ENDM

            TEST HERE   ; labl ## 1 gets replaced with "HERE1"
                    ; labl ## 2 gets replaced with "HERE2"

Macro parameters can also be inserted by using the backslash ("\") character. This method also includes a way to access the actual number of macro parameters supplied, and a unique identifier for creating temporary labels.

    \0 = number of macro parameters
    \1..\9 = nth macro parameter
    \? = unique ID per macro invocation (padded with leading zeros to five digits)

NOTE: The line with the ENDM may have a label, and that will be included in the macro definition. However if you include a backslash escape before the ENDM, the ENDM will not be recognized, and the macro definition will not end. Be careful!


# ORG
Sets the origin address of the following code. This defaults to zero at the start of each assembler pass.

# REND
Ends an RORG block. A label in front of REND receives the relocated address + 1 of the last relocated byte in the RORG / REND block.

# RORG
Sets the relocated origin address of the following code. Code in the object file still goes to the same addresses that follow the previous ORG, but labels and branches are handled as though the code were being assembled starting at the RORG address.

# SEG / RSEG / SEG.U segment
Switches to a new code segment. Code segments are simply different sections of code which get assembled to different addresses. They remember their last location when you switch back to them. If no segment name is specified, the null segment is used.

At the start of each assembler pass, all segment pointers are reset to zero, and the null segment becomes the current segment.

SEG.U is for DASM compatibility. DASM uses SEG.U to indicate an "unitialized" segment. This is necessary because its DS pseudo-op always generates data even when none is specified. Since the DS pseudo-op in this assembler normally doesn't generate any data, unitialized segments aren't supported as such.

RSEG is for compatibility with vintage Atari 7800 source code.

# SUBROUTINE / SUBR name
This sets the scope for temporary labels beginning with a dot. At the start of each pass, and when this pseudo-op is used with no name specified, temporary labels beginning with a dot use the previous non-temporary label, just as the temporary labels beginning with an '@'.

Example:
    START
    .LABEL  NOP        ; this becomes "START.LABEL"
        SUBROUTINE foo
    .LABEL  NOP        ; this becomes "FOO.LABEL"
        SUBROUTINE bar
    .LABEL  NOP        ; this becomes "BAR.LABEL"
        SUBROUTINE
    LABEL
    .LABEL  NOP        ; this becomes "LABEL.LABEL"


# Symbol Table Dump

The symbol table is dumped at the end of the listing file. Each symbol shows its name, value, and flags. The flags are:

    U   Undefined           this symbol was referenced but never defined
    M   Multiply defined    this symbol was defined more than once with different values (only the first is kept)
    S   Set                 this symbol was defined with the SET pseudo-op
    E   Equ                 this symbol was defined with the EQU pseudo-op
