//------------------------------------------------------------------------------
//  ui_asm.cc
//
//  UI code for the integrated assembler.
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "TextEditor.h"

static TextEditor* editor;

void ui_asm_init(void) {
    editor = new TextEditor();
    editor->SetPalette(TextEditor::GetRetroBluePalette());
    editor->SetShowWhitespaces(false);
    editor->SetTabSize(8);
    editor->SetImGuiChildIgnored(true);
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
        ImGui::Text("%6d/%-6d %6d lines  | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor->GetTotalLines(),
            editor->IsOverwrite() ? "Ovr" : "Ins",
            editor->CanUndo() ? "*" : " ");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(editor->GetPalette()[(int)TextEditor::PaletteIndex::Background]));
        ImGui::BeginChild("##asm", {0,-footer_h}, false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
        editor->Render("Assembler");
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
