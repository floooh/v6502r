//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "imgui.h"
#include "imgui_internal.h"
#include "res/fonts.h"
#include "res/iconsfontawesome4_c.h"
#include "res/markdown.h"
#include "imgui_markdown/imgui_markdown.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"
#include "sokol_gfx_imgui.h"

#define CHIPS_IMPL
#define CHIPS_UI_IMPL
#if defined(CHIP_6502) || defined(CHIP_2A03)
#define UI_DASM_USE_M6502
#include "chips/m6502dasm.h"
#elif defined(CHIP_Z80)
#define UI_DASM_USE_Z80
#include "chips/z80dasm.h"
#endif
#include "chips/ui_memedit.h"
#include "chips/ui_util.h"
#include "chips/ui_dasm.h"

#include "ui.h"
#include "ui_asm.h"
#include "asm.h"
#include "gfx.h"
#include "sim.h"
#include "pick.h"
#include "trace.h"
#include "util.h"
#include "imgui_util.h"
#include "TextEditor.h"
#include "nodenames.h"

#include <math.h>   // sinf, fmodf
#include <string.h> // strncpy

#ifdef _MSC_VER
#pragma warning (disable: 4566) // character represented by universal-character-name '\u...' cannot be represented in the current code page
#endif

enum {
    TRACELOG_HOVERED = (1<<0),
    TIMINGDIAGRAM_HOVERED = (1<<1),
};

static const ImU32 ui_trace_hovered_color = 0xFF3643F4;
static const ImU32 ui_trace_selected_color = 0xFF0000D5;
static const ImU32 ui_trace_diff_selected_color = 0xFF8843F4;
static const ImU32 ui_trace_bg_colors[4] = { 0xFF327D2E, 0xFF3C8E38, 0xFFC06515, 0xFFD17619 };
static const ImU32 ui_diagram_line_color = 0xFFEEEEEE;
static const ImU32 ui_diagram_text_color = 0xFFFFFFFF;
static const ImU32 ui_diagram_text_bg_color = 0x88000000;
static const ImU32 ui_found_text_color = 0xFF00FF00;
static const ImU32 ui_notfound_text_color = 0xFF4488FF;

#define MAX_NODEEXPLORER_TOKENBUFFER_SIZE (1024)
#define MAX_NODEEXPLORER_HOVERED_NODES (32)
#define MAX_TRACELOG_COLUMNS (64)

typedef struct {
    const char* label;
    ImGuiTableColumnFlags flags;
    int width;
} ui_tracelog_column_t;

#if defined(CHIP_6502)
#define UI_TRACELOG_NUM_COLUMNS (19)
static const ui_tracelog_column_t ui_tracelog_columns[UI_TRACELOG_NUM_COLUMNS] = {
    { "Cycle/h", ImGuiTableColumnFlags_None, 7 },
    { "SYNC", ImGuiTableColumnFlags_NoClip, 4 },
    { "RW", ImGuiTableColumnFlags_NoClip, 2 },
    { "AB", ImGuiTableColumnFlags_NoClip, 4 },
    { "DB", ImGuiTableColumnFlags_NoClip, 2 },
    { "PC", ImGuiTableColumnFlags_NoClip, 4 },
    { "OP", ImGuiTableColumnFlags_NoClip, 2 },
    { "A", ImGuiTableColumnFlags_NoClip, 2 },
    { "X", ImGuiTableColumnFlags_NoClip, 2 },
    { "Y", ImGuiTableColumnFlags_NoClip, 2 },
    { "S", ImGuiTableColumnFlags_NoClip, 2 },
    { "P", ImGuiTableColumnFlags_NoClip, 2 },
    { "Watch", ImGuiTableColumnFlags_NoClip, 5 },
    { "Flags", ImGuiTableColumnFlags_NoClip, 8 },
    { "IRQ", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "NMI", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "RES", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "RDY", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "Asm", ImGuiTableColumnFlags_NoClip, 8 },
};
#elif defined(CHIP_Z80)
#define UI_TRACELOG_NUM_COLUMNS (36)
static const ui_tracelog_column_t ui_tracelog_columns[UI_TRACELOG_NUM_COLUMNS] = {
    { "Cycle/h", ImGuiTableColumnFlags_None, 7 },
    { "M", ImGuiTableColumnFlags_DefaultHide, 2 },
    { "T", ImGuiTableColumnFlags_DefaultHide, 2 },
    { "M1", ImGuiTableColumnFlags_NoClip, 2 },
    { "MREQ", ImGuiTableColumnFlags_NoClip, 4 },
    { "IORQ", ImGuiTableColumnFlags_NoClip, 4 },
    { "RFSH", ImGuiTableColumnFlags_NoClip, 4 },
    { "RD", ImGuiTableColumnFlags_NoClip, 2 },
    { "WR", ImGuiTableColumnFlags_NoClip, 2 },
    { "INT", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "NMI", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "WAIT", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "HALT", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "AB", ImGuiTableColumnFlags_NoClip, 4 },
    { "DB", ImGuiTableColumnFlags_NoClip, 2 },
    { "PC", ImGuiTableColumnFlags_NoClip, 4 },
    { "OP", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 2 },
    { "AF", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "BC", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "DE", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "HL", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "AF'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "BC'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "DE'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "HL'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "IX", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "IY", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "SP", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "WZ", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "I", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 2 },
    { "R", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 2 },
    { "IM", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 2 },
    { "IFF1", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4 },
    { "Watch", ImGuiTableColumnFlags_NoClip, 5 },
    { "Flags", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 8 },
    { "Asm", ImGuiTableColumnFlags_NoClip, 12 }
};
#elif defined(CHIP_2A03)
#define UI_TRACELOG_NUM_COLUMNS (24)
static const ui_tracelog_column_t ui_tracelog_columns[UI_TRACELOG_NUM_COLUMNS] = {
    { "Cycle/h", ImGuiTableColumnFlags_None, 7 },
    { "SYNC", ImGuiTableColumnFlags_NoClip, 4 },
    { "RW", ImGuiTableColumnFlags_NoClip, 2 },
    { "AB", ImGuiTableColumnFlags_NoClip, 4 },
    { "DB", ImGuiTableColumnFlags_NoClip, 2 },
    { "PC", ImGuiTableColumnFlags_NoClip, 4 },
    { "OP", ImGuiTableColumnFlags_NoClip, 2 },
    { "A", ImGuiTableColumnFlags_NoClip, 2 },
    { "X", ImGuiTableColumnFlags_NoClip, 2 },
    { "Y", ImGuiTableColumnFlags_NoClip, 2 },
    { "S", ImGuiTableColumnFlags_NoClip, 2 },
    { "P", ImGuiTableColumnFlags_NoClip, 2 },
    { "Watch", ImGuiTableColumnFlags_NoClip, 5 },
    { "Flags", ImGuiTableColumnFlags_NoClip, 8 },
    { "IRQ", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "NMI", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "RES", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "RDY", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "Asm", ImGuiTableColumnFlags_NoClip, 8 },
    { "Sq0", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "Sq1", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "Tri", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "Noi", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
    { "Pcm", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3 },
};
#endif

#define MAX_TRACEDUMP_SIZE ((MAX_TRACE_ITEMS+2) * UI_TRACELOG_NUM_COLUMNS * 16)

static struct {
    bool valid;
    ui_memedit_t memedit;
    #if defined(CHIP_Z80)
    ui_memedit_t ioedit;
    #endif
    ui_dasm_t dasm;
    bool window_open[UI_WINDOW_NUM];
    bool prev_window_open[UI_WINDOW_NUM];
    struct {
        bool open_source_hovered;
        bool save_source_hovered;
        bool save_binary_hovered;
        bool save_listing_hovered;
        bool save_tracelog_hovered;
        bool cut_hovered;
        bool copy_hovered;
        int layer_slider;
    } menu;
    struct {
        int hovered_flags;
        bool diff_view_active;
        bool is_selected;
        bool is_diff_selected;
        uint32_t hovered_cycle;
        uint32_t selected_cycle;
        uint32_t diff_selected_cycle;
        bool watch_node_valid;
        char watch_str[32];
        uint32_t watch_node_index;
        bool column_visible[UI_TRACELOG_NUM_COLUMNS];
    } trace;
    struct {
        TextEditor* editor;
        bool window_focused;
        bool explorer_mode_active;
        int num_hovered_nodes;
        uint32_t hovered_node_indices[MAX_NODEEXPLORER_HOVERED_NODES];
        uint8_t selected[MAX_NODES];
        char token_buffer[MAX_NODEEXPLORER_TOKENBUFFER_SIZE];
    } explorer;
    ui_window_t keyboard_target;
    bool link_hovered;
    char link_url[MAX_LINKURL_SIZE];
    struct {
        int pos;
        char buf[MAX_TRACEDUMP_SIZE];
    } tracedump;
} ui;

static void ui_handle_docking(void);
static void ui_menu(void);
static void ui_picking(void);
static void ui_floating_controls(void);
static void ui_tracelog_timingdiagram_begin(void);
static void ui_tracelog(void);
static void ui_timingdiagram(void);
static void ui_tracelog_timingdiagram_end(void);
static void ui_nodeexplorer_init(void);
static void ui_nodeexplorer_discard(void);
static void ui_nodeexplorer_undo(void);
static void ui_nodeexplorer_redo(void);
static void ui_nodeexplorer_cut(void);
static void ui_nodeexplorer_copy(void);
static void ui_nodeexplorer_paste(void);
static void ui_nodeexplorer(void);
static void ui_cassette_deck_controls(void);
static void ui_cpu_controls(void);
static void ui_listing(void);
static void ui_memedit(void);
static void ui_dasm(void);
static void ui_help_assembler(void);
static void ui_help_opcodes(void);
static void ui_help_about(void);
static void ui_handle_save_settings(void);
static void ui_load_settings(void);
static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data);
#if defined(CHIP_2A03)
static void ui_audio_controls(void);
#endif
#if defined(CHIP_Z80)
static void ui_ioedit(void);
#endif

static ImGui::MarkdownConfig md_conf;

void ui_check_dirty(ui_window_t win) {
    assert((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    if (ui.window_open[win] != ui.prev_window_open[win]) {
        ui.prev_window_open[win] = ui.window_open[win];
        ImGui::MarkIniSettingsDirty();
    }
}

void ui_init_window_open(ui_window_t win, bool state) {
    assert((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    ui.window_open[win] = state;
    ui.prev_window_open[win] = state;
}

void ui_set_window_open(ui_window_t win, bool state) {
    assert((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    ui.window_open[win] = state;
}

bool ui_is_window_open(ui_window_t win) {
    assert((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    return ui.window_open[win];
}

void ui_toggle_window(ui_window_t win) {
    assert((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    ui.window_open[win] = !ui.window_open[win];
}

bool* ui_window_open_ptr(ui_window_t win) {
    assert((win > UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    return &ui.window_open[win];
}

void ui_set_keyboard_target(ui_window_t win) {
    assert((win >= UI_WINDOW_INVALID) && (win < UI_WINDOW_NUM));
    ui.keyboard_target = win;
}

bool ui_pre_check(ui_window_t win) {
    ui_check_dirty(win);
    return ui_is_window_open(win);
}

const char* ui_window_id(ui_window_t win) {
    switch (win) {
        case UI_WINDOW_FLOATING_CONTROLS: return "##floating";
        case UI_WINDOW_CPU_CONTROLS: return "CPU Controls";
        case UI_WINDOW_TRACELOG: return "Trace Log";
        case UI_WINDOW_TIMING_DIAGRAM: return "Timing Diagram";
        case UI_WINDOW_NODE_EXPLORER: return "Node Explorer";
        case UI_WINDOW_LISTING: return "Listing";
        case UI_WINDOW_ASM: return "Assembler";
        case UI_WINDOW_MEMEDIT: return "Memory Editor";
        case UI_WINDOW_DASM: return "Disassembler";
        case UI_WINDOW_HELP_ASM: return "Assembler Help";
        case UI_WINDOW_HELP_OPCODES: return "Opcode Help";
        case UI_WINDOW_HELP_ABOUT: return "About";
        #if defined(CHIP_Z80)
        case UI_WINDOW_IOEDIT: return "IO Editor";
        #endif
        #if defined(CHIP_2A03)
        case UI_WINDOW_AUDIO: return "Audio";
        #endif
        default: assert(false); return "INVALID";
    }
}

const char* ui_window_name(ui_window_t win) {
    switch (win) {
        case UI_WINDOW_FLOATING_CONTROLS: return "Floating Controls";
        case UI_WINDOW_HELP_ASM: return "Assembler";
        case UI_WINDOW_HELP_OPCODES: return "Opcode";
        default: return ui_window_id(win);
    }
}

ui_window_t ui_window_by_id(const char* id) {
    assert(id);
    for (int i = UI_WINDOW_INVALID + 1; i < UI_WINDOW_NUM; i++) {
        ui_window_t win = (ui_window_t)i;
        if (0 == strcmp(ui_window_id(win), id)) {
            return win;
        }
    }
    return UI_WINDOW_INVALID;
}

// read and write callback for memory/io editor windows
static uint8_t ui_mem_read(int /*layer*/, uint16_t addr, void* /*user_data*/) {
    return sim_mem_r8(addr);
}

static void ui_mem_write(int /*layer*/, uint16_t addr, uint8_t data, void* /*user_data*/) {
    sim_mem_w8(addr, data);
}

#if defined(CHIP_Z80)
static uint8_t ui_io_read(int /*layer*/, uint16_t addr, void* /*user_data*/) {
    return sim_io_r8(addr);
}

static void ui_io_write(int /*layer*/, uint16_t addr, uint8_t data, void* /*user_data*/) {
    sim_io_w8(addr, data);
}
#endif

static const ImVec4 dim(const ImVec4 c, float d) {
    return ImVec4(c.x*d, c.y*d, c.z*d, c.w);
}

void ui_init() {
    assert(!ui.valid);

    // setup sokol-gfx debugging UI
    const sgimgui_desc_t sgimgui_desc = {};
    sgimgui_setup(&sgimgui_desc);

    // default window open state
    ui_init_window_open(UI_WINDOW_FLOATING_CONTROLS, true);
    ui_nodeexplorer_init();

    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_desc.no_default_font = true;
    simgui_desc.disable_paste_override = true;
    simgui_setup(&simgui_desc);
    imgui_register_settings_handler();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_Border] = dim(style.Colors[ImGuiCol_TitleBgActive], 0.75f);
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_TableBorderLight] = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_TableBorderStrong] = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_MenuBarBg] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_ChildBg] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_PopupBg] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_ScrollbarBg] = dim(style.Colors[ImGuiCol_WindowBg], 0.85f);

    // setup ImGui font with custom icons
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontDefault();
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)dump_fontawesome_ttf,
        sizeof(dump_fontawesome_ttf),
        16.0f, &icons_config, icons_ranges);
    ImFontConfig h1Conf;
    h1Conf.SizePixels = 26.0f;
    md_conf.headingFormats[0].font = io.Fonts->AddFontDefault(&h1Conf);

    // initialize helper windows from the chips projects
    {
        ui_memedit_desc_t desc = { };
        desc.title = ui_window_id(UI_WINDOW_MEMEDIT);
        desc.open = false;
        desc.num_cols = 8;
        desc.h = 300;
        desc.x = 50;
        desc.y = 50;
        desc.hide_ascii = true;
        desc.read_cb = ui_mem_read;
        desc.write_cb = ui_mem_write;
        ui_memedit_init(&ui.memedit, &desc);
    }
    #if defined(CHIP_Z80)
    {
        ui_memedit_desc_t desc = { };
        desc.title = ui_window_id(UI_WINDOW_IOEDIT);
        desc.open = false;
        desc.num_cols = 8;
        desc.h = 300;
        desc.x = 60;
        desc.y = 60;
        desc.hide_ascii = true;
        desc.read_cb = ui_io_read;
        desc.write_cb = ui_io_write;
        ui_memedit_init(&ui.ioedit, &desc);
    }
    #endif
    {
        ui_dasm_desc_t desc = { };
        desc.title = ui_window_id(UI_WINDOW_DASM);
        desc.open = false;
        #if defined(CHIP_6502) || defined(CHIP_2A03)
        desc.cpu_type = UI_DASM_CPUTYPE_M6502;
        #elif defined(CHIP_Z80)
        desc.cpu_type = UI_DASM_CPUTYPE_Z80;
        #endif
        desc.start_addr = 0;
        desc.read_cb = ui_mem_read;
        desc.x = 50;
        desc.y = 50;
        desc.w = 450;
        ui_dasm_init(&ui.dasm, &desc);
    }
    ui_asm_init();

    // setup ImGui::Markdown configuration
    md_conf.linkCallback = markdown_link_callback;
    md_conf.headingFormats[0].separator = true;
    md_conf.headingFormats[0].newline_above = false;
    md_conf.headingFormats[0].newline_below = false;
    md_conf.headingFormats[1].newline_below = false;
    md_conf.linkIcon = ICON_FA_LINK;

    ui.keyboard_target = UI_WINDOW_INVALID;
    ui.menu.layer_slider = MAX_LAYERS;
    ui_load_settings();
    ui.valid = true;
}

void ui_shutdown() {
    assert(ui.valid);
    sgimgui_shutdown();
    ui_nodeexplorer_discard();
    ui_asm_discard();
    ui_dasm_discard(&ui.dasm);
    ui_memedit_discard(&ui.memedit);
    #if defined(CHIP_Z80)
    ui_memedit_discard(&ui.ioedit);
    #endif
    simgui_shutdown();
}

static bool test_alt(const sapp_event* ev, sapp_keycode key_code) {
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if ((ev->modifiers == SAPP_MODIFIER_ALT) && (ev->key_code == key_code)) {
            return true;
        }
    }
    return false;
}

static bool test_ctrl(const sapp_event* ev, sapp_keycode key_code) {
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        uint32_t mod = util_is_osx() ? SAPP_MODIFIER_SUPER : SAPP_MODIFIER_CTRL;
        if ((ev->modifiers == mod) && (ev->key_code == key_code)) {
            return true;
        }
    }
    return false;
}

static bool test_ctrl_shift(const sapp_event* ev, sapp_keycode key_code) {
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        uint32_t mod = SAPP_MODIFIER_SHIFT | (util_is_osx() ? SAPP_MODIFIER_SUPER : SAPP_MODIFIER_CTRL);
        if ((ev->modifiers == mod) && (ev->key_code == key_code)) {
            return true;
        }
    }
    return false;
}

static bool test_click(const sapp_event* ev, bool hovered) {
    if (hovered && (ev->type == SAPP_EVENTTYPE_MOUSE_UP) && (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT)) {
        return true;
    }
    else {
        return false;
    }
}

static bool handle_special_link(void) {
    if (0 == strcmp(ui.link_url, "ui://listing")) {
        ui_set_window_open(UI_WINDOW_LISTING, true);
        ImGui::SetWindowFocus("Listing");
        return true;
    } else if (0 == strcmp(ui.link_url, "ui://assembler")) {
        ui_set_window_open(UI_WINDOW_ASM, true);
        ImGui::SetWindowFocus("Assembler");
        return true;
    }
    return false;
}

static void ui_undo(void) {
    switch (ui.keyboard_target) {
        case UI_WINDOW_ASM: ui_asm_undo(); break;
        case UI_WINDOW_NODE_EXPLORER: ui_nodeexplorer_undo(); break;
        default: break;
    }
}

static void ui_redo(void) {
    switch (ui.keyboard_target) {
        case UI_WINDOW_ASM: ui_asm_redo(); break;
        case UI_WINDOW_NODE_EXPLORER: ui_nodeexplorer_redo(); break;
        default: break;
    }
}

static void ui_cut(void) {
    switch (ui.keyboard_target) {
        case UI_WINDOW_ASM: ui_asm_cut(); break;
        case UI_WINDOW_NODE_EXPLORER: ui_nodeexplorer_cut(); break;
        default: break;
    }
}

static void ui_copy(void) {
    switch (ui.keyboard_target) {
        case UI_WINDOW_ASM: ui_asm_copy(); break;
        case UI_WINDOW_NODE_EXPLORER: ui_nodeexplorer_copy(); break;
        default: break;
    }
}

static void ui_paste(void) {
    switch (ui.keyboard_target) {
        case UI_WINDOW_ASM: ui_asm_paste(); break;
        case UI_WINDOW_NODE_EXPLORER: ui_nodeexplorer_paste(); break;
        default: break;
    }
}

bool ui_handle_input(const sapp_event* ev) {
    assert(ui.valid);
    int l = -1;
    if (test_click(ev, ui.link_hovered)) {
        ui.link_hovered = false;
        if (!handle_special_link()) {
            util_html5_open_link(ui.link_url);
        }
    }
    if (test_alt(ev, SAPP_KEYCODE_1)) {
        l = 0;
    }
    else if (test_alt(ev, SAPP_KEYCODE_2)) {
        l = 1;
    }
    else if (test_alt(ev, SAPP_KEYCODE_3)) {
        l = 2;
    }
    else if (test_alt(ev, SAPP_KEYCODE_4)) {
        l = 3;
    }
    else if (test_alt(ev, SAPP_KEYCODE_5)) {
        l = 4;
    }
    else if (test_alt(ev, SAPP_KEYCODE_6)) {
        l = 5;
    }
    if (l != -1) {
        gfx_toggle_layer_visibility(l);
        return true;
    }
    if (test_alt(ev, SAPP_KEYCODE_F)) {
        ui_toggle_window(UI_WINDOW_FLOATING_CONTROLS);
    }
    if (test_alt(ev, SAPP_KEYCODE_C)) {
        ui_toggle_window(UI_WINDOW_CPU_CONTROLS);
    }
    if (test_alt(ev, SAPP_KEYCODE_T)) {
        ui_toggle_window(UI_WINDOW_TRACELOG);
    }
    if (test_alt(ev, SAPP_KEYCODE_A)) {
        ui_toggle_window(UI_WINDOW_ASM);
    }
    if (test_alt(ev, SAPP_KEYCODE_L)) {
        ui_toggle_window(UI_WINDOW_LISTING);
    }
    if (test_alt(ev, SAPP_KEYCODE_M)) {
        ui_toggle_window(UI_WINDOW_MEMEDIT);
    }
    if (test_alt(ev, SAPP_KEYCODE_D)) {
        ui_toggle_window(UI_WINDOW_DASM);
    }
    if (test_alt(ev, SAPP_KEYCODE_E)) {
        ui_toggle_window(UI_WINDOW_NODE_EXPLORER);
    }
    if (test_click(ev, ui.menu.open_source_hovered) || test_ctrl_shift(ev, SAPP_KEYCODE_O)) {
        util_html5_load();
        ui.menu.open_source_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.save_source_hovered) || test_ctrl_shift(ev, SAPP_KEYCODE_S)) {
        util_html5_download_string("source.asm", ui_asm_source());
        ui.menu.save_source_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.save_listing_hovered)) {
        util_html5_download_string("listing.lst", asm_listing());
        ui.menu.save_listing_hovered = false;
    }
    if (test_click(ev, ui.menu.save_binary_hovered)) {
        util_html5_download_binary("binary.bin", ui_asm_get_binary());
        ui.menu.save_binary_hovered = false;
    }
    if (test_click(ev, ui.menu.save_tracelog_hovered)) {
        util_html5_download_string("tracelog.txt", ui_trace_get_dump());
        ui.menu.save_tracelog_hovered = false;
    }
    if (test_ctrl(ev, SAPP_KEYCODE_Z) || test_ctrl(ev, SAPP_KEYCODE_Y)) {
        ui_undo();
        sapp_consume_event();
    }
    if (test_ctrl_shift(ev, SAPP_KEYCODE_Z) || test_ctrl_shift(ev, SAPP_KEYCODE_Y)) {
        ui_redo();
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.cut_hovered) || test_ctrl(ev, SAPP_KEYCODE_X)) {
        ui_cut();
        ui.menu.cut_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.copy_hovered) || test_ctrl(ev, SAPP_KEYCODE_C)) {
        ui_copy();
        ui.menu.copy_hovered = false;
        sapp_consume_event();
    }
    if (ev->type == SAPP_EVENTTYPE_CLIPBOARD_PASTED) {
        ui_paste();
    }
    if (0 != (ev->modifiers & (SAPP_MODIFIER_CTRL|SAPP_MODIFIER_ALT|SAPP_MODIFIER_SUPER))) {
        return false;
    }
    else {
        // sokol_imgui.h returns true if either WantsCaptureKeyboard or
        // WantsCaptureMouse is set which is too broad in our case,
        // we just need to prevent further mouse input handling when
        // Dear ImGui wants to capture the mouse
        simgui_handle_event(ev);
        return ImGui::GetIO().WantCaptureMouse;
    }
}

