// deno-lint-ignore no-unversioned-import
import { Builder, Configurer } from "jsr:@floooh/fibs";

export function configure(c: Configurer) {
    c.addImport({
        name: "libs",
        url: "https://github.com/floooh/fibs-libs",
        files: ["sokol.ts"],
    });
    c.addImport({
        name: "platforms",
        url: "https://github.com/floooh/fibs-platforms",
        files: ["emscripten.ts"],
    });
    c.addImport({
        name: "utils",
        url: "https://github.com/floooh/fibs-utils",
        files: ["stdoptions.ts", "sokolshdc.ts", "embedfiles.ts"],
    });
    c.addImport({
        name: "dcimgui",
        url: "https://github.com/floooh/dcimgui",
        files: ["fibs-docking.ts"],
    });
}

export function build(b: Builder) {
    b.addTarget("asmx_6502", "lib", (t) => {
        t.setDir("ext/asmx/src");
        t.addSources(['asmx.c', 'asmx.h', 'asm6502.c']);
        t.addIncludeDirectories(['.']);
        t.addCompileDefinitions({ ASMX_6502: '1' });
        if (b.isGcc() || b.isClang()) {
            t.addCompileOptions(['-Wno-implicit-fallthrough']);
        };
    });
    b.addTarget("asmx_z80", "lib", (t) => {
        t.setDir("ext/asmx/src");
        t.addSources(['asmx.c', 'asmx.h', 'asmz80.c']);
        t.addIncludeDirectories(['.']);
        t.addCompileDefinitions({ ASMX_Z80: '1' });
        if (b.isGcc() || b.isClang()) {
            t.addCompileOptions({ opts: ['-Wno-implicit-fallthrough'], scope: 'private' });
        };
    });
    b.addTarget("perfect6502", "lib", (t) => {
        t.setDir("ext/perfect6502");
        t.addSources(["netlist_6502.h", "types.h", "netlist_sim.h", "netlist_sim.c", "perfect6502.h", "perfect6502.c"]);
        t.addIncludeDirectories(['.']);
    });
    b.addTarget("perfectz80", "lib", (t) => {
        t.setDir("ext/perfect6502");
        t.addSources(["netlist_z80.h", "types.h", "netlist_sim.h", "netlist_sim.c", "perfectz80.h", "perfectz80.c"]);
        t.addIncludeDirectories(['.']);
    });
    b.addTarget("perfect2a03", "lib", (t) => {
        t.setDir("ext/perfect6502");
        t.addSources(["netlist_2a03.h", "types.h", "netlist_sim.h", "netlist_sim.c", "perfect2a03.h", "perfect2a03.c"]);
        t.addIncludeDirectories(['.']);
    });
    b.addTarget("sokolimpl", "lib", (t) => {
        t.setDir("ext/sokol");
        t.addSources(["sokol.c"]);
        t.addDependencies(["sokol", "imgui-docking"]);
    });
    b.addTarget("texteditor", "lib", (t) => {
        t.setDir("ext/texteditor");
        t.addSources(["TextEditor.cpp", "TextEditor.h"]);
        t.addIncludeDirectories(['.']);
        t.addDependencies(["imgui-docking"]);
        if (b.isClang()) {
            t.addCompileOptions({ opts: ['-Wno-unused-but-set-variable'], scope: 'private' });
        }
    });
    b.addTarget("assets", "interface", (t) => {
        t.setDir("src/res");
        t.addSources(['help_assembler.md', 'about.md'])
        t.addJob({
            job: "embedfiles",
            args: { outHeader: "fonts.h", files: ["fontawesome.ttf"] },
        });
        t.addJob({
            job: "embedfiles",
            args: {
                outHeader: "markdown.h",
                asText: true,
                files: ["help_assembler.md", "about.md"],
            },
        });
        t.addDependencies(['sokol', 'texteditor']);
    });
}
