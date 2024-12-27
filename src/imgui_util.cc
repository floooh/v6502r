#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_util.h"
#include "ui.h"
#include "util.h"

static void imgui_ClearAllFn(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    (void)ctx; (void)handler;
}

// read: Called before reading (in registration order)
static void imgui_ReadInitFn(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    (void)ctx; (void)handler;
}

// read: Called when entering into a new ini entry e.g. "[Window][Name]"
static void* imgui_ReadOpenFn(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) {
    (void)ctx; (void)handler;
    return (void*)ui_window_by_id(name);
}

// read: Called for every line of text within an ini entry
static void imgui_ReadLineFn(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
    (void)ctx; (void)handler; (void)entry;
    ui_window_t win = (ui_window_t)(uintptr_t)entry;
    if ((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM)) {
        int is_open = 0;
        if (sscanf(line, "IsOpen=%i", &is_open) == 1) {
            ui_init_window_open(win, true);
        }
    }
}

// read: Called after reading (in registration order)
static void imgui_ApplyAllFn(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    (void)ctx; (void)handler;
}

// write: output all entries into 'out_buf'
static void imgui_WriteAllFn(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
    (void)ctx; (void)handler;
    for (int i = UI_WINDOW_INVALID + 1; i < UI_WINDOW_NUM; i++) {
        ui_window_t win = (ui_window_t)i;
        buf->appendf("[%s][%s]\n", handler->TypeName, ui_window_id(win));
        if (ui_is_window_open(win)) {
            buf->append("IsOpen=1\n");
        }
    }
    buf->append("\n");
}

void imgui_register_settings_handler(void) {
    const char* type_name = "floooh.v6502r";
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = type_name;;
    ini_handler.TypeHash = ImHashStr(type_name);
    ini_handler.ClearAllFn = imgui_ClearAllFn;
    ini_handler.ReadInitFn = imgui_ReadInitFn;
    ini_handler.ReadOpenFn = imgui_ReadOpenFn;
    ini_handler.ReadLineFn = imgui_ReadLineFn;
    ini_handler.ApplyAllFn = imgui_ApplyAllFn;
    ini_handler.WriteAllFn = imgui_WriteAllFn;
    ImGui::AddSettingsHandler(&ini_handler);
}
