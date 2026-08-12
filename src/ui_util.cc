#include "ui_util.h"
#include "pystring.h"

// helper shim to simulate old TextEditor::GetTextLines() method
std::vector<std::string> ui_editor_get_text_lines(TextEditor* editor) {
    assert(editor);
    std::vector<std::string> lines;
    pystring::splitlines(editor->GetText(), lines);
    return lines;
}

// helper shim to simulate old TextEditor::MoveEnd() method
void ui_editor_move_to_end(TextEditor* editor) {
    const size_t last_line = editor->GetLineCount() - 1;
    const size_t last_index = editor->GetLineText(last_line).size();
    editor->SetCursor(TextEditor::DocPos{ last_line, last_index });
}

// reconstruct original RetroBlue palette
TextEditor::Palette ui_editor_retro_blue_palette(void) {
    const size_t num = (size_t)TextEditor::Color::count;
    static const ImU32 colors[num] = {
        0xffffffff,  // text
        0xff00ffff,  // keyword
        0xffffff00,  // declaration
        0xff00ff00,  // number
        0xff7070e0,  // string
        0xffffffff,  // punctuation
        0xff408080,  // preprocessor
        0xffdddddd,  // identifier
        0xff9bc64d,  // knownIdentifier
        0xff206020,  // comment
        0xff800000,  // background
        0xff0080ff,  // cursor
        0x80a06020,  // selection
        0x40808080,  // whitespace
        0x40a0a0a0,  // matchingBracketBackground
        0xffa0a0a0,  // matchingBracketActive
        0xff00ff00,  // matchingBracketLevel1
        0xff00ffff,  // matchingBracketLevel2
        0xffff00ff,  // matchingBracketLevel3
        0xff0000ff,  // matchingBracketError
        0xff808000,  // lineNumber
        0xffc0c000,  // currentLineNumber
    };
    TextEditor::Palette p{};
    for (size_t i = 0; i < num; i++) {
        p[i] = colors[i];
    }
    return p;
}

static bool ui_editor_is_hex_digit(ImWchar c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static bool ui_editor_is_ident_head(ImWchar c) {
    return (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool ui_editor_is_ident_cont(ImWchar c) {
    return ui_editor_is_ident_head(c) || (c >= '0' && c <= '9');
}

const TextEditor::Language* ui_editor_asm_language_def() {
    static TextEditor::Language def;
    def.name = "ASM";
    def.commentStart = "/*";
    def.commentEnd = "*/";
    def.caseSensitive = false;
    def.singleLineComment = ";";
    def.hasSingleQuotedStrings = true;   // 'x' char literals
    def.hasDoubleQuotedStrings = true;   // "..." strings
    def.stringEscape = '\\';
    def.indentationForBlocks = true;
    static const char* keywords[] = {
        #if defined(CHIP_6502) || defined(CHIP_2A03)
        "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk",
        "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex",
        "dey", "eor", "inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy", "lsr",
        "nop", "ora", "pha", "php", "pla", "plp", "rol", "ror", "rti", "rts", "sbc",
        "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs",
        "tya",
        "db", "dw", "ds", "equ", "end", "org", "include", "if", "else", "endif", "align", "error"
        #elif defined(CHIP_Z80)
        "adc", "add", "and", "bit", "call", "ccf", "cp", "cpd", "cpdr", "cpi", "cpir",
        "cpl", "daa", "dec", "di", "djnz", "ei", "ex", "exx", "halt", "im", "in",
        "inc", "ind", "indr", "ini", "inir", "jp", "jr", "ld", "ldd", "lddr", "ldi", "ldir",
        "neg", "nop", "or", "otdr", "otir", "out", "outd", "outi", "pop", "push", "res",
        "ret", "reti", "retn", "rl", "rla", "rlc", "rlca", "rld", "rr", "rra", "rrc", "rrca",
        "rrd", "rst", "sbc", "scf", "set", "sla", "sll", "sra", "srl", "sub", "xor",
        "db", "dw", "ds", "equ", "end", "org", "include", "if", "else", "endif", "align", "error"
        #endif
    };
    for (const auto& k: keywords) {
        def.keywords.insert(k);
    }
    static const char* registers[] = {
        #if defined(CHIP_6502) || defined(CHIP_2A03)
        "a", "x", "y",
        #elif defined(CHIP_Z80)
        "a", "b", "c", "d", "e", "h", "l",
        "i", "r",
        "ixh", "ixl", "iyh", "iyl",
        "af", "bc", "de", "hl", "sp", "pc",
        "ix", "iy",
        "af'",
        "nz", "z", "nc", "po", "pe", "p", "m",
        #endif
    };
    for (const auto& r: registers) {
        def.declarations.insert(r);
    }
    def.getNumber = [](TextEditor::Iterator start, TextEditor::Iterator end) {
        TextEditor::Iterator it = start;

        // $-prefixed hex
        if (it != end && *it == '$') {
            ++it;
            TextEditor::Iterator hexStart = it;
            while (it != end && ui_editor_is_hex_digit(*it)) {
                ++it;
            }
            return (it == hexStart) ? start : it;
        }

        // h/H-suffixed hex (must start with a decimal digit)
        if (it != end && *it >= '0' && *it <= '9') {
            TextEditor::Iterator scan = it;
            while (scan != end && ui_editor_is_hex_digit(*scan)) {
                ++scan;
            }
            if (scan != end && (*scan == 'h' || *scan == 'H') && scan != it) {
                ++scan;
                return scan;
            }
        }

        // plain decimal
        while (it != end && *it >= '0' && *it <= '9') {
            ++it;
        }
        return (it == start) ? start : it;
    };

    def.isPunctuation = [](ImWchar c) {
        switch (c) {
            case '[': case ']': case '{': case '}':
            case '!': case '%': case '^': case '&':
            case '*': case '(': case ')': case '-':
            case '+': case '=': case '~': case '|':
            case '<': case '>': case '?': case '/':
            case ';': case ',': case '.':
                return true;
            default:
                return false;
        }
    };

    def.getIdentifier = [](TextEditor::Iterator start, TextEditor::Iterator end) {
        if (start == end) return start;
        ImWchar c = *start;
        bool head = (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (!head) return start;
        TextEditor::Iterator it = start;
        ++it;
        while (it != end) {
            ImWchar cc = *it;
            bool cont = (cc == '_') ||
                        (cc >= 'a' && cc <= 'z') ||
                        (cc >= 'A' && cc <= 'Z') ||
                        (cc >= '0' && cc <= '9');
            if (!cont) break;
            ++it;
        }
        return it;
    };

    def.customTokenizer = [](TextEditor::Iterator start, TextEditor::Iterator end, TextEditor::Color& color) {
        // detect labels (identifier:)
        if (start == end || !ui_editor_is_ident_head(*start)) {
            return start;
        }
        TextEditor::Iterator it = start;
        ++it;
        while (it != end && ui_editor_is_ident_cont(*it)) {
            ++it;
        }
        if (it != end && *it == ':') {
            ++it; // include the ':' in the token
            color = TextEditor::Color::knownIdentifier;
            return it;
        }
        return start; // not a label — let default lexer handle it
    };

    return &def;
}