void ui_frame() {
    assert(ui.valid);
    ui.link_hovered = false;
    simgui_new_frame({ sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });
    ui_handle_docking();
    ui_menu();
    ui_floating_controls();
    ui_picking();
    ui_memedit();
    #if defined(CHIP_Z80)
    ui_ioedit();
    #endif
    ui_dasm();
    ui_tracelog_timingdiagram_begin();
    ui_tracelog();
    ui_timingdiagram();
    ui_tracelog_timingdiagram_end();
    ui_nodeexplorer();
    ui_cpu_controls();
    #if defined(CHIP_2A03)
    ui_audio_controls();
    #endif
    ui_asm_draw();
    ui_listing();
    ui_help_assembler();
    ui_help_opcodes();
    ui_help_about();
    if (ui.link_hovered) {
        sapp_set_mouse_cursor(SAPP_MOUSECURSOR_POINTING_HAND);
    }
    ui_handle_save_settings();
}

void ui_draw() {
    assert(ui.valid);
    sgimgui_draw();
    simgui_render();
}

static void ui_handle_docking(void) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiID dspace_id = ImGui::GetID("main_dockspace");
    ImGui::DockSpaceOverViewport(dspace_id, viewport, ImGuiDockNodeFlags_NoDockingOverCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);
}

static void ui_menu(void) {
    ui.menu.open_source_hovered = false;
    ui.menu.save_source_hovered = false;
    ui.menu.save_binary_hovered = false;
    ui.menu.save_listing_hovered = false;
    ui.menu.save_tracelog_hovered = false;
    if (ImGui::BeginMainMenuBar()) {
        bool is_osx = util_is_osx();
        if (ImGui::BeginMenu("File")) {
            // this looks all a bit weired because on the web platforms
            // these actions must be invoked from within an input handler
            ImGui::MenuItem("Open Source...", is_osx?"Cmd+Shift+O":"Ctrl+Shift+O");
            ui.menu.open_source_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save Source...", is_osx?"Cmd+Shift+S":"Ctrl+Shift+S");
            ui.menu.save_source_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save .BIN/.PRG...", 0);
            ui.menu.save_binary_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save Listing...", 0);
            ui.menu.save_listing_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save Tracelog...", 0);
            ui.menu.save_tracelog_hovered = ImGui::IsItemHovered();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", is_osx?"Cmd+Z|Y":"Ctrl+Z|Y")) {
                ui_undo();
            }
            if (ImGui::MenuItem("Redo", is_osx?"Cmd+Shift+Z|Y":"Ctrl+Shift+Z|Y")) {
                ui_redo();
            }
            ImGui::Separator();
            ImGui::MenuItem("Cut", is_osx?"Cmd+X":"Ctrl+X");
            ui.menu.cut_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Copy", is_osx?"Cmd+C":"Ctrl+C");
            ui.menu.copy_hovered = ImGui::IsItemHovered();
            #if defined(__EMSCRIPTEN__)
            ImGui::MenuItem("Paste", is_osx?"Cmd+V":"Ctrl+V");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Please use keyboard shortcut or\nbrowser menu for pasting!");
            }
            #else
            if (ImGui::MenuItem("Paste", is_osx?"Cmd+V":"Ctrl+V")) {
                ui_paste();
            }
            #endif
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem(ui_window_name(UI_WINDOW_FLOATING_CONTROLS), "Alt+F", ui_window_open_ptr(UI_WINDOW_FLOATING_CONTROLS));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_CPU_CONTROLS), "Alt+C", ui_window_open_ptr(UI_WINDOW_CPU_CONTROLS));
            #if defined(CHIP_2A03)
            ImGui::MenuItem(ui_window_name(UI_WINDOW_AUDIO), nullptr, ui_window_open_ptr(UI_WINDOW_AUDIO));
            #endif
            ImGui::MenuItem(ui_window_name(UI_WINDOW_NODE_EXPLORER), "Alt+E", ui_window_open_ptr(UI_WINDOW_NODE_EXPLORER));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_TRACELOG), "Alt+T", ui_window_open_ptr(UI_WINDOW_TRACELOG));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_TIMING_DIAGRAM), nullptr, ui_window_open_ptr(UI_WINDOW_TIMING_DIAGRAM));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_LISTING), "Alt+L", ui_window_open_ptr(UI_WINDOW_LISTING));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_MEMEDIT), "Alt+M", ui_window_open_ptr(UI_WINDOW_MEMEDIT));
            #if defined(CHIP_Z80)
            ImGui::MenuItem(ui_window_name(UI_WINDOW_IOEDIT), nullptr, ui_window_open_ptr(UI_WINDOW_IOEDIT));
            #endif
            ImGui::MenuItem(ui_window_name(UI_WINDOW_DASM), "Alt+D", ui_window_open_ptr(UI_WINDOW_DASM));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_ASM), "Alt+A", ui_window_open_ptr(UI_WINDOW_ASM));
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Layers")) {
            if (ImGui::BeginMenu("Visible")) {
                const char* label[MAX_LAYERS] = {
                    "Layer #1##V", "Layer #2##V", "Layer #3##V", "Layer #4##V", "Layer #5##V", "Layer #6##V",
                };
                const char* hotkey[MAX_LAYERS] = {
                    "Alt+1", "Alt+2", "Alt+3", "Alt+4", "Alt+5", "Alt+6"
                };
                for (int i = 0; i < MAX_LAYERS; i++) {
                    if (ImGui::MenuItem(label[i], hotkey[i], gfx_get_layer_visibility(i))) {
                        gfx_toggle_layer_visibility(i);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Pickable")) {
                const char* label[MAX_LAYERS] = {
                    "Layer #1##P", "Layer #2##P", "Layer #3##P", "Layer #4##P", "Layer #5##P", "Layer #6##P",
                };
                for (int i = 0; i < MAX_LAYERS; i++) {
                    if (ImGui::MenuItem(label[i], 0, pick_get_layer_enabled(i))) {
                        pick_toggle_layer_enabled(i);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Remix")) {
                gfx_set_layer_palette(false, gfx_default_palette);
            }
            if (ImGui::MenuItem("Original")) {
                gfx_set_layer_palette(false, {
                    {
                        { 1.0f, 0.0f, 0.0f, 1.0f },
                        { 1.0f, 1.0f, 0.0f, 1.0f },
                        { 1.0f, 0.0f, 1.0f, 1.0f },
                        { 0.3f, 1.5f, 0.3f, 1.0f },
                        { 1.0f, 0.3f, 0.3f, 1.0f },
                        { 0.5f, 0.1f, 0.75f, 1.0f },
                    },
                    { 0.0f, 0.0f, 0.0f, 1.0f }
                });
            }
            if (ImGui::MenuItem("Contrast")) {
                gfx_set_layer_palette(false, {
                    {
                        { 1.0f, 0.0f, 0.0f, 1.0f },
                        { 0.8f, 0.5f, 0.0f, 1.0f },
                        { 1.0f, 0.0f, 1.0f, 1.0f },
                        { 0.3f, 1.0f, 0.3f, 1.0f },
                        { 1.0f, 0.3f, 0.3f, 1.0f },
                        { 0.5f, 0.1f, 0.75f, 1.0f },
                    },
                    { 1.0f, 1.0f, 1.0f, 1.0f }
                });
            }
            if (ImGui::MenuItem("Matrix")) {
                gfx_set_layer_palette(true, {
                    {
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                    },
                    { 0.0f, 0.0f, 0.0f, 1.0f }
                });
            }
            if (ImGui::MenuItem("X-Ray")) {
                gfx_set_layer_palette(true, {
                    {
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                    },
                    { 0.0f, 0.0f, 0.0f, 1.0f }
                });
            }
            ImGui::EndMenu();
        }
        sgimgui_draw_menu("Sokol");
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem(ui_window_name(UI_WINDOW_HELP_ASM), 0, ui_window_open_ptr(UI_WINDOW_HELP_ASM));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_HELP_OPCODES), 0, ui_window_open_ptr(UI_WINDOW_HELP_OPCODES));
            ImGui::MenuItem(ui_window_name(UI_WINDOW_HELP_ABOUT), 0, ui_window_open_ptr(UI_WINDOW_HELP_ABOUT));
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void ui_floating_controls(void) {
    if (!ui_pre_check(UI_WINDOW_FLOATING_CONTROLS)) {
        return;
    }
    ImGui::SetNextWindowPos({ 10, 20 }, ImGuiCond_FirstUseEver);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin(ui_window_id(UI_WINDOW_FLOATING_CONTROLS), ui_window_open_ptr(UI_WINDOW_FLOATING_CONTROLS), flags)) {
        if (!ui_is_window_open(UI_WINDOW_CPU_CONTROLS)) {
            ui_cassette_deck_controls();
        }
        if (ImGui::SliderInt("Layers", &ui.menu.layer_slider, 0, MAX_LAYERS)) {
            int i = 0;
            for (; i < ui.menu.layer_slider; i++) {
                gfx_set_layer_visibility(i, true);
                pick_set_layer_enabled(i, true);
            }
            for (; i < MAX_LAYERS; i++) {
                gfx_set_layer_visibility(i, false);
                pick_set_layer_enabled(i, false);
            }
        }
        float scale = gfx_get_scale();
        if (ImGui::SliderFloat("Scale", &scale, gfx_min_scale(), gfx_max_scale(), "%.2f", ImGuiSliderFlags_Logarithmic)) {
            gfx_set_scale(scale);
        }
    }
    ImGui::End();
}

