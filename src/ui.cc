//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "imgui.h"
#include "res/fonts.h"
#include "res/iconsfontawesome4_c.h"
#include "res/markdown.h"
#include "imgui_markdown/imgui_markdown.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"

#define CHIPS_IMPL
#if defined(CHIP_6502)
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
#include "nodenames.h"

#include <math.h>

enum {
    TRACELOG_HOVERED = (1<<0),
    TIMINGDIAGRAM_HOVERED = (1<<1),
};

static struct {
    bool valid;
    ui_memedit_t memedit;
    ui_memedit_t memedit_integrated;
    #if defined(CHIP_Z80)
    ui_memedit_t ioedit;
    ui_memedit_t ioedit_integrated;
    #endif
    ui_dasm_t dasm;
    struct {
        bool cpu_controls;
        bool tracelog;
        bool timingdiagram;
        bool listing;
        bool help_asm;
        bool help_opcodes;
        bool help_about;
    } window_open;
    struct {
        bool open_source_hovered;
        bool save_source_hovered;
        bool save_binary_hovered;
        bool save_listing_hovered;
        bool cut_hovered;
        bool copy_hovered;
    } menu;
    struct {
        int hovered_flags;
        bool is_selected;
        uint32_t hovered_cycle;
        uint32_t selected_cycle;
    } trace;
    bool link_hovered;
    char link_url[MAX_LINKURL_SIZE];
} ui;

static void ui_menu(void);
static void ui_picking(void);
static void ui_tracelog_timingdiagram_new_frame();
static void ui_tracelog(void);
static void ui_timingdiagram(void);
static void ui_controls(void);
static void ui_listing(void);
static void ui_help_assembler(void);
static void ui_help_opcodes(void);
static void ui_help_about(void);
static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data);

static ImGui::MarkdownConfig md_conf;

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

