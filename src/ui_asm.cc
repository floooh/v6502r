//------------------------------------------------------------------------------
//  ui_asm.cc
//
//  UI code for the integrated assembler.
//------------------------------------------------------------------------------
#include <stdint.h>
#include "TextEditor.h"
#include "imgui_internal.h"

#include "ui.h"
#include "ui_asm.h"
#include "asm.h"
#include "sim.h"

static struct {
    bool valid;
    bool window_focused;
    TextEditor* editor;
    uint16_t prev_addr;
    uint16_t prev_len;
    struct {
        uint32_t num_bytes;
        uint8_t buf[MAX_BINARY_SIZE];
    } binary;
} state;

bool ui_asm_is_window_focused(void) {
    assert(state.valid);
    return state.window_focused;
}

range_t ui_asm_get_binary(void) {
    assert(state.valid);
    return { state.binary.buf, state.binary.num_bytes };
}

void ui_asm_init(void) {
    assert(!state.valid);
    state.prev_addr = 0x0000;
    state.prev_len = 0x0200;
    state.editor = new TextEditor();
    state.editor->SetPalette(TextEditor::GetRetroBluePalette());
    state.editor->SetShowWhitespaces(false);
    state.editor->SetTabSize(8);

    // language definition for 6502 asm
    static TextEditor::LanguageDefinition def;
    static const char* keywords[] = {
        #if defined(CHIP_6502) || defined(CHIP_2A03)
        "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK",
        "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DEC", "DEX",
        "DEY", "EOR", "INC", "INX", "INY", "JMP", "JSR", "LDA", "LDX", "LDY", "LSR",
        "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "ROL", "ROR", "RTI", "RTS", "SBC",
        "SEC", "SED", "SEI", "STA", "STX", "STY", "TAX", "TAY", "TSX", "TXA", "TXS",
        "TYA"
        #elif defined(CHIP_Z80)
        "ADC", "ADD", "AND", "BIT", "CALL", "CCF", "CP", "CPD", "CPDR", "CPI", "CPIR",
        "CPL", "DAA", "DEC", "DI", "DJNZ", "EI", "EX", "EXX", "HALT", "IM", "IN",
        "INC", "IND", "INDR", "INI", "INIR", "JP", "JR", "LD", "LDD", "LDDR", "LDI", "LDIR",
        "NEG", "NOP", "OR", "OTDR", "OTIR", "OUT", "OUTD", "OUTI", "POP", "PUSH", "RES",
        "RET", "RETI", "RETN", "RL", "RLA", "RLC", "RLCA", "RLD", "RR", "RRA", "RRC", "RRCA",
        "RRD", "RST", "SBC", "SCF", "SET", "SLA", "SLL", "SRA", "SRL", "SUB", "XOR"
        #endif
    };
    for (const auto& k: keywords) {
        def.mKeywords.insert(k);
    }
    def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
    def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
    def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\$[+-]?[0-9a-fA-F]*", TextEditor::PaletteIndex::Number));
    def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?[0-9]", TextEditor::PaletteIndex::Number));
    def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
    def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));
    def.mCommentStart = "/*";
    def.mCommentEnd = "*/";
    def.mSingleLineComment = ";";
    def.mCaseSensitive = false;
    def.mAutoIndentation = true;
    def.mName = "ASM";
    state.editor->SetLanguageDefinition(def);
    state.valid = true;
}

void ui_asm_discard(void) {
    assert(state.valid);
    assert(state.editor);
    delete state.editor;
    state.valid = false;
}

void ui_asm_draw(void) {
    assert(state.valid);
    state.window_focused = false;
    if (!ui_is_window_open(UI_WINDOW_ASM)) {
        return;
    }
    auto cpos = state.editor->GetCursorPosition();
    const float footer_h = ImGui::GetFrameHeightWithSpacing();
    ImGui::SetNextWindowSize({480, 260}, ImGuiCond_FirstUseEver);
    const asm_error_t* cur_error = 0;
    for (int i = 0; i < asm_num_errors(); i++) {
        const asm_error_t* err = asm_error(i);
        if (err->line_nr == (cpos.mLine+1)) {
            cur_error = err;
        }
    }
    if (ImGui::Begin(ui_window_id(UI_WINDOW_ASM), ui_window_open_ptr(UI_WINDOW_ASM))) {
        bool is_active = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        state.editor->Render("Assembler", {0,-footer_h}, false);
        if (is_active) {
            ui_set_keyboard_target(UI_WINDOW_ASM);
        }
        if (cur_error) {
            ImGui::PushStyleColor(ImGuiCol_Text, cur_error->warning ? 0xFF44FFFF : 0xFF4444FF);
            ImGui::Text("%s", cur_error->msg);
            ImGui::PopStyleColor();
        }
        state.window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    }
    ImGui::End();
    if (state.editor->IsTextChanged()) {
        ui_asm_assemble();
    }
    ui_check_dirty(UI_WINDOW_ASM);
}

void ui_asm_assemble(void) {
    assert(state.valid);
    state.binary.num_bytes = 0;
    asm_init();
    asm_source_open();
    auto lines = state.editor->GetTextLines();
    for (const auto& line: lines) {
        asm_source_write(line.c_str(), 8);
        asm_source_write("\n", 8);
    }
    asm_source_close();
    asm_result_t asm_res = asm_assemble();
    if (!asm_res.errors && (asm_res.len > 0)) {
        sim_mem_clear(state.prev_addr, state.prev_len);
        sim_mem_write(asm_res.addr, asm_res.len, asm_res.bytes);
        state.prev_addr = asm_res.addr;
        state.prev_len = asm_res.len;
        // store in app.binary as PRG/BIN format (start address in first two bytes)
        state.binary.num_bytes = asm_res.len + 2;
        state.binary.buf[0] = (uint8_t) asm_res.addr;
        state.binary.buf[1] = (uint8_t) (asm_res.addr>>8);
        memcpy(&state.binary.buf[2], asm_res.bytes, asm_res.len);
    }
    TextEditor::ErrorMarkers err_markers;
    for (int err_index = 0; err_index < asm_num_errors(); err_index++) {
        const asm_error_t* err = asm_error(err_index);
        if (!err->warning) {
            err_markers[err->line_nr] = err->msg;
        }
    }
    state.editor->SetErrorMarkers(err_markers);
}

const char* ui_asm_source(void) {
    assert(state.valid);
    asm_init();
    asm_source_open();
    auto lines = state.editor->GetTextLines();
    for (const auto& line: lines) {
        asm_source_write(line.c_str(), 8);
        asm_source_write("\n", 8);
    }
    asm_source_close();
    return asm_source();
}

void ui_asm_put_source(const char* name, range_t src) {
    assert(state.valid);
    (void)name;
    if (src.ptr && (src.size > 0)) {
        std::string str((char*)src.ptr, src.size);
        state.editor->SetText(str);
    }
}

void ui_asm_undo(void) {
    assert(state.valid);
    state.editor->Undo();
    ui_asm_assemble();
}

void ui_asm_redo(void) {
    assert(state.valid);
    state.editor->Redo();
    ui_asm_assemble();
}

void ui_asm_cut(void) {
    assert(state.valid);
    state.editor->Cut();
    ui_asm_assemble();
}

void ui_asm_copy(void) {
    assert(state.valid);
    state.editor->Copy();
    ui_asm_assemble();
}

void ui_asm_paste(void) {
    assert(state.valid);
    state.editor->Paste();
    ui_asm_assemble();
}
