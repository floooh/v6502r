import {
    Builder,
    Configurer,
    log,
    proj,
    Project,
    TargetBuilder,
} from "jsr:@floooh/fibs@^1";
import { copy, ensureDirSync, existsSync } from "jsr:@std/fs@^1";

export function configure(c: Configurer) {
    c.addImportOptions({
        emscripten: {
            // we're using our own shell file templates
            useMinimalShellFile: false,
        },
    });
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
    c.addCommand({
        name: "webpage",
        help: webpageCmdHelp,
        run: webpageCmdRun,
    });
}

export function build(b: Builder) {
    b.addIncludeDirectories(["ext"]);
    if (b.isMsvc()) {
        b.addCompileOptions(["/wd4244"]); // conversion from X to Y, possible loss of data
    }
    const commonSources = [
        "main.c",
        "input.c",
        "input.h",
        "util.c",
        "util.h",
        "gfx.c",
        "gfx.h",
        "pick.c",
        "pick.h",
        "sim.c",
        "sim.h",
        "trace.c",
        "trace.h",
        "asm.c",
        "asm.h",
        "ui.cc",
        "ui.h",
        "ui_asm.cc",
        "ui_asm.h",
        "imgui_util.cc",
        "imgui_util.h",
        "common.h",
    ];
    b.addTarget("v6502r", "windowed-exe", (t) => {
        t.setDir("src");
        t.addSources(commonSources);
        t.addIncludeDirectories(["m6502"]);
        t.addSources([
            "m6502/segdefs.c",
            "m6502/segdefs.h",
            "m6502/nodenames.c",
            "m6502/nodenames.h",
            "m6502/nodegroups.h",
        ]);
        t.addDependencies(["assets", "perfect6502", "asmx_6502"]);
        t.addCompileDefinitions({ CHIP_6502: "1" });
        if (b.isEmscripten()) {
            t.addLinkOptions([
                `--shell-file=${b.projectDir()}/src/m6502/shell.html`,
            ]);
        }
    });
    b.addTarget("vz80r", "windowed-exe", (t) => {
        t.setDir("src");
        t.addSources(commonSources);
        t.addIncludeDirectories(["z80"]);
        t.addSources([
            "z80/segdefs.c",
            "z80/segdefs.h",
            "z80/nodenames.c",
            "z80/nodenames.h",
            "z80/nodegroups.h",
        ]);
        t.addDependencies(["assets", "perfectz80", "asmx_z80"]);
        t.addCompileDefinitions({ CHIP_Z80: "1" });
        if (b.isEmscripten()) {
            t.addLinkOptions([
                `--shell-file=${b.projectDir()}/src/z80/shell.html`,
            ]);
        }
    });
    b.addTarget("v2a03r", "windowed-exe", (t) => {
        t.setDir("src");
        t.addSources(commonSources);
        t.addIncludeDirectories(["2a03"]);
        t.addSources([
            "2a03/segdefs.c",
            "2a03/segdefs.h",
            "2a03/nodenames.c",
            "2a03/nodenames.h",
            "2a03/nodegroups.h",
        ]);
        t.addDependencies(["assets", "perfect2a03", "asmx_6502"]);
        t.addCompileDefinitions({ CHIP_2A03: "1" });
        if (b.isEmscripten()) {
            t.addLinkOptions([
                `--shell-file=${b.projectDir()}/src/2a03/shell.html`,
            ]);
        }
    });
    const addAsmxCompileOptions = (t: TargetBuilder) => {
        if (b.isGcc() || b.isClang()) {
            t.addCompileOptions({
                scope: "private",
                opts: ["-Wno-implicit-fallthrough"],
            });
            if (b.isGcc()) {
                t.addCompileOptions({
                    scope: "private",
                    opts: ["-Wno-misleading-indentation"],
                });
            }
        } else if (b.isMsvc()) {
            t.addCompileOptions({
                scope: "private",
                opts: [
                    "/wd4459", // hides global declaration...
                    "/wd4210", // nonstandard extension used
                    "/wd4701", // potentially uninitialized local variable used
                    "/wd4245", // conversion from X to Y
                ],
            });
        }
    };
    b.addTarget("asmx_6502", "lib", (t) => {
        t.setDir("ext/asmx/src");
        t.addSources(["asmx.c", "asmx.h", "asm6502.c"]);
        t.addIncludeDirectories(["."]);
        t.addCompileDefinitions({ ASMX_6502: "1" });
        addAsmxCompileOptions(t);
    });
    b.addTarget("asmx_z80", "lib", (t) => {
        t.setDir("ext/asmx/src");
        t.addSources(["asmx.c", "asmx.h", "asmz80.c"]);
        t.addIncludeDirectories(["."]);
        t.addCompileDefinitions({ ASMX_Z80: "1" });
        addAsmxCompileOptions(t);
    });
    b.addTarget("perfect6502", "lib", (t) => {
        t.setDir("ext/perfect6502");
        t.addSources([
            "netlist_6502.h",
            "types.h",
            "netlist_sim.h",
            "netlist_sim.c",
            "perfect6502.h",
            "perfect6502.c",
        ]);
        t.addIncludeDirectories(["."]);
    });
    b.addTarget("perfectz80", "lib", (t) => {
        t.setDir("ext/perfect6502");
        t.addSources([
            "netlist_z80.h",
            "types.h",
            "netlist_sim.h",
            "netlist_sim.c",
            "perfectz80.h",
            "perfectz80.c",
        ]);
        t.addIncludeDirectories(["."]);
    });
    b.addTarget("perfect2a03", "lib", (t) => {
        t.setDir("ext/perfect6502");
        t.addSources([
            "netlist_2a03.h",
            "types.h",
            "netlist_sim.h",
            "netlist_sim.c",
            "perfect2a03.h",
            "perfect2a03.c",
        ]);
        t.addIncludeDirectories(["."]);
    });
    b.addTarget("sokolimpl", "lib", (t) => {
        t.setDir("ext/sokol");
        t.addSources(["sokol.c"]);
        t.addDependencies(["sokol", "imgui-docking"]);
    });
    b.addTarget("texteditor", "lib", (t) => {
        t.setDir("ext/texteditor");
        t.addSources(["TextEditor.cpp", "TextEditor.h"]);
        t.addIncludeDirectories(["."]);
        t.addDependencies(["imgui-docking"]);
        if (b.isClang()) {
            t.addCompileOptions({
                opts: ["-Wno-unused-but-set-variable"],
                scope: "private",
            });
        } else if (b.isMsvc()) {
            t.addCompileOptions({
                opts: ["/wd4189"], // local variable is initialized but no referenced
                scope: "private",
            });
        }
    });
    b.addTarget("assets", "interface", (t) => {
        const shdcOutDir = t.buildDir();
        t.setDir("src");
        t.addSources(["gfx.glsl", "res/help_assembler.md", "res/about.md"]);
        t.addJob({
            job: "sokolshdc",
            args: { src: "gfx.glsl", outDir: shdcOutDir },
        });
        t.addIncludeDirectories([shdcOutDir]);
        t.addJob({
            job: "embedfiles",
            args: {
                outHeader: "res/fonts.h",
                prefix: "dump_",
                files: ["res/fontawesome.ttf"],
            },
        });
        t.addJob({
            job: "embedfiles",
            args: {
                outHeader: "res/markdown.h",
                asText: true,
                prefix: "dump_",
                files: ["res/help_assembler.md", "res/about.md"],
            },
        });
        t.addDependencies(["sokolimpl", "texteditor"]);
    });
}