void ui_init() {
    assert(!ui.valid);

    // default window open state
    ui.window_open.cpu_controls = true;
    ui.window_open.tracelog = true;
    ui.window_open.timingdiagram = true;

    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_desc.no_default_font = true;
    simgui_desc.disable_hotkeys = true;
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    //style.TabRounding = 0.0f;

    // setup ImGui font with custom icons
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(dump_fontawesome_ttf,
        sizeof(dump_fontawesome_ttf),
        16.0f, &icons_config, icons_ranges);
    ImFontConfig h1Conf;
    h1Conf.SizePixels = 26.0f;
    md_conf.headingFormats[0].font = io.Fonts->AddFontDefault(&h1Conf);

    unsigned char* font_pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
    {
        sg_image_desc desc = { };
        desc.width = font_width;
        desc.height = font_height;
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        desc.min_filter = SG_FILTER_LINEAR;
        desc.mag_filter = SG_FILTER_LINEAR;
        desc.data.subimage[0][0].ptr = font_pixels;
        desc.data.subimage[0][0].size = font_width * font_height * 4;
        io.Fonts->TexID = (ImTextureID)(uintptr_t) sg_make_image(&desc).id;
    }

    // initialize helper windows from the chips projects
    {
        ui_memedit_desc_t desc = { };
        desc.title = "Memory Editor";
        desc.open = false;
        desc.num_rows = 8;
        desc.h = 300;
        desc.x = 50;
        desc.y = 50;
        desc.hide_ascii = true;
        desc.read_cb = ui_mem_read;
        desc.write_cb = ui_mem_write;
        ui_memedit_init(&ui.memedit, &desc);
        desc.title = "Integrated Memory Editor";
        desc.hide_options = true;
        desc.hide_addr_input = true;
        ui_memedit_init(&ui.memedit_integrated, &desc);
    }
    #if defined(CHIP_Z80)
    {
        ui_memedit_desc_t desc = { };
        desc.title = "IO Editor";
        desc.open = false;
        desc.num_rows = 8;
        desc.h = 300;
        desc.x = 60;
        desc.y = 60;
        desc.hide_ascii = true;
        desc.read_cb = ui_io_read;
        desc.write_cb = ui_io_write;
        ui_memedit_init(&ui.ioedit, &desc);
        desc.title = "Integrated IO Editor";
        desc.hide_options = true;
        desc.hide_addr_input = true;
        ui_memedit_init(&ui.ioedit_integrated, &desc);
    }
    #endif
    {
        ui_dasm_desc_t desc = { };
        desc.title = "Disassembler";
        desc.open = false;
        #if defined(CHIP_6502)
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

    ui.valid = true;
}

void ui_shutdown() {
    assert(ui.valid);
    ui_asm_discard();
    ui_dasm_discard(&ui.dasm);
    ui_memedit_discard(&ui.memedit);
    ui_memedit_discard(&ui.memedit_integrated);
    #if defined(CHIP_Z80)
    ui_memedit_discard(&ui.ioedit);
    ui_memedit_discard(&ui.ioedit_integrated);
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
        ui.window_open.listing = true;
        ImGui::SetWindowFocus("Listing");
        return true;
    }
    else if (0 == strcmp(ui.link_url, "ui://assembler")) {
        ui_asm_set_window_open(true);
        ImGui::SetWindowFocus("Assembler");
        return true;
    }
    return false;
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
    if (test_alt(ev, SAPP_KEYCODE_C)) {
        ui.window_open.cpu_controls = !ui.window_open.cpu_controls;
    }
    if (test_alt(ev, SAPP_KEYCODE_T)) {
        ui.window_open.tracelog = !ui.window_open.tracelog;
    }
    if (test_alt(ev, SAPP_KEYCODE_A)) {
        ui_asm_toggle_window_open();
    }
    if (test_alt(ev, SAPP_KEYCODE_L)) {
        ui.window_open.listing = !ui.window_open.listing;
    }
    if (test_alt(ev, SAPP_KEYCODE_M)) {
        ui.memedit.open = !ui.memedit.open;
    }
    if (test_alt(ev, SAPP_KEYCODE_D)) {
        ui.dasm.open = !ui.dasm.open;
    }
    if (test_click(ev, ui.menu.open_source_hovered) || test_ctrl_shift(ev, SAPP_KEYCODE_O)) {
        util_html5_load();
        ui.menu.open_source_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.save_source_hovered) || test_ctrl_shift(ev, SAPP_KEYCODE_S)) {
        util_html5_download_string("v6502r.asm", ui_asm_source());
        ui.menu.save_source_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.save_listing_hovered)) {
        util_html5_download_string("v6502r.lst", asm_listing());
        ui.menu.save_listing_hovered = false;
    }
    if (test_click(ev, ui.menu.save_binary_hovered)) {
        util_html5_download_binary("v6502r.bin", ui_asm_get_binary());
        ui.menu.save_binary_hovered = false;
    }
    if (test_ctrl(ev, SAPP_KEYCODE_Z) || test_ctrl(ev, SAPP_KEYCODE_Y)) {
        ui_asm_undo();
        sapp_consume_event();
    }
    if (test_ctrl_shift(ev, SAPP_KEYCODE_Z) || test_ctrl_shift(ev, SAPP_KEYCODE_Y)) {
        ui_asm_redo();
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.cut_hovered) || test_ctrl(ev, SAPP_KEYCODE_X)) {
        ui_asm_cut();
        ui.menu.cut_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, ui.menu.copy_hovered) || test_ctrl(ev, SAPP_KEYCODE_C)) {
        ui_asm_copy();
        ui.menu.copy_hovered = false;
        sapp_consume_event();
    }
    if (ev->type == SAPP_EVENTTYPE_CLIPBOARD_PASTED) {
        ui_asm_paste();
    }
    if (0 != (ev->modifiers & (SAPP_MODIFIER_CTRL|SAPP_MODIFIER_ALT|SAPP_MODIFIER_SUPER))) {
        return true;
    }
    else {
        return simgui_handle_event(ev);
    }
}

void ui_frame() {
    assert(ui.valid);
    ui.link_hovered = false;
    ui_tracelog_timingdiagram_new_frame();
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60.0);
    ui_menu();
    ui_picking();
    ui_memedit_draw(&ui.memedit);
    #if defined(CHIP_Z80)
    ui_memedit_draw(&ui.ioedit);
    #endif
    ui_dasm_draw(&ui.dasm);
    ui_tracelog();
    ui_timingdiagram();
    ui_controls();
    ui_asm_draw();
    ui_listing();
    ui_help_assembler();
    ui_help_opcodes();
    ui_help_about();
    if (ui.link_hovered) {
        util_html5_cursor_to_pointer();
    }
    else {
        util_html5_cursor_to_default();
    }
}