#if defined(CHIP_6502) || defined(CHIP_2A03)
static void ui_input_6502_vec(const char* label, const char* id, uint16_t addr) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label); ImGui::SameLine();
    sim_mem_w16(addr, ui_util_input_u16(id, sim_mem_r16(addr)));
}
#endif

#if defined(CHIP_Z80)
static void ui_input_z80_intvec(const char* label, const char* id) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label); ImGui::SameLine();
    sim_z80_set_intvec(ui_util_input_u8(id, sim_z80_get_intvec()));
}
#endif

static const char* ui_cpu_flags_as_string(uint8_t flags, char* buf, size_t buf_size) {
    assert(buf && (buf_size >= 9)); (void)buf_size;
    #if defined(CHIP_6502) || defined(CHIP_2A03)
    const char* chrs[2] = { "czidbxvn", "CZIDBXVN" };
    #elif defined(CHIP_Z80)
    const char* chrs[2] = { "cnvxhyzs", "CNVXHYZS" };
    #endif
    for (int i = 0; i < 8; i++) {
        buf[7 - i] = chrs[(flags>>i) & 1][i];
    }
    buf[8] = 0;
    return buf;
}

#if defined(CHIP_6502)
static void ui_cpu_status_panel(void) {
    const uint8_t ir = sim_6502_get_op();
    const uint8_t p = sim_get_flags();
    ImGui::Text("A:%02X X:%02X Y:%02X SP:%02X PC:%04X",
        sim_6502_get_a(), sim_6502_get_x(), sim_6502_get_y(), sim_6502_get_sp(), sim_get_pc());
    char p_buf[9];
    ImGui::Text("P:%02X (%s) Cycle: %d", p, ui_cpu_flags_as_string(sim_get_flags(), p_buf, sizeof(p_buf)), sim_get_cycle()>>1);
    ImGui::Text("OP:%02X [%s]\n", ir, trace_get_disasm(0));
    ImGui::Text("Data:%02X Addr:%04X %s %s %s",
        sim_get_data(), sim_get_addr(),
        sim_6502_get_rw()?"R":"W",
        sim_6502_get_clk0()?"CLK0":"clk0",
        sim_6502_get_sync()?"SYNC":"sync");
    ImGui::Separator();
    ui_input_6502_vec("NMI vector (FFFA): ", "##nmi_vec", 0xFFFA);
    ui_input_6502_vec("RES vector (FFFC): ", "##res_vec", 0xFFFC);
    ui_input_6502_vec("IRQ vector (FFFE): ", "##irq_vec", 0xFFFE);
    ImGui::Separator();
    bool rdy_active = !sim_6502_get_rdy();
    if (ImGui::Checkbox("RDY", &rdy_active)) {
        sim_6502_set_rdy(!rdy_active);
    }
    ImGui::SameLine();
    bool irq_active = !sim_6502_get_irq();
    if (ImGui::Checkbox("IRQ", &irq_active)) {
        sim_6502_set_irq(!irq_active);
    }
    ImGui::SameLine();
    bool nmi_active = !sim_6502_get_nmi();
    if (ImGui::Checkbox("NMI", &nmi_active)) {
        sim_6502_set_nmi(!nmi_active);
    }
    ImGui::SameLine();
    bool res_active = !sim_6502_get_res();
    if (ImGui::Checkbox("RES", &res_active)) {
        sim_6502_set_res(!res_active);
    }
}
#endif

#if defined(CHIP_Z80)
static void ui_cpu_status_panel(void) {
    ImGui::Text("AF:%04X  BC:%04X  DE:%04X  HL:%04X", sim_z80_get_af(), sim_z80_get_bc(), sim_z80_get_de(), sim_z80_get_hl());
    ImGui::Text("AF'%04X  BC'%04X  DE'%04X  HL'%04X", sim_z80_get_af2(), sim_z80_get_bc2(), sim_z80_get_de2(), sim_z80_get_hl2());
    ImGui::Text("IX:%04X  IY:%04X  SP:%04X  PC:%04X", sim_z80_get_ix(), sim_z80_get_iy(), sim_z80_get_sp(), sim_z80_get_pc());
    char f_buf[9];
    ImGui::Text("WZ:%04X  IR:%02X%02X  IM:%X  F:%s", sim_z80_get_wz(), sim_z80_get_i(), sim_z80_get_r(), sim_z80_get_im(), ui_cpu_flags_as_string(sim_z80_get_f(), f_buf, sizeof(f_buf)));
    ImGui::Text("OP:%02X [%s]", sim_z80_get_op(), trace_get_disasm(0));
    const char* m_str = "??";
    const char* t_str = "??";
    switch (sim_z80_get_m()) {
        case (1<<0): m_str = "M1"; break;
        case (1<<1): m_str = "M2"; break;
        case (1<<2): m_str = "M3"; break;
        case (1<<3): m_str = "M4"; break;
        case (1<<4): m_str = "M5"; break;
    };
    switch (sim_z80_get_t()) {
        case (1<<0): t_str = "T1"; break;
        case (1<<1): t_str = "T2"; break;
        case (1<<2): t_str = "T3"; break;
        case (1<<3): t_str = "T4"; break;
        case (1<<4): t_str = "T5"; break;
        case (1<<5): t_str = "T6"; break;
    }
    ImGui::Text("Cycle:%s/%s DBus:%02X ABus:%04X %s",
        m_str, t_str, sim_get_data(), sim_get_addr(), sim_z80_get_iff1() ? "IFF1":"iff1");
    ImGui::Text("%s %s %s %s %s %s %s %s\n%s %s %s %s %s %s",
        sim_z80_get_clk() ? "CLK":"clk",
        sim_z80_get_m1() ? "m1":"M1",
        sim_z80_get_mreq() ? "mreq":"MREQ",
        sim_z80_get_iorq() ? "iorq":"IORQ",
        sim_z80_get_rd() ? "rd":"RD",
        sim_z80_get_wr() ? "wr":"WR",
        sim_z80_get_rfsh() ? "rfsh":"RFSH",
        sim_z80_get_halt() ? "halt":"HALT",
        sim_z80_get_wait() ? "wait":"WAIT",
        sim_z80_get_int() ? "int":"INT",
        sim_z80_get_nmi() ? "nmi":"NMI",
        sim_z80_get_reset() ? "reset":"RESET",
        sim_z80_get_busrq() ? "busrq":"BUSRQ",
        sim_z80_get_busak() ? "busak":"BUSAK");
    ImGui::Separator();
    ui_input_z80_intvec("INT vector: ", "##int_vec");
    ImGui::Separator();
    bool wait_active = !sim_z80_get_wait();
    if (ImGui::Checkbox("WAIT", &wait_active)) {
        sim_z80_set_wait(!wait_active);
    }
    ImGui::SameLine();
    bool int_active = !sim_z80_get_int();
    if (ImGui::Checkbox("INT", &int_active)) {
        sim_z80_set_int(!int_active);
    }
    ImGui::SameLine();
    bool nmi_active = !sim_z80_get_nmi();
    if (ImGui::Checkbox("NMI", &nmi_active)) {
        sim_z80_set_nmi(!nmi_active);
    }
    ImGui::SameLine();
    bool res_active = !sim_z80_get_reset();
    if (ImGui::Checkbox("RESET", &res_active)) {
        sim_z80_set_reset(!res_active);
    }
    bool busrq_active = !sim_z80_get_busrq();
    if (ImGui::Checkbox("BUSRQ", &busrq_active)) {
        sim_z80_set_busrq(!busrq_active);
    }
}
#endif

