//------------------------------------------------------------------------------
//  ui_asm.cc
//
//  UI code for the integrated assembler.
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "TextEditor.h"
#include "imgui_internal.h"

static struct {
    TextEditor* editor;
    uint16_t prev_addr;
    uint16_t prev_len;
} state;

void ui_asm_init(void) {
    state.prev_addr = 0x0000;
    state.prev_len = 0x0200;
    state.editor = new TextEditor();
    state.editor->SetPalette(TextEditor::GetRetroBluePalette());
    state.editor->SetShowWhitespaces(false);
    state.editor->SetTabSize(8);
    state.editor->SetImGuiChildIgnored(true);

    // language definition for 6502 asm
    static TextEditor::LanguageDefinition def;
    static const char* keywords[] = {
        "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK",
        "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DEC", "DEX",
        "DEY", "EOR", "INC", "INX", "INY", "JMP", "JSR", "LDA", "LDX", "LDY", "LSR",
        "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "ROL", "ROR", "RTI", "RTS", "SBC",
        "SEC", "SED", "SEI", "STA", "STX", "STY", "TAX", "TAY", "TSX", "TXA", "TXS",
        "TYA"
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
    def.mName = "6502 ASM";
    state.editor->SetLanguageDefinition(def);
}

void ui_asm_discard(void) {
    assert(state.editor);
    delete state.editor;
}

void ui_asm_draw(void) {
    if (!app.ui.asm_open) {
        return;
    }
    auto cpos = state.editor->GetCursorPosition();
    const float footer_h = ImGui::GetItemsLineHeightWithSpacing();
    ImGui::SetNextWindowSize({480, 260}, ImGuiCond_Once);
    const char* cur_error_msg = 0;
    for (int i = 0; i < asm_num_errors(); i++) {
        const asm_error_t* err = asm_error(i);
        if (err->line_nr == (cpos.mLine+1)) {
            cur_error_msg = err->msg;
        }
    }
    if (ImGui::Begin("Assembler", &app.ui.asm_open, ImGuiWindowFlags_None)) {
        bool is_active = ImGui::IsWindowFocused();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(state.editor->GetPalette()[(int)TextEditor::PaletteIndex::Background]));
        ImGui::BeginChild("##asm", {0,-footer_h}, false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
        state.editor->Render("Assembler");
        // set focus to text input field whenever the parent window is active
        if (is_active) {
            ImGui::SetFocusID(ImGui::GetCurrentWindow()->ID, ImGui::GetCurrentWindow());
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        if (cur_error_msg) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xFF4444FF);
            ImGui::Text("%s", cur_error_msg);
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
    if (state.editor->IsTextChanged()) {
        ui_asm_assemble();
    }
}

void ui_asm_assemble(void) {
    app.binary.num_bytes = 0;
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
        sim_clear(state.prev_addr, state.prev_len);
        sim_write(asm_res.addr, asm_res.len, asm_res.bytes);
        state.prev_addr = asm_res.addr;
        state.prev_len = asm_res.len;
        // store in app.binary as PRG/BIN format (start address in first two bytes)
        app.binary.num_bytes = asm_res.len + 2;
        app.binary.buf[0] = (uint8_t) asm_res.addr;
        app.binary.buf[1] = (uint8_t) (asm_res.addr>>8);
        memcpy(&app.binary.buf[2], asm_res.bytes, asm_res.len);
    }
    TextEditor::ErrorMarkers err_markers;
    for (int err_index = 0; err_index < asm_num_errors(); err_index++) {
        const asm_error_t* err = asm_error(err_index);
        err_markers[err->line_nr] = err->msg;
    }
    state.editor->SetErrorMarkers(err_markers);
}

const char* ui_asm_source(void) {
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

void ui_asm_put_source(const char* name, const uint8_t* bytes, int num_bytes) {
    if (bytes && (num_bytes > 0)) {
        std::string str((char*)bytes, num_bytes);
        state.editor->SetText(str);
    }
}

void ui_asm_undo(void) {
    state.editor->Undo();
    ui_asm_assemble();
}

void ui_asm_redo(void) {
    state.editor->Redo();
    ui_asm_assemble();
}

void ui_asm_cut(void) {
    state.editor->Cut();
    ui_asm_assemble();
}

void ui_asm_copy(void) {
    state.editor->Copy();
    ui_asm_assemble();
}

void ui_asm_paste(void) {
    state.editor->Paste();
    ui_asm_assemble();
}