void ui_draw() {
    assert(ui.valid);
    simgui_render();
}

static void ui_menu(void) {
    ui.menu.open_source_hovered = false;
    ui.menu.save_source_hovered = false;
    ui.menu.save_binary_hovered = false;
    ui.menu.save_listing_hovered = false;
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
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", is_osx?"Cmd+Z|Y":"Ctrl+Z|Y")) {
                ui_asm_undo();
            }
            if (ImGui::MenuItem("Redo", is_osx?"Cmd+Shift+Z|Y":"Ctrl+Shift+Z|Y")) {
                ui_asm_redo();
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
                ui_asm_paste();
            }
            #endif
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controls", "Alt+C", &ui.window_open.cpu_controls);
            ImGui::MenuItem("Trace Log", "Alt+T", &ui.window_open.tracelog);
            ImGui::MenuItem("Timing Diagram", nullptr, &ui.window_open.timingdiagram);
            ImGui::MenuItem("Listing", "Alt+L", &ui.window_open.listing);
            ImGui::MenuItem("Memory Editor", "Alt+M", &ui.memedit.open);
            #if defined(CHIP_Z80)
            ImGui::MenuItem("IO Editor", nullptr, &ui.ioedit.open);
            #endif
            ImGui::MenuItem("Disassembler", "Alt+D", &ui.dasm.open);
            if (ImGui::MenuItem("Assembler", "Alt+A", ui_asm_get_window_open())) {
                ui_asm_toggle_window_open();
            }
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
            if (ImGui::MenuItem("Default")) {
                gfx_set_layer_palette(false, (gfx_palette_t){
                    .colors = {
                        { 1.0f, 0.0f, 0.0f, 1.0f },
                        { 0.0f, 1.0f, 0.0f, 1.0f },
                        { 0.0f, 0.0f, 1.0f, 1.0f },
                        { 1.0f, 1.0f, 0.0f, 1.0f },
                        { 0.0f, 1.0f, 1.0f, 1.0f },
                        { 1.0f, 0.0f, 1.0f, 1.0f },
                    }
                });
            }
            if (ImGui::MenuItem("Visual6502")) {
                gfx_set_layer_palette(false, (gfx_palette_t){
                    .colors = {
                        { 1.0f, 0.0f, 0.0f, 1.0f },
                        { 1.0f, 1.0f, 0.0f, 1.0f },
                        { 1.0f, 0.0f, 1.0f, 1.0f },
                        { 0.3f, 1.0f, 0.3f, 1.0f },
                        { 1.0f, 0.3f, 0.3f, 1.0f },
                        { 0.5f, 0.1f, 0.75f, 1.0f },
                    }
                });
            }
            if (ImGui::MenuItem("Matrix")) {
                gfx_set_layer_palette(true, (gfx_palette_t){
                    .colors = {
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                    }
                });
            }
            if (ImGui::MenuItem("X-Ray")) {
                gfx_set_layer_palette(true, (gfx_palette_t){
                    .colors = {
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                    }
                });
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Assembler", 0, &ui.window_open.help_asm);
            ImGui::MenuItem("Opcode Table", 0, &ui.window_open.help_opcodes);
            ImGui::MenuItem("About", 0, &ui.window_open.help_about);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void ui_input_uint16(const char* label, const char* id, uint16_t addr) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label); ImGui::SameLine();
    sim_mem_w16(addr, ui_util_input_u16(id, sim_mem_r16(addr)));
}