#if defined(CHIP_2A03)
static void ui_cpu_status_panel(void) {
    const uint8_t ir = sim_2a03_get_op();
    const uint8_t p = sim_get_flags();
    ImGui::Text("A:%02X X:%02X Y:%02X SP:%02X PC:%04X",
        sim_2a03_get_a(), sim_2a03_get_x(), sim_2a03_get_y(), sim_2a03_get_sp(), sim_get_pc());
    char p_buf[9];
    ImGui::Text("P:%02X (%s) Cycle: %d", p, ui_cpu_flags_as_string(sim_get_flags(), p_buf, sizeof(p_buf)), sim_get_cycle()>>1);
    ImGui::Text("OP:%02X [%s]\n", ir, trace_get_disasm(0));
    ImGui::Text("Data:%02X Addr:%04X %s %s %s",
        sim_get_data(), sim_get_addr(),
        sim_2a03_get_rw()?"R":"W",
        sim_2a03_get_clk0()?"CLK0":"clk0",
        sim_2a03_get_sync()?"SYNC":"sync");
    ImGui::Separator();
    ui_input_6502_vec("NMI vector (FFFA): ", "##nmi_vec", 0xFFFA);
    ui_input_6502_vec("RES vector (FFFC): ", "##res_vec", 0xFFFC);
    ui_input_6502_vec("IRQ vector (FFFE): ", "##irq_vec", 0xFFFE);
    ImGui::Separator();
    bool rdy_active = !sim_2a03_get_rdy();
    if (ImGui::Checkbox("RDY", &rdy_active)) {
        sim_2a03_set_rdy(!rdy_active);
    }
    ImGui::SameLine();
    bool irq_active = !sim_2a03_get_irq();
    if (ImGui::Checkbox("IRQ", &irq_active)) {
        sim_2a03_set_irq(!irq_active);
    }
    ImGui::SameLine();
    bool nmi_active = !sim_2a03_get_nmi();
    if (ImGui::Checkbox("NMI", &nmi_active)) {
        sim_2a03_set_nmi(!nmi_active);
    }
    ImGui::SameLine();
    bool res_active = !sim_2a03_get_res();
    if (ImGui::Checkbox("RES", &res_active)) {
        sim_2a03_set_res(!res_active);
    }
}

static void ui_audio_status_panel(void) {
    uint16_t frm_t = sim_2a03_get_frm_t();
    ImGui::Text("frm: t:|%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c| (%04x)",
        frm_t & 0x4000 ? '1' : ' ', frm_t & 0x2000 ? '1' : ' ',
        frm_t & 0x1000 ? '1' : ' ', frm_t & 0x0800 ? '1' : ' ',
        frm_t & 0x0400 ? '1' : ' ', frm_t & 0x0200 ? '1' : ' ',
        frm_t & 0x0100 ? '1' : ' ', frm_t & 0x0080 ? '1' : ' ',
        frm_t & 0x0040 ? '1' : ' ', frm_t & 0x0020 ? '1' : ' ',
        frm_t & 0x0010 ? '1' : ' ', frm_t & 0x0008 ? '1' : ' ',
        frm_t & 0x0004 ? '1' : ' ', frm_t & 0x0002 ? '1' : ' ',
        frm_t & 0x0001 ? '1' : ' ', frm_t);

    ImGui::Text("Sq0:%u Sq1:%u Tri:%u Noi:%u Pcm:%u", sim_2a03_get_sq0_out(),
            sim_2a03_get_sq1_out(), sim_2a03_get_tri_out(),
          sim_2a03_get_noi_out(), sim_2a03_get_pcm_out());

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Square0")) {
            ImGui::Text("$4000:\n  duty:%u halt:%u c:%u env:%2u\n  timer:%2u "
                        "cycle:%u vol:%u",
                        sim_2a03_get_sq0_duty(), sim_2a03_get_sq0_lenhalt(),
                        sim_2a03_get_sq0_envmode(), sim_2a03_get_sq0_envp(),
                        sim_2a03_get_sq0_envt(), sim_2a03_get_sq0_c(),
                        sim_2a03_get_sq0_envc());
            ImGui::Text("$4001:\n  en:%u period:%2u neg:%u shift:%2u\n  timer:%2u",
                        sim_2a03_get_sq0_swpen(), sim_2a03_get_sq0_swpp(),
                        sim_2a03_get_sq0_swpdir(), sim_2a03_get_sq0_swpb(),
                        sim_2a03_get_sq0_swpt());
            ImGui::Text("$4002:\n  period:%u\n  timer:%u", sim_2a03_get_sq0_p(),
                        sim_2a03_get_sq0_t());
            ImGui::Text("$4003:\n  len:%3u\n  en:%u on:%u len_reload:%u silence:%u",
                        sim_2a03_get_sq0_len(), sim_2a03_get_sq0_en(),
                        sim_2a03_get_sq0_on(), sim_2a03_get_sq0_len_reload(),
                        sim_2a03_get_sq0_silence());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Square1")) {
            ImGui::Text("$4004:\n  duty:%u halt:%u c:%u env:%2u\n  timer:%2u "
                        "cycle:%u vol:%u",
                        sim_2a03_get_sq1_duty(), sim_2a03_get_sq1_lenhalt(),
                        sim_2a03_get_sq1_envmode(), sim_2a03_get_sq1_envp(),
                        sim_2a03_get_sq1_envt(), sim_2a03_get_sq1_c(),
                        sim_2a03_get_sq1_envc());
            ImGui::Text("$4005:\n  en:%u period:%2u neg:%u shift:%2u\n  timer:%2u",
                        sim_2a03_get_sq1_swpen(), sim_2a03_get_sq1_swpp(),
                        sim_2a03_get_sq1_swpdir(), sim_2a03_get_sq1_swpb(),
                        sim_2a03_get_sq1_swpt());
            ImGui::Text("$4006:\n  period:%u\n  timer:%u", sim_2a03_get_sq1_p(),
                        sim_2a03_get_sq1_t());
            ImGui::Text("$4007:\n  len:%3u\n  en:%u on:%u len_reload:%u silence:%u",
                        sim_2a03_get_sq1_len(), sim_2a03_get_sq1_en(),
                        sim_2a03_get_sq1_on(), sim_2a03_get_sq1_len_reload(),
                        sim_2a03_get_sq1_silence());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Triangle")) {
            ImGui::Text("$4008:\n  en:%u linear:%u\n  lin.counter:%u cycle:%u",
                        sim_2a03_get_tri_lin_en(), sim_2a03_get_tri_lin(),
                        sim_2a03_get_tri_lc(), sim_2a03_get_tri_c());
            ImGui::Text("$400a:\n  period:%u\n  timer:%u", sim_2a03_get_tri_p(),
                        sim_2a03_get_tri_t());
            ImGui::Text("$400b:\n  len:%3u\n  en:%u on:%u len_reload:%u silence:%u",
                        sim_2a03_get_tri_len(), sim_2a03_get_tri_en(),
                        sim_2a03_get_tri_on(), sim_2a03_get_tri_len_reload(),
                        sim_2a03_get_tri_silence());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Noise")) {
            uint16_t noi_c = sim_2a03_get_noi_c();
            ImGui::Text("$400c:\n  halt:%u c:%u env:%2u\n  timer:%2u vol:%u\n  "
                        "cycle:|%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c| (%04x)",
                        sim_2a03_get_noi_lenhalt(), sim_2a03_get_noi_envmode(),
                        sim_2a03_get_noi_envp(), sim_2a03_get_noi_envt(),
                        sim_2a03_get_noi_envc(), noi_c & 0x4000 ? '1' : ' ',
                        noi_c & 0x2000 ? '1' : ' ', noi_c & 0x1000 ? '1' : ' ',
                        noi_c & 0x0800 ? '1' : ' ', noi_c & 0x0400 ? '1' : ' ',
                        noi_c & 0x0200 ? '1' : ' ', noi_c & 0x0100 ? '1' : ' ',
                        noi_c & 0x0080 ? '1' : ' ', noi_c & 0x0040 ? '1' : ' ',
                        noi_c & 0x0020 ? '1' : ' ', noi_c & 0x0010 ? '1' : ' ',
                        noi_c & 0x0008 ? '1' : ' ', noi_c & 0x0004 ? '1' : ' ',
                        noi_c & 0x0002 ? '1' : ' ', noi_c & 0x0001 ? '1' : ' ',
                        noi_c);
            uint16_t noi_t = sim_2a03_get_noi_t();
            ImGui::Text("$400e:\n  mode:%u period:%u\n  "
                        "timer:|%c%c%c%c%c%c%c%c%c%c%c| (%04x)",
                        sim_2a03_get_noi_lfsrmode(), sim_2a03_get_noi_freq(),
                        noi_t & 0x0400 ? '1' : ' ', noi_t & 0x0200 ? '1' : ' ',
                        noi_t & 0x0100 ? '1' : ' ', noi_t & 0x0080 ? '1' : ' ',
                        noi_t & 0x0040 ? '1' : ' ', noi_t & 0x0020 ? '1' : ' ',
                        noi_t & 0x0010 ? '1' : ' ', noi_t & 0x0008 ? '1' : ' ',
                        noi_t & 0x0004 ? '1' : ' ', noi_t & 0x0002 ? '1' : ' ',
                        noi_t & 0x0001 ? '1' : ' ', noi_t);
            ImGui::Text("$400f:\n  len:%3u\n  en:%u on:%u len_reload:%u silence:%u",
                        sim_2a03_get_noi_len(), sim_2a03_get_noi_en(),
                        sim_2a03_get_noi_on(), sim_2a03_get_noi_len_reload(),
                        sim_2a03_get_noi_silence());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("DMC")) {
            uint16_t pcm_t = sim_2a03_get_pcm_t();
            ImGui::Text("$4010:\n  irqen:%u loop:%u freq:%2u\n  "
                        "timer:|%c%c%c%c%c%c%c%c%c| (%03x)",
                        sim_2a03_get_pcm_irqen(), sim_2a03_get_pcm_loop(),
                        sim_2a03_get_pcm_freq(), pcm_t & 0x0100 ? '1' : ' ',
                        pcm_t & 0x0080 ? '1' : ' ', pcm_t & 0x0040 ? '1' : ' ',
                        pcm_t & 0x0020 ? '1' : ' ', pcm_t & 0x0010 ? '1' : ' ',
                        pcm_t & 0x0008 ? '1' : ' ', pcm_t & 0x0004 ? '1' : ' ',
                        pcm_t & 0x0002 ? '1' : ' ', pcm_t & 0x0001 ? '1' : ' ', pcm_t);
            ImGui::Text("$4012:\n  saddr:%02x addr:%04x\n  buf:%02x load:%u\n  "
                        "sr:%02x load:%u shift:%u",
                        sim_2a03_get_pcm_sa(), sim_2a03_get_pcm_a(),
                        sim_2a03_get_pcm_buf(), sim_2a03_get_pcm_loadbuf(),
                        sim_2a03_get_pcm_sr(), sim_2a03_get_pcm_loadsr(),
                        sim_2a03_get_pcm_shiftsr());
            ImGui::Text("$4013:\n  len:%3u\n  counter:%u\n  en:%u rd_active:%u",
                        sim_2a03_get_pcm_l(), sim_2a03_get_pcm_lc(),
                        sim_2a03_get_pcm_en(), sim_2a03_get_pcm_rd_active());
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
#endif

static bool ui_cassette_deck_button(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 10.0f } );
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 7.0f, 7.0f } );
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFFFFFFFF);
    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF000000);
    const bool res = ImGui::Button(label, { 28, 25 });
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    return res;
}

static void ui_cassette_deck_controls(void) {
    if (ui_cassette_deck_button(ICON_FA_STEP_BACKWARD)) {
        sim_set_paused(true);
        trace_revert_to_previous();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Step back one half-cycle");
    }
    ImGui::SameLine();
    if (sim_get_paused()) {
        if (ui_cassette_deck_button(ICON_FA_PLAY)) {
            sim_set_paused(false);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Run (one half-cycle per frame)");
        }
    } else {
        if (ui_cassette_deck_button(ICON_FA_PAUSE)) {
            sim_set_paused(true);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Pause");
        }
    }
    ImGui::SameLine();
    if (ui_cassette_deck_button(ICON_FA_STEP_FORWARD)) {
        sim_set_paused(true);
        sim_step(1);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Step one half-cycle");
    }
    ImGui::SameLine();
    if (ui_cassette_deck_button(ICON_FA_FAST_FORWARD)) {
        sim_set_paused(true);
        sim_step(2);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Step two half-cycles");
    }
    ImGui::SameLine();
    if (ui_cassette_deck_button(ICON_FA_ARROW_RIGHT)) {
        sim_set_paused(true);
        sim_step_op();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Step to next instruction");
    }
    ImGui::SameLine();
    if (ui_cassette_deck_button(ICON_FA_EJECT)) {
        trace_clear();
        sim_shutdown();
        sim_init();
        sim_start();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset");
    }
}

static void ui_cpu_controls(void) {
    if (!ui_pre_check(UI_WINDOW_CPU_CONTROLS)) {
        return;
    }
    ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x - 300, 50 }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ 270, 480 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(ui_window_id(UI_WINDOW_CPU_CONTROLS), ui_window_open_ptr(UI_WINDOW_CPU_CONTROLS), ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_AlwaysAutoResize)) {
        ui_cassette_deck_controls();
        ui_cpu_status_panel();
    }
    ImGui::End();
}

#if defined(CHIP_2A03)
static void ui_audio_controls(void) {
    if (!ui_pre_check(UI_WINDOW_AUDIO)) {
        return;
    }
    if (ImGui::Begin(ui_window_id(UI_WINDOW_AUDIO), ui_window_open_ptr(UI_WINDOW_AUDIO))) {
        ui_audio_status_panel();
    }
    ImGui::End();
}
#endif