function webpageCmdHelp() {
    log.helpCmd([
        "webpage build [vz80r|v6502r|v2a03r]",
        "webpage serve [vz80r|v6502r|v2a03r]",
    ], "build or serve webpage");
}

async function webpageCmdRun(p: Project, args: string[]) {
    const subcmd = args[1];
    const targetName = args[2];
    if (!["build", "serve"].includes(subcmd)) {
        throw new Error(`expected subcmd 'build' or 'serve' (run 'fibs help webpage')`);
    }
    if (!["vz80r", "v6502r", "v2a03r"].includes(targetName)) {
        throw new Error(`expected target name 'vz80r', 'v6502r' or 'v2a03r' (run 'fibs help webpage')`);
    }
    const configName = "emsc-ninja-release";
    const config = p.config(configName);
    const srcDir = p.targetDistDir(targetName, configName);
    const dstDir = `${p.fibsDir()}/webpage/${targetName}`;
    if (subcmd === "build") {
        if (existsSync(dstDir)) {
            if (log.ask(`Ok to delete directory ${dstDir}?`, false)) {
                Deno.removeSync(dstDir, { recursive: true });
            }
        }
        ensureDirSync(dstDir);
        await proj.generate(config);
        await proj.build({ buildTarget: targetName, forceRebuild: true });
        await copy(`${srcDir}/${targetName}.html`, `${dstDir}/index.html`);
        await copy(
            `${srcDir}/${targetName}.wasm`,
            `${dstDir}/${targetName}.wasm`,
        );
        await copy(`${srcDir}/${targetName}.js`, `${dstDir}/${targetName}.js`);
    } else if (subcmd === "serve") {
        const emsc = p.importModule("platforms", "emscripten.ts");
        emsc.emrun(p, { cwd: dstDir, file: "index.html" });
    }
}