static const char* ui_cpu_flags_as_string(uint8_t flags, char* buf, size_t buf_size) {
    assert(buf && (buf_size >= 9)); (void)buf_size;
    #if defined(CHIP_6502)
    const char* chrs[2] = { "czidbxvn", "CZIDBXVN" };
    #else
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
    const uint8_t ir = sim_6502_get_ir();
    const uint8_t p = sim_get_flags();
    ImGui::Text("A:%02X X:%02X Y:%02X SP:%02X PC:%04X",
        sim_6502_get_a(), sim_6502_get_x(), sim_6502_get_y(), sim_6502_get_sp(), sim_get_pc());
    char p_buf[9];
    ImGui::Text("P:%02X (%s) Cycle: %d", p, ui_cpu_flags_as_string(sim_get_flags(), p_buf, sizeof(p_buf)), sim_get_cycle()>>1);
    ImGui::Text("IR:%02X\n", ir);
    ImGui::Text("Data:%02X Addr:%04X %s %s %s",
        sim_get_data(), sim_get_addr(),
        sim_6502_get_rw()?"R":"W",
        sim_6502_get_clk0()?"CLK0":"    ",
        sim_6502_get_sync()?"SYNC":"    ");
    ImGui::Separator();
    ui_input_uint16("NMI vector (FFFA): ", "##nmi_vec", 0xFFFA);
    ui_input_uint16("RES vector (FFFC): ", "##res_vec", 0xFFFC);
    ui_input_uint16("IRQ vector (FFFE): ", "##irq_vec", 0xFFFE);
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
    ImGui::Text("WZ:%04X  I:%02X  R:%02X F:%s", sim_z80_get_wz(), sim_z80_get_i(), sim_z80_get_r(), ui_cpu_flags_as_string(sim_z80_get_f(), f_buf, sizeof(f_buf)));
    ImGui::Text("Data:%02X Addr:%04X %s %s %s %s",
        sim_get_data(), sim_get_addr(),
        sim_z80_get_m1() ? "  ":"M1",
        sim_z80_get_mreq() ? "    ":"MREQ",
        sim_z80_get_rd() ? "  ":"RD",
        sim_z80_get_wr() ? "  ":"WR");
}
#endif

static void ui_cassette_deck_controls(void) {
    const char* tooltip = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 10.0f } );
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 7.0f, 7.0f } );
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFFFFFFFF);
    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF000000);
    if (ImGui::Button(ICON_FA_STEP_BACKWARD, { 28, 25 })) {
        sim_set_paused(true);
        trace_revert_to_previous();
    }
    if (ImGui::IsItemHovered()) {
        tooltip = "Step back one half-cycle";
    }
    ImGui::SameLine();
    if (sim_get_paused()) {
        if (ImGui::Button(ICON_FA_PLAY, { 28, 25 })) {
            sim_set_paused(false);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Run (one half-cycle per frame)";
        }
    }
    else {
        if (ImGui::Button(ICON_FA_PAUSE, { 28, 25 })) {
            sim_set_paused(true);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Pause";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_STEP_FORWARD, { 28, 25 })) {
        sim_set_paused(true);
        sim_step(1);
    }
    if (ImGui::IsItemHovered()) {
        tooltip = "Step one half-cycle";
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FAST_FORWARD, { 28, 25 })) {
        sim_set_paused(true);
        sim_step(2);
    }
    if (ImGui::IsItemHovered()) {
        tooltip = "Step two half-cycles";
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_RIGHT, { 28, 25 })) {
        sim_set_paused(true);
        sim_step_op();
    }
    if (ImGui::IsItemHovered()) {
        tooltip = "Step to next instruction";
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_EJECT, { 28, 25 })) {
        trace_clear();
        sim_shutdown();
        sim_init();
        sim_start();
    }
    if (ImGui::IsItemHovered()) {
        tooltip = "Reset";
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    if (!tooltip) {
        tooltip = sim_get_paused() ? "Paused" : "Running";
    }
    ImGui::Text("%s", tooltip);
}

static void ui_controls(void) {
    if (!ui.window_open.cpu_controls) {
        return;
    }
    const float disp_w = (float) sapp_width();
    ImGui::SetNextWindowPos({ disp_w - 300, 50 }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 270, 480 }, ImGuiCond_Once);
    #if defined(CHIP_6502)
    const char* cpu_name = "MOS 6502";
    #else
    const char* cpu_name = "Zilog Z80";
    #endif
    if (ImGui::Begin(cpu_name, &ui.window_open.cpu_controls, ImGuiWindowFlags_None)) {
        /* cassette deck controls */
        ui_cassette_deck_controls();
        ImGui::Separator();

        /* CPU state */
        ui_cpu_status_panel();
        ImGui::Separator();

        /* memory dump */
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Memory")) {
                ui_memedit_draw_content(&ui.memedit_integrated);
                ImGui::EndTabItem();
            }
            #if defined(CHIP_Z80)
            if (ImGui::BeginTabItem("IO")) {
                ui_memedit_draw_content(&ui.ioedit_integrated);
                ImGui::EndTabItem();
            }
            #endif
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