static void ui_listing(void) {
    if (!ui_pre_check(UI_WINDOW_LISTING)) {
        return;
    }
    ImGui::SetNextWindowPos({60, 320}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({480, 200}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(ui_window_id(UI_WINDOW_LISTING), ui_window_open_ptr(UI_WINDOW_LISTING))) {
        ImGui::Text("%s", asm_listing());
    }
    ImGui::End();
}

static void ui_memedit(void) {
    ui_check_dirty(UI_WINDOW_MEMEDIT);
    ui.memedit.open = ui_is_window_open(UI_WINDOW_MEMEDIT);
    ui_memedit_draw(&ui.memedit);
    ui_set_window_open(UI_WINDOW_MEMEDIT, ui.memedit.open);
}

#if defined(CHIP_Z80)
static void ui_ioedit(void) {
    ui_check_dirty(UI_WINDOW_IOEDIT);
    ui.ioedit.open = ui_is_window_open(UI_WINDOW_IOEDIT);
    ui_memedit_draw(&ui.ioedit);
    ui_set_window_open(UI_WINDOW_IOEDIT, ui.ioedit.open);
}
#endif

static void ui_dasm(void) {
    ui_check_dirty(UI_WINDOW_DASM);
    ui.dasm.open = ui_is_window_open(UI_WINDOW_DASM);
    ui_dasm_draw(&ui.dasm);
    ui_set_window_open(UI_WINDOW_DASM, ui.dasm.open);
}

static void ui_tracelog_timingdiagram_begin() {
    if (!trace_empty()) {
        const uint32_t min_cycle = trace_get_cycle(trace_num_items()-1);
        const uint32_t max_cycle = trace_get_cycle(0);
        if (ui.trace.is_selected) {
            if ((ui.trace.selected_cycle < min_cycle) || (ui.trace.selected_cycle > max_cycle)) {
                ui.trace.is_selected = false;
                ui.trace.is_diff_selected = false;
            }
        }
        if (ui.trace.is_diff_selected) {
            if ((ui.trace.diff_selected_cycle < min_cycle) || (ui.trace.diff_selected_cycle > max_cycle)) {
                ui.trace.is_diff_selected = false;
            }
        }
    }
}

static void ui_tracelog_timingdiagram_end() {
    if (trace_ui_get_scroll_to_end()) {
        trace_ui_set_scroll_to_end(false);
    }
}

#if defined(CHIP_6502)
static int ui_tracelog_print_item(int trace_index, int col_index, char* buf, size_t buf_size) {
    const uint32_t cur_cycle = trace_get_cycle(trace_index);
    char p_buf[9];
    switch (col_index) {
        case 0: return snprintf(buf, buf_size, "%5d/%d", cur_cycle >> 1, cur_cycle & 1);
        case 1: return snprintf(buf, buf_size, "%s", trace_6502_get_sync(trace_index)?"SYNC":"    ");
        case 2: return snprintf(buf, buf_size, "%c", trace_6502_get_rw(trace_index)?'R':'W');
        case 3: return snprintf(buf, buf_size, "%04X", trace_get_addr(trace_index));
        case 4: return snprintf(buf, buf_size, "%02X", trace_get_data(trace_index));
        case 5: return snprintf(buf, buf_size, "%04X", trace_get_pc(trace_index));
        case 6: return snprintf(buf, buf_size, "%02X", trace_6502_get_op(trace_index));
        case 7: return snprintf(buf, buf_size, "%02X", trace_6502_get_a(trace_index));
        case 8: return snprintf(buf, buf_size, "%02X", trace_6502_get_x(trace_index));
        case 9: return snprintf(buf, buf_size, "%02X", trace_6502_get_y(trace_index));
        case 10: return snprintf(buf, buf_size, "%02X", trace_6502_get_sp(trace_index));
        case 11: return snprintf(buf, buf_size, "%02X", trace_get_flags(trace_index));
        case 12:
            if (ui.trace.watch_node_valid) {
                return snprintf(buf, buf_size, "%c", trace_is_node_high(trace_index, ui.trace.watch_node_index) ? '1':'0');
            }
            else {
                return snprintf(buf, buf_size, "%s", "??");
            }
        case 13: return snprintf(buf, buf_size, "%s", ui_cpu_flags_as_string(trace_get_flags(trace_index), p_buf, sizeof(p_buf)));
        case 14: return snprintf(buf, buf_size, "%s", trace_6502_get_irq(trace_index)?"   ":"IRQ");
        case 15: return snprintf(buf, buf_size, "%s", trace_6502_get_nmi(trace_index)?"   ":"NMI");
        case 16: return snprintf(buf, buf_size, "%s", trace_6502_get_res(trace_index)?"   ":"RES");
        case 17: return snprintf(buf, buf_size, "%s", trace_6502_get_rdy(trace_index)?"   ":"RDY");
        case 18: return snprintf(buf, buf_size, "%s", trace_get_disasm(trace_index));
        default: return snprintf(buf, buf_size, "%s", "???");
    }
}
#elif defined(CHIP_Z80)
static int ui_tracelog_print_item(int trace_index, int col_index, char* buf, size_t buf_size) {
    const uint32_t cur_cycle = trace_get_cycle(trace_index);
    char f_buf[9];
    switch (col_index) {
        case 0: return snprintf(buf, buf_size, "%5d/%d", cur_cycle >> 1, cur_cycle & 1);
        case 1: return snprintf(buf, buf_size, "%d", trace_z80_get_mcycle(trace_index));
        case 2: return snprintf(buf, buf_size, "%d", trace_z80_get_tstate(trace_index));
        case 3: return snprintf(buf, buf_size, "%s", trace_z80_get_m1(trace_index)?"":"M1");
        case 4: return snprintf(buf, buf_size, "%s", trace_z80_get_mreq(trace_index)?"":"MREQ");
        case 5: return snprintf(buf, buf_size, "%s", trace_z80_get_ioreq(trace_index)?"":"IORQ");
        case 6: return snprintf(buf, buf_size, "%s", trace_z80_get_rfsh(trace_index)?"":"RFSH");
        case 7: return snprintf(buf, buf_size, "%s", trace_z80_get_rd(trace_index)?"":"RD");
        case 8: return snprintf(buf, buf_size, "%s", trace_z80_get_wr(trace_index)?"":"WR");
        case 9: return snprintf(buf, buf_size, "%s", trace_z80_get_int(trace_index)?"":"INT");
        case 10: return snprintf(buf, buf_size, "%s", trace_z80_get_nmi(trace_index)?"":"NMI");
        case 11: return snprintf(buf, buf_size, "%s", trace_z80_get_wait(trace_index)?"":"WAIT");
        case 12: return snprintf(buf, buf_size, "%s", trace_z80_get_halt(trace_index)?"":"HALT");
        case 13: return snprintf(buf, buf_size, "%04X", trace_get_addr(trace_index));
        case 14: return snprintf(buf, buf_size, "%02X", trace_get_data(trace_index));
        case 15: return snprintf(buf, buf_size, "%04X", trace_get_pc(trace_index));
        case 16: return snprintf(buf, buf_size, "%02X", trace_z80_get_op(trace_index));
        case 17: return snprintf(buf, buf_size, "%04X", trace_z80_get_af(trace_index));
        case 18: return snprintf(buf, buf_size, "%04X", trace_z80_get_bc(trace_index));
        case 19: return snprintf(buf, buf_size, "%04X", trace_z80_get_de(trace_index));
        case 20: return snprintf(buf, buf_size, "%04X", trace_z80_get_hl(trace_index));
        case 21: return snprintf(buf, buf_size, "%04X", trace_z80_get_af2(trace_index));
        case 22: return snprintf(buf, buf_size, "%04X", trace_z80_get_bc2(trace_index));
        case 23: return snprintf(buf, buf_size, "%04X", trace_z80_get_de2(trace_index));
        case 24: return snprintf(buf, buf_size, "%04X", trace_z80_get_hl2(trace_index));
        case 25: return snprintf(buf, buf_size, "%04X", trace_z80_get_ix(trace_index));
        case 26: return snprintf(buf, buf_size, "%04X", trace_z80_get_iy(trace_index));
        case 27: return snprintf(buf, buf_size, "%04X", trace_z80_get_sp(trace_index));
        case 28: return snprintf(buf, buf_size, "%04X", trace_z80_get_wz(trace_index));
        case 29: return snprintf(buf, buf_size, "%02X", trace_z80_get_i(trace_index));
        case 30: return snprintf(buf, buf_size, "%02X", trace_z80_get_r(trace_index));
        case 31: return snprintf(buf, buf_size, "%01X", trace_z80_get_im(trace_index));
        case 32: return snprintf(buf, buf_size, "%s", trace_z80_get_iff1(trace_index) ? "IFF1":"    ");
        case 33:
            if (ui.trace.watch_node_valid) {
                return snprintf(buf, buf_size, "%c", trace_is_node_high(trace_index, ui.trace.watch_node_index) ? '1':'0');
            }
            else {
                return snprintf(buf, buf_size, "%s", "??");
            }
        case 34: return snprintf(buf, buf_size, "%s", ui_cpu_flags_as_string(trace_get_flags(trace_index), f_buf, sizeof(f_buf)));
        case 35: return snprintf(buf, buf_size, "%s", trace_get_disasm(trace_index));
        default: return snprintf(buf, buf_size, "%s", "???");
    }
}
#elif defined(CHIP_2A03)
static int ui_tracelog_print_item(int trace_index, int col_index, char* buf, size_t buf_size) {
    const uint32_t cur_cycle = trace_get_cycle(trace_index);
    char p_buf[9];
    switch (col_index) {
        case 0: return snprintf(buf, buf_size, "%5d/%d", cur_cycle >> 1, cur_cycle & 1);
        case 1: return snprintf(buf, buf_size, "%s", trace_2a03_get_sync(trace_index)?"SYNC":"    ");
        case 2: return snprintf(buf, buf_size, "%c", trace_2a03_get_rw(trace_index)?'R':'W');
        case 3: return snprintf(buf, buf_size, "%04X", trace_get_addr(trace_index));
        case 4: return snprintf(buf, buf_size, "%02X", trace_get_data(trace_index));
        case 5: return snprintf(buf, buf_size, "%04X", trace_get_pc(trace_index));
        case 6: return snprintf(buf, buf_size, "%02X", trace_2a03_get_op(trace_index));
        case 7: return snprintf(buf, buf_size, "%02X", trace_2a03_get_a(trace_index));
        case 8: return snprintf(buf, buf_size, "%02X", trace_2a03_get_x(trace_index));
        case 9: return snprintf(buf, buf_size, "%02X", trace_2a03_get_y(trace_index));
        case 10: return snprintf(buf, buf_size, "%02X", trace_2a03_get_sp(trace_index));
        case 11: return snprintf(buf, buf_size, "%02X", trace_get_flags(trace_index));
        case 12:
            if (ui.trace.watch_node_valid) {
                return snprintf(buf, buf_size, "%c", trace_is_node_high(trace_index, ui.trace.watch_node_index) ? '1':'0');
            }
            else {
                return snprintf(buf, buf_size, "%s", "??");
            }
        case 13: return snprintf(buf, buf_size, "%s", ui_cpu_flags_as_string(trace_get_flags(trace_index), p_buf, sizeof(p_buf)));
        case 14: return snprintf(buf, buf_size, "%s", trace_2a03_get_irq(trace_index)?"   ":"IRQ");
        case 15: return snprintf(buf, buf_size, "%s", trace_2a03_get_nmi(trace_index)?"   ":"NMI");
        case 16: return snprintf(buf, buf_size, "%s", trace_2a03_get_res(trace_index)?"   ":"RES");
        case 17: return snprintf(buf, buf_size, "%s", trace_2a03_get_rdy(trace_index)?"   ":"RDY");
        case 18: return snprintf(buf, buf_size, "%s", trace_get_disasm(trace_index));
        case 19: return snprintf(buf, buf_size, "%u", trace_2a03_get_sq0_out(trace_index));
        case 20: return snprintf(buf, buf_size, "%u", trace_2a03_get_sq1_out(trace_index));
        case 21: return snprintf(buf, buf_size, "%u", trace_2a03_get_tri_out(trace_index));
        case 22: return snprintf(buf, buf_size, "%u", trace_2a03_get_noi_out(trace_index));
        case 23: return snprintf(buf, buf_size, "%u", trace_2a03_get_pcm_out(trace_index));

        default: return snprintf(buf, buf_size, "%s", "???");
    }
}
#endif

static void ui_tracelog_table_setup_columns(void) {
    const float char_width = 8.0f;
    for (int i = 0; i < UI_TRACELOG_NUM_COLUMNS; i++) {
        const ui_tracelog_column_t* col = &ui_tracelog_columns[i];
        ImGui::TableSetupColumn(col->label, col->flags, col->width * char_width);
    }
}

static uint32_t ui_trace_bgcolor(uint32_t flipbits) {
    return ui_trace_bg_colors[flipbits & 3];
}

static void ui_update_watch_node(void) {
    ui.trace.watch_node_valid = false;
    int node_index = sim_find_node(ui.trace.watch_str);
    if (node_index >= 0) {
        ui.trace.watch_node_valid = true;
        ui.trace.watch_node_index = (uint32_t)node_index;
    }
}

static void ui_tracelog(void) {
    if (!ui_pre_check(UI_WINDOW_TRACELOG)) {
        // diff-view is deactivated when tracelog window isn't open
        ui.trace.diff_view_active = false;
        return;
    }
    const float footer_h = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    const float disp_w = ImGui::GetIO().DisplaySize.x;
    const float disp_h = ImGui::GetIO().DisplaySize.y;
    ImGui::SetNextWindowPos({ disp_w / 2, disp_h - 150 }, ImGuiCond_FirstUseEver, { 0.5f, 0.0f });
    ImGui::SetNextWindowSize({ 600, 128 }, ImGuiCond_FirstUseEver);
    bool any_hovered = false;
    if (ImGui::Begin(ui_window_id(UI_WINDOW_TRACELOG), ui_window_open_ptr(UI_WINDOW_TRACELOG))) {
        const ImGuiTableFlags table_flags =
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_Hideable |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV |
            ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("##trace_data", UI_TRACELOG_NUM_COLUMNS, table_flags, {0.0f, -footer_h})) {
            ImGui::TableSetupScrollFreeze(1, 1); // top and left row always visible
            ui_tracelog_table_setup_columns();
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(trace_num_items());
            while (clipper.Step()) {
                for (int row_index = clipper.DisplayStart; row_index < clipper.DisplayEnd; row_index++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    const int trace_index = trace_num_items() - 1 - row_index;
                    assert(trace_index >= 0);
                    const uint32_t cur_cycle = trace_get_cycle(trace_index);
                    uint32_t bg_color = ui_trace_bgcolor(trace_get_flipbits(trace_index));
                    if ((0 != ui.trace.hovered_flags) && (ui.trace.hovered_cycle == cur_cycle)) {
                        if (ui.trace.diff_view_active && ui.trace.is_selected && !ui.trace.is_diff_selected) {
                            bg_color = ui_trace_diff_selected_color;
                        }
                        else {
                            bg_color = ui_trace_hovered_color;
                        }
                    }
                    if (ui.trace.is_selected && (ui.trace.selected_cycle == cur_cycle)) {
                        bg_color = ui_trace_selected_color;
                    }
                    if (ui.trace.diff_view_active && ui.trace.is_diff_selected && (ui.trace.diff_selected_cycle == cur_cycle)) {
                        bg_color = ui_trace_diff_selected_color;
                    }
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg_color);
                    const ImVec2 min_p = ImGui::GetCursorScreenPos();
                    for (int col_index = 0; col_index < UI_TRACELOG_NUM_COLUMNS; col_index++) {
                        ImGui::TableSetColumnIndex(col_index);
                        char buf[32];
                        ui_tracelog_print_item(trace_index, col_index, buf, sizeof(buf));
                        ImGui::Text("%s", buf);
                    }
                    ImVec2 max_p = ImGui::GetCursorScreenPos();
                    max_p.x = min_p.x + ImGui::GetWindowContentRegionMax().x;
                    const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(min_p, max_p, false);
                    if (hovered) {
                        any_hovered = true;
                        ui.trace.hovered_flags |= TRACELOG_HOVERED;
                        ui.trace.hovered_cycle = cur_cycle;
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            if (ui.trace.diff_view_active && ui.trace.is_selected && !ui.trace.is_diff_selected) {
                                ui.trace.is_diff_selected = true;
                                ui.trace.diff_selected_cycle = cur_cycle;
                            }
                            else {
                                ui.trace.is_diff_selected = false;
                                ui.trace.is_selected = true;
                                ui.trace.selected_cycle = cur_cycle;
                            }
                        }
                    }
                }
            }
            if (trace_ui_get_scroll_to_end()) {
                ImGui::SetScrollHereY();
            }
            // record the current column visibility status
            for (int i = 0; i < UI_TRACELOG_NUM_COLUMNS; i++) {
                ui.trace.column_visible[i] = 0 != (ImGui::TableGetColumnFlags(i) & ImGuiTableColumnFlags_IsVisible);
            }
            ImGui::EndTable();
            ImGui::Separator();

            // Watch search field
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Watch:");
            ImGui::SameLine();
            ImGui::PushItemWidth(64.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ui.trace.watch_node_valid ? ui_found_text_color : ui_notfound_text_color);
            if (ImGui::InputText("##watch", ui.trace.watch_str, sizeof(ui.trace.watch_str))) {
                ui_update_watch_node();
            }
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();
            ImGui::EndGroup();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enter node name or number.\nSee status in Watch column.");
            }
            if (ui.trace.is_selected) {
                ImGui::SameLine();
                if (ImGui::Button("Revert to Selected")) {
                    trace_revert_to_cycle(ui.trace.selected_cycle);
                }
                ImGui::SameLine();
                ImGui::Checkbox("Diff View", &ui.trace.diff_view_active);
                if (ui.trace.diff_view_active && !ui.trace.is_diff_selected) {
                    ImGui::SameLine();
                    ImGui::Text("(hint: select another cycle to view diff)");
                }
            }
        }
    }
    ImGui::End();
    if (!any_hovered) {
        ui.trace.hovered_flags &= ~TRACELOG_HOVERED;
    }
}

