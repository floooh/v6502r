//------------------------------------------------------------------------------
//  ui_asm.cc
//
//  UI code for the integrated assembler.
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "TextEditor.h"
#include "imgui_internal.h"

static TextEditor* editor;

void ui_asm_init(void) {
    editor = new TextEditor();
    editor->SetPalette(TextEditor::GetRetroBluePalette());
    editor->SetShowWhitespaces(false);
    editor->SetTabSize(8);
    editor->SetImGuiChildIgnored(true);

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
    editor->SetLanguageDefinition(def);
}

void ui_asm_discard(void) {
    assert(editor);
    delete editor;
}

void ui_asm_draw(void) {
    if (!app.ui.asm_open) {
        return;
    }
    auto cpos = editor->GetCursorPosition();
    const float footer_h = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::SetNextWindowSize({480, 260}, ImGuiCond_Once);
    if (ImGui::Begin("Assembler", &app.ui.asm_open, ImGuiWindowFlags_None)) {
        bool is_active = ImGui::IsWindowFocused();
        ImGui::Text("%6d/%-6d %6d lines  | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor->GetTotalLines(),
            editor->IsOverwrite() ? "Ovr" : "Ins",
            editor->CanUndo() ? "*" : " ");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(editor->GetPalette()[(int)TextEditor::PaletteIndex::Background]));
        ImGui::BeginChild("##asm", {0,-footer_h}, false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
        editor->Render("Assembler");
        // set focus to text input field whenever the parent window is active
        if (is_active) {
            ImGui::SetFocusID(ImGui::GetCurrentWindow()->ID, ImGui::GetCurrentWindow());
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        if (ImGui::Button("Assemble")) {
            // FIXME
        }
        ImGui::SameLine();
        if (editor->CanUndo()) {
            if (ImGui::Button("Undo")) {
                editor->Undo();
            }
            ImGui::SameLine();
        }
        if (editor->CanRedo()) {
            if (ImGui::Button("Redo")) {
                editor->Redo();
            }
            ImGui::SameLine();
        }
    }
    ImGui::End();
}