static void ui_listing(void) {
    if (!ui.window_open.listing) {
        return;
    }
    ImGui::SetNextWindowPos({60, 320}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({480, 200}, ImGuiCond_Once);
    if (ImGui::Begin("Listing", &ui.window_open.listing, ImGuiWindowFlags_None)) {
        ImGui::Text("%s", asm_listing());
    }
    ImGui::End();
}

static void ui_tracelog_timingdiagram_new_frame() {
    if (ui.trace.is_selected && !trace_empty()) {
        if ((ui.trace.selected_cycle < trace_get_cycle(trace_num_items()-1)) ||
            (ui.trace.selected_cycle > trace_get_cycle(0)))
        {
            ui.trace.is_selected = false;
        }
    }
}

#if defined(CHIP_6502)
static const int ui_tracelog_table_num_columns = 18;

static void ui_tracelog_table_setup_columns(void) {
    const float char_width = 8.0f;
    ImGui::TableSetupColumn("Cycle/h", ImGuiTableColumnFlags_None, 6*char_width);
    ImGui::TableSetupColumn("SYNC", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("RW", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("AB", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("DB", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("PC", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("IR", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("S", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("P", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_NoClip, 8*char_width);
    ImGui::TableSetupColumn("IRQ", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3*char_width);
    ImGui::TableSetupColumn("NMI", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3*char_width);
    ImGui::TableSetupColumn("RES", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3*char_width);
    ImGui::TableSetupColumn("RDY", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 3*char_width);
    ImGui::TableSetupColumn("Asm", ImGuiTableColumnFlags_NoClip, 8*char_width);
}

static void ui_tracelog_table_row(int trace_index) {
    const uint32_t cur_cycle = trace_get_cycle(trace_index);
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%5d/%d", cur_cycle >> 1, cur_cycle & 1);
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_6502_get_sync(trace_index)?"SYNC":"    ");
    ImGui::TableNextColumn();
    ImGui::Text("%c", trace_6502_get_rw(trace_index)?'R':'W');
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_get_addr(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_get_data(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_get_pc(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_6502_get_ir(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_6502_get_a(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_6502_get_x(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_6502_get_y(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_6502_get_sp(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_get_flags(trace_index));
    ImGui::TableNextColumn();
    char p_buf[9];
    ImGui::Text("%s", ui_cpu_flags_as_string(trace_get_flags(trace_index), p_buf, sizeof(p_buf)));
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_6502_get_irq(trace_index)?"   ":"IRQ");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_6502_get_nmi(trace_index)?"   ":"NMI");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_6502_get_res(trace_index)?"   ":"RES");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_6502_get_rdy(trace_index)?"   ":"RDY");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_get_disasm(trace_index));
}
#elif defined(CHIP_Z80)
static const int ui_tracelog_table_num_columns = 27;

static void ui_tracelog_table_setup_columns(void) {
    const float char_width = 8.0f;
    ImGui::TableSetupColumn("Cycle/h", ImGuiTableColumnFlags_None, 6*char_width);
    ImGui::TableSetupColumn("M1", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("MREQ", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("IORQ", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("RFSH", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("RD", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("WR", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("AB", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("DB", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("PC", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("IR", ImGuiTableColumnFlags_NoClip, 2*char_width);
    ImGui::TableSetupColumn("AF", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("BC", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("DE", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("HL", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("AF'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4*char_width);
    ImGui::TableSetupColumn("BC'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4*char_width);
    ImGui::TableSetupColumn("DE'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4*char_width);
    ImGui::TableSetupColumn("HL'", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4*char_width);
    ImGui::TableSetupColumn("IX", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("IY", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("SP", ImGuiTableColumnFlags_NoClip, 4*char_width);
    ImGui::TableSetupColumn("WZ", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 4*char_width);
    ImGui::TableSetupColumn("I", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 2*char_width);
    ImGui::TableSetupColumn("R", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_DefaultHide, 2*char_width);
    ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_None, 8*char_width);
    ImGui::TableSetupColumn("Asm", ImGuiTableColumnFlags_NoClip|ImGuiTableColumnFlags_None, 12*char_width);
}

static void ui_tracelog_table_row(int trace_index) {
    const uint32_t cur_cycle = trace_get_cycle(trace_index);
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%5d/%d", cur_cycle >> 1, cur_cycle & 1);
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_z80_get_m1(trace_index)?"":"M1");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_z80_get_mreq(trace_index)?"":"MREQ");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_z80_get_ioreq(trace_index)?"":"IORQ");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_z80_get_rfsh(trace_index)?"":"RFSH");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_z80_get_rd(trace_index)?"":"RD");
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_z80_get_wr(trace_index)?"":"WR");
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_get_addr(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_get_data(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_get_pc(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_z80_get_ir(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_af(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_bc(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_de(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_hl(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_af2(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_bc2(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_de2(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_hl2(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_ix(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_iy(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_sp(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%04X", trace_z80_get_wz(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_z80_get_i(trace_index));
    ImGui::TableNextColumn();
    ImGui::Text("%02X", trace_z80_get_r(trace_index));
    ImGui::TableNextColumn();
    char f_buf[9];
    ImGui::Text("%s", ui_cpu_flags_as_string(trace_get_flags(trace_index), f_buf, sizeof(f_buf)));
    ImGui::TableNextColumn();
    ImGui::Text("%s", trace_get_disasm(trace_index));
}
#endif

static uint32_t ui_trace_bgcolor(uint32_t flipbits) {
    static const uint32_t colors[4] = { 0xFF327D2E, 0xFF3C8E38, 0xFFC06515, 0xFFD17619 };
    return colors[flipbits & 3];
}

static const uint32_t ui_trace_hovered_color = 0xFF3643F4;
static const uint32_t ui_trace_selected_color = 0xFF0000D5;

static void ui_tracelog(void) {
    if (!ui.window_open.tracelog) {
        return;
    }
    const float disp_w = (float) sapp_width();
    const float disp_h = (float) sapp_height();
    const float footer_h = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::SetNextWindowPos({ disp_w / 2, disp_h - 150 }, ImGuiCond_Once, { 0.5f, 0.0f });
    ImGui::SetNextWindowSize({ 600, 128 }, ImGuiCond_Once);
    bool any_hovered = false;
    if (ImGui::Begin("Trace Log", &ui.window_open.tracelog, ImGuiWindowFlags_None)) {
        const ImGuiTableFlags table_flags =
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_Hideable |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV |
            ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("##trace_data", ui_tracelog_table_num_columns, table_flags, {0.0f, -footer_h})) {
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
                        bg_color = ui_trace_hovered_color;
                    }
                    if (ui.trace.is_selected && (ui.trace.selected_cycle == cur_cycle)) {
                        bg_color = ui_trace_selected_color;
                    }
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg_color);
                    const ImVec2 min_p = ImGui::GetCursorScreenPos();
                    ui_tracelog_table_row(trace_index);
                    ImVec2 max_p = ImGui::GetCursorScreenPos();
                    max_p.x = min_p.x + ImGui::GetWindowContentRegionWidth();
                    const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(min_p, max_p, false);
                    if (hovered) {
                        any_hovered = true;
                        ui.trace.hovered_flags |= TRACELOG_HOVERED;
                        ui.trace.hovered_cycle = cur_cycle;
                        any_hovered = true;
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            ui.trace.is_selected = true;
                            ui.trace.selected_cycle = cur_cycle;
                        }
                    }
                }
            }
            if (trace_ui_get_scroll_to_end()) {
                trace_ui_set_scroll_to_end(false);
                ImGui::SetScrollHereY();
            }
            ImGui::EndTable();
            ImGui::Separator();
            if (ImGui::Button("Clear Log")) {
                trace_clear();
            }
            if (ui.trace.is_selected) {
                ImGui::SameLine();
                if (ImGui::Button("Revert to Selected")) {
                    trace_revert_to_cycle(ui.trace.selected_cycle);
                }
            }
        }
    }
    ImGui::End();
    if (!any_hovered) {
        ui.trace.hovered_flags &= ~TRACELOG_HOVERED;
    }
}

#if defined(CHIP_6502)
static const int num_time_diagram_nodes = 7;
static struct { uint32_t node; const char* name; } time_diagram_nodes[num_time_diagram_nodes] = {
    { p6502_clk0, "CLK0" },
    { p6502_sync, "SYNC" },
    { p6502_rw, "RW" },
    { p6502_irq, "IRQ" },
    { p6502_nmi, "NMI" },
    { p6502_res, "RES" },
    { p6502_rdy, "RDY" },
};
#elif defined(CHIP_Z80)
static const int num_time_diagram_nodes = 14;
static struct { uint32_t node; const char* name; } time_diagram_nodes[num_time_diagram_nodes] = {
    { pz80_clk, "CLK" },
    { pz80__m1, "M1" },
    { pz80__mreq, "MREQ" },
    { pz80__iorq, "IORQ" },
    { pz80__rd, "RD" },
    { pz80__wr, "WR" },
    { pz80__rfsh, "RFSH" },
    { pz80__halt, "HALT" },
    { pz80__wait, "WAIT" },
    { pz80__int, "INT" },
    { pz80__nmi, "NMI" },
    { pz80__reset, "RESET" },
    { pz80__busrq, "BUSRQ" },
    { pz80__busak, "BUSAK" },
};
#endif

static void ui_timingdiagram(void) {
    if (!ui.window_open.timingdiagram) {
        return;
    }
    const float cell_padding = 5.0f;
    const float cell_height = 2 * cell_padding + ImGui::GetTextLineHeight();
    const float cell_width  = cell_height;
    const int num_cols = (int)trace_num_items();
    const float graph_height = num_time_diagram_nodes * cell_height;
    const float graph_width  = num_cols * cell_width;
    ImGui::SetNextWindowPos({60, 60}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({300, 400}, ImGuiCond_Once);
    ImGui::SetNextWindowContentSize({graph_width, graph_height});
    bool any_hovered = false;
    if (ImGui::Begin("Timing Diagram", &ui.window_open.timingdiagram, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 dl_orig = ImGui::GetCursorScreenPos();
        const ImVec2 w_orig = ImGui::GetCursorPos();

        // draw vertical background stripes
        const int right_col = num_cols - 1 - ((ImGui::GetScrollX() + ImGui::GetWindowWidth()) / cell_width);
        const int left_col = num_cols - 1 - ((ImGui::GetScrollX() - cell_width) / cell_width);
        assert(left_col >= right_col);
        for (int col_index = right_col; col_index <= left_col; col_index++) {
            if ((col_index < 0) || (col_index >= num_cols)) {
                continue;
            }
            const int trace_index = num_cols - 1 - col_index;
            const uint32_t cur_cycle = trace_get_cycle(trace_index);
            const float x0 = dl_orig.x + trace_index * cell_width;
            const float x1 = x0 + cell_width;
            const float y0 = dl_orig.y;
            const float y1 = y0 + graph_height;
            uint32_t bg_color = ui_trace_bgcolor(trace_get_flipbits(trace_index));
            if ((0 != ui.trace.hovered_flags) && (ui.trace.hovered_cycle == cur_cycle)) {
                bg_color = ui_trace_hovered_color;
            }
            if (ui.trace.is_selected && (ui.trace.selected_cycle == cur_cycle)) {
                bg_color = ui_trace_selected_color;
            }
            dl->AddRectFilled({x0,y0}, {x1,y1}, bg_color);
            if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect({x0,y0}, {x1,y1}, false)) {
                any_hovered = true;
                ui.trace.hovered_flags |= TIMINGDIAGRAM_HOVERED;
                ui.trace.hovered_cycle = cur_cycle;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    ui.trace.is_selected = true;
                    ui.trace.selected_cycle = cur_cycle;
                }
            }
            if ((trace_index == 0) || ((trace_index >= 1) && (0 != strcmp(trace_get_disasm(trace_index), trace_get_disasm(trace_index - 1))))) {
                dl->AddText({x0,y0}, 0xFFFFFFFF, trace_get_disasm(trace_index));
            }
        }
        for (int line = 0; line < num_time_diagram_nodes; line++) {
            float last_y = 0.0f;
            for (int col_index = right_col; col_index <= left_col; col_index++) {
                if ((col_index < 0) || (col_index >= num_cols)) {
                    continue;
                }
                int trace_index = num_cols - 1 - col_index;
                float x = dl_orig.x + trace_index * cell_width;
                float y = line * cell_height + cell_height * 0.5f + dl_orig.y;
                bool clk = trace_is_node_high(trace_index, time_diagram_nodes[line].node);
                if (clk) {
                    y += cell_height * 0.25f;
                }
                else {
                    y -= cell_height * 0.25f;
                }
                dl->AddLine({x,y}, {x+cell_width,y}, 0xFFFFFFFF);
                if (last_y != 0.0f) {
                    dl->AddLine({x+cell_width,y}, {x+cell_width,last_y}, 0xFFFFFFFF);
                }
                last_y = y;
            }
            const char* node_name = time_diagram_nodes[line].name;
            ImGui::SetCursorPos({ImGui::GetScrollX() + w_orig.x, line * cell_height + cell_padding + w_orig.y });
            ImGui::Text("%s", node_name);
        }
    }
    ImGui::End();
    if (!any_hovered) {
        ui.trace.hovered_flags &= ~TIMINGDIAGRAM_HOVERED;
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
                if (!sim_is_ignore_picking_highlight_node(node_index) && (node_names[node_index][0] != 0)) {
                    if (valid_node_index != 0) {
                        strcat(str, "\n");
                    }
                    valid_node_index++;
                    strcat(str, node_names[node_index]);
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
    return ImVec2((float)sapp_width()*0.5f, (float)sapp_height()*0.5f);
}

static void ui_help_assembler(void) {
    if (!ui.window_open.help_asm) {
        return;
    }
    ImGui::SetNextWindowSize({640, 400}, ImGuiCond_Once);
    ImGui::SetNextWindowPos(display_center(), ImGuiCond_Once, { 0.5f, 0.5f });
    if (ImGui::Begin("Assembler Help", &ui.window_open.help_asm, ImGuiWindowFlags_None)) {
        ImGui::Markdown(dump_help_assembler_md, sizeof(dump_help_assembler_md)-1, md_conf);
    }
    ImGui::End();
}

static void ui_help_opcodes(void) {
    if (!ui.window_open.help_opcodes) {
        return;
    }
    ImGui::SetNextWindowSize({640, 400}, ImGuiCond_Once);
    if (ImGui::Begin("Opcode Help", &ui.window_open.help_opcodes, ImGuiWindowFlags_None)) {
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
    #endif
    const char* subtitle = "*** remix ***";
    ImVec2 pos = { (c_pos.x + (win_width - ((float)strlen(title)) * d * 8.0f) * 0.5f), c_pos.y + 10.0f };
    draw_string_wobble(dl, title, pos, r, d, pal_0, hdr_time, 0.1f, 1.0f, 5.0f, 1.0f);
    pos = { (c_pos.x + (win_width - ((float)strlen(subtitle)) * d * 8.0f) * 0.5f), c_pos.y + 10.0f + 8.0f * d };
    draw_string_wobble(dl, subtitle, pos, r, d, pal_1, hdr_time, 0.1f, 1.0f, 5.0f, 1.0f);
    return { c_pos.x, c_pos.y + 16.0f * d };
}

#define NUM_FOOTERS (26)
static const char* footers[NUM_FOOTERS] = {
    "SOWACO proudly presents",
    "a 2019 production",
    #if defined(CHIP_6502)
    "VISUAL 6502 REMIX",
    #elif defined(CHIP_Z80)
    "VISUAL Z80 REMIX",
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
    "Tripy",
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
    "for reversing the 6502 and Z80!",
    "...and of course...",
    #if defined(CHIP_6502)
    "6502 4EVER!!!",
    #elif defined(CHIP_Z80)
    "Z80 4EVER!!!",
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
    if (!ui.window_open.help_about) {
        footer_time = 0.0f;
        footer_frame_count = 0;
        return;
    }
    ImVec2 disp_center = display_center();
    const float win_width = 600.0f;
    const float win_height = 420.0f;
    const float box_padding = 40.0f;
    ImGui::SetNextWindowSize({win_width, win_height}, ImGuiCond_Once);
    ImGui::SetNextWindowPos(disp_center, ImGuiCond_Once, { 0.5f, 0.5f });
    if (ImGui::Begin("About", &ui.window_open.help_about, ImGuiWindowFlags_NoResize)) {
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