static void ui_tracedump_append(const char* str) {
    char c;
    while ((ui.tracedump.pos < (MAX_TRACEDUMP_SIZE-1)) && ((c = *str++) != 0)) {
        ui.tracedump.buf[ui.tracedump.pos++] = c;
    }
}

static void ui_tracedump_begin(void) {
    // see: https://unicode.org/charts/nameslist/n_2500.html
    char buf[64];
    ui.tracedump.pos = 0;
    bool first = true;
    for (int col_index = 0; col_index < UI_TRACELOG_NUM_COLUMNS; col_index++) {
        if (ui.trace.column_visible[col_index]) {
            if (first) {
                first = false;
                ui_tracedump_append("\u250C\u2500");
            }
            else {
                ui_tracedump_append("\u252C\u2500");
            }
            for (int i = 0; i < ui_tracelog_columns[col_index].width; i++) {
                ui_tracedump_append("\u2500");
            }
            ui_tracedump_append("\u2500");
        }
    }
    ui_tracedump_append("\u2510\n");
    for (int col_index = 0; col_index < UI_TRACELOG_NUM_COLUMNS; col_index++) {
        if (ui.trace.column_visible[col_index]) {
            const ui_tracelog_column_t* col = &ui_tracelog_columns[col_index];
            ui_tracedump_append("\u2502 ");
            int pos = snprintf(buf, sizeof(buf), "%s", col->label);
            for (; pos < col->width; pos++) {
                buf[pos] = ' ';
            }
            buf[pos] = 0;
            ui_tracedump_append(buf);
            ui_tracedump_append(" ");
        }
    }
    ui_tracedump_append("\u2502\n");
    first = true;
    for (int col_index = 0; col_index < UI_TRACELOG_NUM_COLUMNS; col_index++) {
        if (ui.trace.column_visible[col_index]) {
            if (first) {
                first = false;
                ui_tracedump_append("\u251C\u2500");
            }
            else {
                ui_tracedump_append("\u253C\u2500");
            }
            for (int i = 0; i < ui_tracelog_columns[col_index].width; i++) {
                ui_tracedump_append("\u2500");
            }
            ui_tracedump_append("\u2500");
        }
    }
    ui_tracedump_append("\u2524\n");
}

static void ui_tracedump_row(int trace_index) {
    char buf[64];
    for (int col_index = 0; col_index < UI_TRACELOG_NUM_COLUMNS; col_index++) {
        if (ui.trace.column_visible[col_index]) {
            ui_tracedump_append("\u2502 ");
            int pos = ui_tracelog_print_item(trace_index, col_index, buf, sizeof(buf));
            for (; pos < (ui_tracelog_columns[col_index].width); pos++) {
                buf[pos] = ' ';
            }
            buf[pos] = 0;
            ui_tracedump_append(buf);
            ui_tracedump_append(" ");
        }
    }
    ui_tracedump_append("\u2502\n");
}

static void ui_tracedump_end(void) {
    assert(ui.tracedump.pos < MAX_TRACEDUMP_SIZE);
    ui.tracedump.buf[ui.tracedump.pos] = 0;
}

const char* ui_trace_get_dump(void) {
    const int num_rows = trace_num_items();
    ui_tracedump_begin();
    for (int row_index = 0; row_index < num_rows; row_index++) {
        const int trace_index = num_rows - 1 - row_index;
        ui_tracedump_row(trace_index);
    }
    ui_tracedump_end();
    return ui.tracedump.buf;
}

#if defined(CHIP_6502)
static const int num_time_diagram_nodes = 7;
static struct { uint32_t node; const char* name; bool active_low; } time_diagram_nodes[num_time_diagram_nodes] = {
    { p6502_clk0, "CLK0", false },
    { p6502_sync, "SYNC", false },
    { p6502_rw, "RW", false },
    { p6502_irq, "IRQ", true },
    { p6502_nmi, "NMI", true },
    { p6502_res, "RES", true },
    { p6502_rdy, "RDY", false },
};
#elif defined(CHIP_Z80)
static const int num_time_diagram_nodes = 15;
static struct { uint32_t node; const char* name; bool active_low; } time_diagram_nodes[num_time_diagram_nodes] = {
    { pz80_clk, "CLK", false },
    { pz80__m1, "M1", true },
    { pz80__mreq, "MREQ", true },
    { pz80__iorq, "IORQ", true },
    { pz80__rd, "RD", true },
    { pz80__wr, "WR", true },
    { pz80__rfsh, "RFSH", true },
    { pz80__halt, "HALT", true },
    { pz80__wait, "WAIT", true },
    { pz80__int, "INT", true },
    { pz80__nmi, "NMI", true },
    { pz80__reset, "RESET", true },
    { pz80__busrq, "BUSRQ", true },
    { pz80__busak, "BUSAK", true },
    { 1278, "IFF1", false },
};
#elif defined(CHIP_2A03)
static const int num_time_diagram_nodes = 7;
static struct { uint32_t node; const char* name; bool active_low; } time_diagram_nodes[num_time_diagram_nodes] = {
    { p2a03_clk0, "CLK0", false },
    { p2a03_sync, "SYNC", false },
    { p2a03_rw, "RW", false },
    { p2a03_irq, "IRQ", true },
    { p2a03_nmi, "NMI", true },
    { p2a03_res, "RES", true },
    { p2a03_rdy, "RDY", false },
};
#endif

