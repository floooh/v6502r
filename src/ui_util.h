#pragma once
#include <vector>
#include <string>
#include "TextEditor.h"

// helper functions to emulate old TextEditor methods
std::vector<std::string> ui_editor_get_text_lines(TextEditor* editor);
void ui_editor_move_to_end(TextEditor* editor);
TextEditor::Palette ui_editor_retro_blue_palette(void);
const TextEditor::Language* ui_editor_asm_language_def();