static void ui_timingdiagram(void) {
    if (!ui_pre_check(UI_WINDOW_TIMING_DIAGRAM)) {
        return;
    }
    const float top_padding = ImGui::GetTextLineHeightWithSpacing();
    const float cell_padding = 8.0f;
    const float cell_height = 2 * cell_padding + ImGui::GetTextLineHeight();
    const float cell_width  = cell_height * 0.75f;
    const int num_cols = trace_num_items();
    const float graph_height = num_time_diagram_nodes * cell_height + top_padding;
    const float graph_width  = num_cols * cell_width;
    ImGui::SetNextWindowPos({60, 60}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({600, top_padding + graph_height + 32.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowContentSize({graph_width, graph_height});
    bool any_hovered = false;
    if (ImGui::Begin(ui_window_id(UI_WINDOW_TIMING_DIAGRAM), ui_window_open_ptr(UI_WINDOW_TIMING_DIAGRAM), ImGuiWindowFlags_HorizontalScrollbar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 dl_orig = ImGui::GetCursorScreenPos();

        if (trace_ui_get_scroll_to_end()) {
            ImGui::SetScrollX(ImGui::GetScrollMaxX());
        }

        // draw vertical background stripes
        const int left_col = (int)(ImGui::GetScrollX() / cell_width); // don't clip instruction string
        const int right_col = (int)((ImGui::GetScrollX() + ImGui::GetWindowWidth() + cell_width) / cell_width);
        assert(left_col <= right_col);
        for (int col_index = left_col; col_index < right_col; col_index++) {
            if ((col_index < 0) || (col_index >= num_cols)) {
                continue;
            }
            const int trace_index = num_cols - 1 - col_index;
            const uint32_t cur_cycle = trace_get_cycle(trace_index);
            const float x0 = dl_orig.x + col_index * cell_width;
            const float x1 = x0 + cell_width;
            const float y0 = dl_orig.y + top_padding;
            const float y1 = y0 + graph_height - top_padding;
            uint32_t bg_color = ui_trace_bgcolor(trace_get_flipbits(trace_index));
            if ((0 != ui.trace.hovered_flags) && (ui.trace.hovered_cycle == cur_cycle)) {
                if (ui.trace.diff_view_active && ui.trace.is_selected && !ui.trace.is_diff_selected) {
                    bg_color = ui_trace_diff_selected_color;
                }
                else {
                    bg_color = ui_trace_hovered_color;
                }
            }
            if (ui.trace.is_selected && (ui.trace.selected_cycle == cur_cycle)) {
                bg_color = ui_trace_selected_color;
            }
            if (ui.trace.diff_view_active && ui.trace.is_diff_selected && (ui.trace.diff_selected_cycle == cur_cycle)) {
                bg_color = ui_trace_diff_selected_color;
            }
            dl->AddRectFilled({x0,y0}, {x1,y1}, bg_color);
            if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect({x0,y0}, {x1,y1}, false)) {
                any_hovered = true;
                ui.trace.hovered_flags |= TIMINGDIAGRAM_HOVERED;
                ui.trace.hovered_cycle = cur_cycle;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (ui.trace.diff_view_active && ui.trace.is_selected && !ui.trace.is_diff_selected) {
                        ui.trace.is_diff_selected = true;
                        ui.trace.diff_selected_cycle = cur_cycle;
                    }
                    else {
                        ui.trace.is_diff_selected = false;
                        ui.trace.is_selected = true;
                        ui.trace.selected_cycle = cur_cycle;
                    }
                }
            }
            if ((col_index == left_col) || ((trace_index < (trace_num_items() - 1)) && (0 != strcmp(trace_get_disasm(trace_index), trace_get_disasm(trace_index + 1))))) {
                float tx = x0;
                if (tx < (dl_orig.x + ImGui::GetScrollX())) {
                    tx = dl_orig.x + ImGui::GetScrollX();
                }
                float ty = y0 - top_padding;
                dl->AddRectFilled({ tx - 3.0f, ty }, { tx + 64.0f, ty + ImGui::GetTextLineHeight() }, ImColor(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]));
                dl->AddText({ tx, ty }, ui_diagram_text_color, trace_get_disasm(trace_index));
            }
        }
        for (int line = 0; line < num_time_diagram_nodes; line++) {
            float last_y = 0.0f;
            for (int col_index = left_col; col_index < right_col; col_index++) {
                if ((col_index < 0) || (col_index >= num_cols)) {
                    continue;
                }
                int trace_index = num_cols - 1 - col_index;
                float x = dl_orig.x + col_index * cell_width;
                float y = line * cell_height + cell_height * 0.5f + dl_orig.y + top_padding;
                bool high = trace_is_node_high(trace_index, time_diagram_nodes[line].node);
                if (high) {
                    y -= cell_height * 0.3f;
                }
                else {
                    y += cell_height * 0.3f;
                }
                dl->AddLine({x,y}, {x+cell_width,y}, ui_diagram_line_color);
                if (last_y != 0.0f) {
                    dl->AddLine({x,y}, {x,last_y}, ui_diagram_line_color);
                }
                last_y = y;
            }
            const char* node_name = time_diagram_nodes[line].name;
            const bool active_low = time_diagram_nodes[line].active_low;
            const float tx = dl_orig.x + ImGui::GetScrollX();
            const float ty = dl_orig.y + top_padding + line * cell_height + cell_padding;
            const ImVec2 ts = ImGui::CalcTextSize(node_name);
            dl->AddRectFilled({ tx - 3.0f, ty - 1.0f }, { tx + ts.x + 3.0f, ty + ts.y + 2.0f }, ui_diagram_text_bg_color);
            if (active_low) {
                dl->AddLine({ tx, ty + 1.0f }, { tx + ts.x, ty + 1.0f }, ui_diagram_text_color);
            }
            dl->AddText({ tx, ty }, ui_diagram_text_color, node_name);
        }
    }
    ImGui::End();
    if (!any_hovered) {
        ui.trace.hovered_flags &= ~TIMINGDIAGRAM_HOVERED;
    }
}

static void ui_nodeexplorer_clear_selected_nodes(void) {
    memset(ui.explorer.selected, gfx_visual_node_inactive, sizeof(ui.explorer.selected));
}

static void ui_nodeexplorer_update_selected_nodes_from_editor(void) {
    TextEditor::ErrorMarkers err_markers;
    ui_nodeexplorer_clear_selected_nodes();
    auto lines = ui.explorer.editor->GetTextLines();
    int line_nr = 0;
    for (const auto& line: lines) {
        line_nr++;
        strncpy(ui.explorer.token_buffer, line.c_str(), sizeof(ui.explorer.token_buffer));
        ui.explorer.token_buffer[sizeof(ui.explorer.token_buffer)-1] = 0;
        char* str = ui.explorer.token_buffer;
        const char* token_str;
        while (0 != (token_str = strtok(str, " ,\t\r\n"))) {
            str = 0;
            int node_index = sim_find_node(token_str);
            if (node_index >= 0) {
                assert(node_index < MAX_NODES);
                ui.explorer.selected[node_index] = gfx_visual_node_active;
            }
            else {
                char err_msg[128];
                snprintf(err_msg, sizeof(err_msg), "Unknown node: '%s'", token_str);
                // err_markers is std::map<int, std::string>
                err_markers[line_nr] = err_msg;
            }
        }
    }
    ui.explorer.editor->SetErrorMarkers(err_markers);
}

static void ui_nodeexplorer_add_node_by_name(const char* node_name) {
    ui.explorer.editor->MoveEnd();
    ui.explorer.editor->InsertText(std::string("\n") + node_name);
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer_add_nodegroup(sim_nodegroup_t group) {
    ui.explorer.editor->MoveEnd();
    ui.explorer.editor->InsertText("\n");
    for (int i = 0; i < group.num_nodes; i++) {
        const uint32_t node_index = group.nodes[i];
        const char* node_name = sim_get_node_name_by_index(node_index);
        if (node_name[0]) {
            ui.explorer.editor->InsertText(node_name + std::string(" "));
        }
    }
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer_init(void) {
    ui_nodeexplorer_clear_selected_nodes();
    ui.explorer.editor = new TextEditor();
    ui.explorer.editor->SetPalette(TextEditor::GetRetroBluePalette());
    ui.explorer.editor->SetShowWhitespaces(false);
    ui.explorer.editor->SetTabSize(8);
    ui.explorer.editor->SetImGuiChildIgnored(true);
    ui.explorer.editor->SetColorizerEnable(false);
}

static void ui_nodeexplorer_discard(void) {
    delete ui.explorer.editor;
    ui.explorer.editor = 0;
}

static void ui_nodeexplorer_undo(void) {
    ui.explorer.editor->Undo();
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer_redo(void) {
    ui.explorer.editor->Redo();
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer_cut(void) {
    ui.explorer.editor->Cut();
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer_copy(void) {
    ui.explorer.editor->Copy();
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer_paste(void) {
    ui.explorer.editor->Paste();
    ui_nodeexplorer_update_selected_nodes_from_editor();
}

static void ui_nodeexplorer(void) {
    ui.explorer.window_focused = false;
    if (!ui_pre_check(UI_WINDOW_NODE_EXPLORER)) {
        return;
    }
    ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - 450, 70}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({400, 300}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(ui_window_id(UI_WINDOW_NODE_EXPLORER), ui_window_open_ptr(UI_WINDOW_NODE_EXPLORER), ImGuiWindowFlags_NoScrollWithMouse)) {
        ui.explorer.num_hovered_nodes = 0;
        ImGui::Text("Select nodes, groups or type node-names and -numbers:");
        ImGui::Separator();

        // node listbox
        ImGui::BeginGroup();
        ImGui::Text("Nodes:");
        if (ImGui::BeginListBox("##nodes", {96.0f, -1.0f})) {
            const sim_named_node_range_t nodes = sim_get_sorted_nodes();
            ImGuiListClipper clipper;
            clipper.Begin(nodes.num, ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(nodes.ptr[i].node_name)) {
                        ui_nodeexplorer_add_node_by_name(nodes.ptr[i].node_name);
                    }
                    if (ImGui::IsItemHovered()) {
                        ui.explorer.num_hovered_nodes = 1;
                        ui.explorer.hovered_node_indices[0] = (uint32_t)nodes.ptr[i].node_index;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndGroup();

        // group listbox
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Groups:");
        if (ImGui::BeginListBox("##groups", {80.0f, -1.0f})) {
            const sim_nodegroup_range_t groups = sim_get_nodegroups();
            ImGuiListClipper clipper;
            clipper.Begin(groups.num, ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step()) {
                for (int group_index = clipper.DisplayStart; group_index < clipper.DisplayEnd; group_index++) {
                    ImGui::PushID(group_index);
                    if (ImGui::Selectable(groups.ptr[group_index].group_name)) {
                        ui_nodeexplorer_add_nodegroup(groups.ptr[group_index]);
                    }
                    if (ImGui::IsItemHovered()) {
                        const int num_nodes = groups.ptr[group_index].num_nodes;
                        assert(num_nodes < MAX_NODEEXPLORER_HOVERED_NODES);
                        ui.explorer.num_hovered_nodes = num_nodes;
                        for (int i = 0; i < num_nodes; i++) {
                            ui.explorer.hovered_node_indices[i] = groups.ptr[group_index].nodes[i];
                        }
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndGroup();

        // editor
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(ui.explorer.editor->GetPalette()[(int)TextEditor::PaletteIndex::Background]));
        ImGui::BeginChild("##editor", {0,0}, false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
        ui.explorer.editor->Render("Editor");
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ui.explorer.window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        ui.explorer.explorer_mode_active = ui.explorer.window_focused || !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        if (ui.explorer.window_focused) {
            ui_set_keyboard_target(UI_WINDOW_NODE_EXPLORER);
        }
    }
    ImGui::End();
    if (ui.explorer.editor->IsTextChanged()) {
        ui_nodeexplorer_update_selected_nodes_from_editor();
    }
}

bool ui_is_nodeexplorer_active(void) {
    return ui_is_window_open(UI_WINDOW_NODE_EXPLORER) && ui.explorer.explorer_mode_active;
}

void ui_write_nodeexplorer_visual_state(range_t to_buffer) {
    assert(to_buffer.size >= sizeof(ui.explorer.selected));
    memcpy(to_buffer.ptr, ui.explorer.selected, sizeof(ui.explorer.selected));
    for (int i = 0; i < ui.explorer.num_hovered_nodes; i++) {
        const uint32_t node_index = ui.explorer.hovered_node_indices[i];
        assert(node_index < to_buffer.size);
        uint8_t* byte_ptr = (uint8_t*)to_buffer.ptr;
        byte_ptr[node_index] = gfx_visual_node_highlighted;
    }
}

static void ui_picking(void) {
    const pick_result_t pick_result = pick_get_last_result();
    if (pick_result.num_hits > 0) {
        char str[256];
        str[0] = 0;
        int valid_node_index = 0;
        for (int i = 0; i < pick_result.num_hits; i++) {
            int node_index = pick_result.node_index[i];
            if ((node_index >= 0) && (node_index < num_node_names)) {
                if (!sim_is_ignore_picking_highlight_node(node_index)) {
                    char print_buf[64];
                    if (valid_node_index != 0) {
                        strcat(str, "\n");
                    }
                    valid_node_index++;
                    if (0 != sim_get_node_name_by_index(node_index)[0]) {
                        snprintf(print_buf, sizeof(print_buf), "%s (#%d)", sim_get_node_name_by_index(node_index), node_index);
                    } else {
                        snprintf(print_buf, sizeof(print_buf), "#%d", node_index);
                    }
                    if ((strlen(str) + strlen(print_buf) + 4) < sizeof(str)) {
                        strcat(str, print_buf);
                    }
                }
            }
        }
        if ((str[0] != 0) && !ImGui::GetIO().WantCaptureMouse) {
            ImGui::SetTooltip("%s", str);
        }
    }
}

static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data) {
    assert(data.link && (data.linkLength > 0));
    int i = 0;
    for (i = 0; (i < data.linkLength) && (i < MAX_LINKURL_SIZE); i++) {
        ui.link_url[i] = data.link[i];
    }
    if (i < MAX_LINKURL_SIZE) {
        ui.link_url[i] = 0;
        ui.link_hovered = true;
    }
    else {
        ui.link_url[0] = 0;
        ui.link_hovered = false;
    }
}

static ImVec2 display_center(void) {
    const ImVec2 s = ImGui::GetIO().DisplaySize;
    return { s.x * 0.5f, s.y * 0.5f };
}

static void ui_help_assembler(void) {
    if (!ui_pre_check(UI_WINDOW_HELP_ASM)) {
        return;
    }
    ImGui::SetNextWindowSize({640, 400}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(display_center(), ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
    if (ImGui::Begin(ui_window_id(UI_WINDOW_HELP_ASM), ui_window_open_ptr(UI_WINDOW_HELP_ASM))) {
        ImGui::Markdown(dump_help_assembler_md, sizeof(dump_help_assembler_md)-1, md_conf);
    }
    ImGui::End();
}

static void ui_help_opcodes(void) {
    if (!ui_pre_check(UI_WINDOW_HELP_OPCODES)) {
        return;
    }
    ImGui::SetNextWindowSize({640, 400}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(ui_window_id(UI_WINDOW_HELP_OPCODES), ui_window_open_ptr(UI_WINDOW_HELP_OPCODES))) {
        ImGui::Text("TODO!");
    }
    ImGui::End();
}

// Atari 8-bitter font extracted from ROM listing, starting at 0x20
static const uint8_t atari_font[96][8] = {
    { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },    // - space
    { 0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x00 },    // - !
    { 0x00,0x66,0x66,0x66,0x00,0x00,0x00,0x00 },    // - "
    { 0x00,0x66,0xFF,0x66,0x66,0xFF,0x66,0x00 },    // - #
    { 0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00 },    // - $
    { 0x00,0x66,0x6C,0x18,0x30,0x66,0x46,0x00 },    // - %
    { 0x1C,0x36,0x1C,0x38,0x6F,0x66,0x3B,0x00 },    // - &
    { 0x00,0x18,0x18,0x18,0x00,0x00,0x00,0x00 },    // - '
    { 0x00,0x0E,0x1C,0x18,0x18,0x1C,0x0E,0x00 },    // - (
    { 0x00,0x70,0x38,0x18,0x18,0x38,0x70,0x00 },    // - )
    { 0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00 },    // - asterisk
    { 0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00 },    // - plus
    { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30 },    // - comma
    { 0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00 },    // - minus
    { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00 },    // - period
    { 0x00,0x06,0x0C,0x18,0x30,0x60,0x40,0x00 },    // - /
    { 0x00,0x3C,0x66,0x6E,0x76,0x66,0x3C,0x00 },    // - 0
    { 0x00,0x18,0x38,0x18,0x18,0x18,0x7E,0x00 },    // - 1
    { 0x00,0x3C,0x66,0x0C,0x18,0x30,0x7E,0x00 },    // - 2
    { 0x00,0x7E,0x0C,0x18,0x0C,0x66,0x3C,0x00 },    // - 3
    { 0x00,0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x00 },    // - 4
    { 0x00,0x7E,0x60,0x7C,0x06,0x66,0x3C,0x00 },    // - 5
    { 0x00,0x3C,0x60,0x7C,0x66,0x66,0x3C,0x00 },    // - 6
    { 0x00,0x7E,0x06,0x0C,0x18,0x30,0x30,0x00 },    // - 7
    { 0x00,0x3C,0x66,0x3C,0x66,0x66,0x3C,0x00 },    // - 8
    { 0x00,0x3C,0x66,0x3E,0x06,0x0C,0x38,0x00 },    // - 9
    { 0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00 },    // - colon
    { 0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x30 },    // - semicolon
    { 0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00 },    // - <
    { 0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00 },    // - =
    { 0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00 },    // - >
    { 0x00,0x3C,0x66,0x0C,0x18,0x00,0x18,0x00 },    // - ?
    { 0x00,0x3C,0x66,0x6E,0x6E,0x60,0x3E,0x00 },    // - @
    { 0x00,0x18,0x3C,0x66,0x66,0x7E,0x66,0x00 },    // - A
    { 0x00,0x7C,0x66,0x7C,0x66,0x66,0x7C,0x00 },    // - B
    { 0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00 },    // - C
    { 0x00,0x78,0x6C,0x66,0x66,0x6C,0x78,0x00 },    // - D
    { 0x00,0x7E,0x60,0x7C,0x60,0x60,0x7E,0x00 },    // - E
    { 0x00,0x7E,0x60,0x7C,0x60,0x60,0x60,0x00 },    // - F
    { 0x00,0x3E,0x60,0x60,0x6E,0x66,0x3E,0x00 },    // - G
    { 0x00,0x66,0x66,0x7E,0x66,0x66,0x66,0x00 },    // - H
    { 0x00,0x7E,0x18,0x18,0x18,0x18,0x7E,0x00 },    // - I
    { 0x00,0x06,0x06,0x06,0x06,0x66,0x3C,0x00 },    // - J
    { 0x00,0x66,0x6C,0x78,0x78,0x6C,0x66,0x00 },    // - K
    { 0x00,0x60,0x60,0x60,0x60,0x60,0x7E,0x00 },    // - L
    { 0x00,0x63,0x77,0x7F,0x6B,0x63,0x63,0x00 },    // - M
    { 0x00,0x66,0x76,0x7E,0x7E,0x6E,0x66,0x00 },    // - N
    { 0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00 },    // - O
    { 0x00,0x7C,0x66,0x66,0x7C,0x60,0x60,0x00 },    // - P
    { 0x00,0x3C,0x66,0x66,0x66,0x6C,0x36,0x00 },    // - Q
    { 0x00,0x7C,0x66,0x66,0x7C,0x6C,0x66,0x00 },    // - R
    { 0x00,0x3C,0x60,0x3C,0x06,0x06,0x3C,0x00 },    // - S
    { 0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x00 },    // - T
    { 0x00,0x66,0x66,0x66,0x66,0x66,0x7E,0x00 },    // - U
    { 0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00 },    // - V
    { 0x00,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00 },    // - W
    { 0x00,0x66,0x66,0x3C,0x3C,0x66,0x66,0x00 },    // - X
    { 0x00,0x66,0x66,0x3C,0x18,0x18,0x18,0x00 },    // - Y
    { 0x00,0x7E,0x0C,0x18,0x30,0x60,0x7E,0x00 },    // - Z
    { 0x00,0x1E,0x18,0x18,0x18,0x18,0x1E,0x00 },    // - [
    { 0x00,0x40,0x60,0x30,0x18,0x0C,0x06,0x00 },    // - backslash
    { 0x00,0x78,0x18,0x18,0x18,0x18,0x78,0x00 },    // - ]
    { 0x00,0x08,0x1C,0x36,0x63,0x00,0x00,0x00 },    // - ^
    { 0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00 },    // - underline
    { 0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00 },    // - Spanish ! (should be `)
    { 0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00 },    // - a
    { 0x00,0x60,0x60,0x7C,0x66,0x66,0x7C,0x00 },    // - b
    { 0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00 },    // - c
    { 0x00,0x06,0x06,0x3E,0x66,0x66,0x3E,0x00 },    // - d
    { 0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00 },    // - e
    { 0x00,0x0E,0x18,0x3E,0x18,0x18,0x18,0x00 },    // - f
    { 0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C },    // - g
    { 0x00,0x60,0x60,0x7C,0x66,0x66,0x66,0x00 },    // - h
    { 0x00,0x18,0x00,0x38,0x18,0x18,0x3C,0x00 },    // - i
    { 0x00,0x06,0x00,0x06,0x06,0x06,0x06,0x3C },    // - j
    { 0x00,0x60,0x60,0x6C,0x78,0x6C,0x66,0x00 },    // - k
    { 0x00,0x38,0x18,0x18,0x18,0x18,0x3C,0x00 },    // - l
    { 0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00 },    // - m
    { 0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00 },    // - n
    { 0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00 },    // - o
    { 0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60 },    // - p
    { 0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06 },    // - q
    { 0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00 },    // - r
    { 0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00 },    // - s
    { 0x00,0x18,0x7E,0x18,0x18,0x18,0x0E,0x00 },    // - t
    { 0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00 },    // - u
    { 0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00 },    // - v
    { 0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00 },    // - w
    { 0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00 },    // - x
    { 0x00,0x00,0x66,0x66,0x66,0x3E,0x0C,0x78 },    // - y
    { 0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00 },    // - z
    { 0x66,0x66,0x18,0x3C,0x66,0x7E,0x66,0x00 },    // - umlaut A
    { 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18 },    // - |
    { 0x00,0x7E,0x78,0x7C,0x6E,0x66,0x06,0x00 },    // - display clear
    { 0x08,0x18,0x38,0x78,0x38,0x18,0x08,0x00 },    // - display backspace
    { 0x10,0x18,0x1C,0x1E,0x1C,0x18,0x10,0x00 },    // - display tab
};

static const uint32_t pal_0[8] = {
    0x000000FF,     // red
    0x000000FF,     // red
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x0000FF00,     // green
    0x0000FF00,     // green
};

static const uint32_t pal_1[8] = {
    0x0000FF00,     // green
    0x0000FF00,     // green
    0x00FFFF00,     // teal
    0x00FFFF00,     // teal
    0x00FF8888,     // blue
    0x00FF8888,     // blue
    0x00FF88FF,     // violet
    0x00FF88FF,     // violet
};

static void draw_string_wobble(ImDrawList* dl, const char* str, ImVec2 pos, float r, float d, const uint32_t* palette, float time, float warp0, float warp1, float warp2, float alpha) {
    uint32_t a = ((uint32_t)(255.0f * alpha))<<24;
    uint8_t chr;
    int chr_index = 0;
    while ((chr = (uint8_t)*str++) != 0) {
        uint8_t font_index = chr - 0x20;
        if (font_index >= 96) {
            continue;
        }
        float py = pos.y;
        for (int y = 0; y < 8; y++, py += d) {
            float px = pos.x + chr_index*8*d + sinf((time + (py * warp0)) * warp1) * warp2;
            uint8_t pixels = atari_font[font_index][y];
            for (int x = 7; x >= 0; x--, px += d) {
                if (pixels & (1<<x)) {
                    dl->AddRectFilled({px-r,py-r},{px+r,py+r}, palette[y]|a);
                }
            }
        }
        chr_index++;
    }
}

static float hdr_time;
static ImVec2 draw_header(ImVec2 c_pos, float win_width) {
    hdr_time += 0.05f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = 1.0f;
    const float d = 3.0f;
    #if defined(CHIP_6502)
        const char* title = "*** VISUAL 6502 ***";
    #elif defined(CHIP_Z80)
        const char* title = "*** VISUALZ80 ***";
    #elif defined(CHIP_2A03)
        const char* title = "*** VISUAL 2A03 ***";
    #endif
    const char* subtitle = "*** remix ***";
    ImVec2 pos = { (c_pos.x + (win_width - ((float)strlen(title)) * d * 8.0f) * 0.5f), c_pos.y + 10.0f };
    draw_string_wobble(dl, title, pos, r, d, pal_0, hdr_time, 0.1f, 1.0f, 5.0f, 1.0f);
    pos = { (c_pos.x + (win_width - ((float)strlen(subtitle)) * d * 8.0f) * 0.5f), c_pos.y + 10.0f + 8.0f * d };
    draw_string_wobble(dl, subtitle, pos, r, d, pal_1, hdr_time, 0.1f, 1.0f, 5.0f, 1.0f);
    return { c_pos.x, c_pos.y + 16.0f * d };
}

#define NUM_FOOTERS (27)
static const char* footers[NUM_FOOTERS] = {
    "SOWACO proudly presents",
    #if defined(CHIP_6502)
    "a 2019 production",
    "VISUAL 6502 REMIX",
    #elif defined(CHIP_Z80)
    "a 2021 production",
    "VISUAL Z80 REMIX",
    #elif defined(CHIP_2A03)
    "a 2023 production",
    "VISUAL 2A03 REMIX",
    #endif
    "Built with:",
    "visual6502",
    "perfect6502",
    "Dear ImGui",
    "ImGuiColorTextEdit",
    "ImGuiMarkdown",
    "ASMX",
    "Sokol Headers",
    "Chips Headers",
    "Earcut-Python",
    "IconFontCppHeaders",
    "Font Awesome 4",
    "Coded in C99",
    "(and a bit of C++)",
    "(and some Python)",
    "(even some JS!)",
    "compiled to WASM with",
    "EMSCRIPTEN",
    "Special Kudos to:",
    "The visual6502.org Team",
    "for reversing the 6502",
    "...and the Z80!",
    "...and of course...",
    #if defined(CHIP_6502)
    "6502 4EVER!!!",
    #elif defined(CHIP_Z80)
    "Z80 4EVER!!!",
    #elif defined(CHIP_2A03)
    "2A03 4EVER!!!",
    #endif
};

static const uint32_t pal_2[32] = {
    0x000000FF,     // red
    0x000000FF,     // red
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x0000FF00,     // green
    0x0000FF00,     // green
    0x00FFFF00,     // teal
    0x00FFFF00,     // teal
    0x00FF8888,     // blue
    0x00FF8888,     // blue
    0x00FF88FF,     // violet
    0x00FF88FF,     // violet
    0x00FF8888,     // blue
    0x00FF8888,     // blue
    0x00FFFF00,     // teal
    0x00FFFF00,     // teal
    0x0000FF00,     // green
    0x0000FF00,     // green
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x000000FF,     // red
    0x000000FF,     // red
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x0000FF00,     // green
    0x0000FF00,     // green
};


static float footer_time;
static uint32_t footer_frame_count;
static void draw_footer(ImVec2 c_pos, float win_width) {
    footer_frame_count++;
    // a small delay after opening the about box
    if (footer_frame_count < 60) {
        return;
    }
    const float time_per_msg = 4.0f;
    // stop at last message
    if (footer_time < ((NUM_FOOTERS * time_per_msg) - 1.0f)) {
        footer_time += 1.0f / 60.0f;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    //const float r = 1.0f;
    uint32_t msg_index = (uint32_t) (footer_time / time_per_msg);
    if (msg_index >= NUM_FOOTERS) {
        msg_index = NUM_FOOTERS;
    }
    const char* msg = footers[msg_index];
    float x = (c_pos.x + win_width * 0.5f);
    float y = (c_pos.y + 4 * 3.0f);

    float t = fmodf(footer_time, time_per_msg);
    float t1 = 0.0f;
    if (t < 1.0f) {
        t1 = 1.0f - t*t*t;
    }
    else if (t > (time_per_msg - 1.0f)) {
        t -= (time_per_msg - 1.0f);
        t1 = t*t*t;
    }
    float d = (30.0f * t1) + 3.0f;
    const float r = 1.0f;
    x -= strlen(msg) * 8 * d * 0.5f;
    y -= 4 * d;
    draw_string_wobble(dl, msg, {x,y}, r, d, &pal_2[23-((footer_frame_count/2)%24)], footer_time*10.0f, 5.0f, 1.0f, 1000.0f * t1, 1.0f - t1);
}

static void ui_help_about(void) {
    if (!ui_pre_check(UI_WINDOW_HELP_ABOUT)) {
        footer_time = 0.0f;
        footer_frame_count = 0;
        return;
    }
    ImVec2 disp_center = display_center();
    const float win_width = 600.0f;
    const float win_height = 420.0f;
    const float box_padding = 40.0f;
    ImGui::SetNextWindowSize({win_width, win_height}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(disp_center, ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking;
    if (ImGui::Begin(ui_window_id(UI_WINDOW_HELP_ABOUT), ui_window_open_ptr(UI_WINDOW_HELP_ABOUT), flags)) {
        ImVec2 c_pos = ImGui::GetCursorScreenPos();
        ImVec2 p = draw_header(c_pos, win_width);
        c_pos.y = p.y + 16.0f;
        c_pos.x += box_padding;
        ImGui::SetCursorScreenPos(c_pos);
        ImGui::BeginChild("##about", {win_width-2*box_padding, -40.0f}, false);
        ImGui::Markdown(dump_about_md, sizeof(dump_about_md)-1, md_conf);
        ImGui::EndChild();
        c_pos = ImGui::GetCursorScreenPos();
        c_pos.x -= 10.0f;
        draw_footer(c_pos, win_width);
    }
    ImGui::End();
}

bool ui_is_diffview(void) {
    if (ui.trace.diff_view_active) {
        return ui.trace.is_selected;
    }
    else {
        return false;
    }
}

ui_diffview_t ui_get_diffview(void) {
    ui_diffview_t res = {};
    if (ui.trace.is_selected) {
        res.cycle0 = res.cycle1 = ui.trace.selected_cycle;
        if (ui.trace.is_diff_selected) {
            res.cycle1 = ui.trace.diff_selected_cycle;
        }
        else if (ui.trace.hovered_flags != 0){
            res.cycle1 = ui.trace.hovered_cycle;
        }
    }
    return res;
}

static const char* ui_save_key(void) {
    #if defined(CHIP_Z80)
    return "floooh.v6502r.z80";
    #elif defined(CHIP_6502)
    return "floooh.v6502r.6502";
    #elif defined(CHIP_2A03)
    return "floooh.v6502r.2A03";
    #else
    #error "FIXME UNHANDLED CPU"
    #endif
}

static void ui_handle_save_settings(void) {
    if (ImGui::GetIO().WantSaveIniSettings) {
        ImGui::GetIO().WantSaveIniSettings = false;
        util_save_settings(ui_save_key(), ImGui::SaveIniSettingsToMemory());
    }
}

static void ui_load_settings(void) {
    const char* payload = util_load_settings(ui_save_key());
    if (payload) {
        ImGui::LoadIniSettingsFromMemory(payload);
        util_free_settings(payload);
    }
}
