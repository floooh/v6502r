/*
    build-source-distro: fibs command which builds a zipped source distro
    with all dependencies and a plain Makefile for macOS and Linux
    (including Raspberry Pi) in .fibs/source-distro
*/
import { CommandDesc, log, proj, Project, util } from "jsr:@floooh/fibs@^1";
import { copy, ensureDirSync, existsSync, walk } from "jsr:@std/fs@^1";
import { dirname, relative } from "jsr:@std/path@^1";
import {
    BlobWriter,
    Uint8ArrayReader,
    ZipWriter,
} from "jsr:@zip-js/zip-js@^2";

// commonSources: the src/ files shared by all executables
export function buildSourceDistroCommand(commonSources: string[]): CommandDesc {
    return {
        name: "build-source-distro",
        help,
        run: (p: Project, args: string[]) => run(p, args, commonSources),
    };
}

// chip-specific settings for the source distro Makefile
const distroExes = [
    {
        name: "v6502r",
        dir: "m6502",
        defines: "-DCHIP_6502=1 -DASMX_6502=1",
        sources: ["ext/asmx/src/asm6502.c", "ext/perfect6502/perfect6502.c"],
    },
    {
        name: "vz80r",
        dir: "z80",
        defines: "-DCHIP_Z80=1 -DASMX_Z80=1",
        sources: ["ext/asmx/src/asmz80.c", "ext/perfect6502/perfectz80.c"],
    },
    {
        name: "v2a03r",
        dir: "2a03",
        defines: "-DCHIP_2A03=1 -DASMX_6502=1",
        sources: ["ext/asmx/src/asm6502.c", "ext/perfect6502/perfect2a03.c"],
    },
];

function help() {
    log.helpCmd([
        "build-source-distro",
        "build-source-distro [target-dir]",
    ], [
        "build a source distro zip archive with all dependencies and a Makefile",
        "for macOS and Linux in .fibs/source-distro, optionally copy the zip to target-dir",
    ]);
}

async function run(p: Project, args: string[], commonSources: string[]) {
    const workDir = `${p.fibsDir()}/source-distro`;
    const stageDir = `${workDir}/v6502r-src`;
    const zipPath = `${workDir}/v6502r-src.zip`;
    if (existsSync(stageDir)) {
        Deno.removeSync(stageDir, { recursive: true });
    }

    // make sure the embedded resource headers in src/res are up to date
    await proj.configureTargets(p.activeConfig());
    const embedJobs = p.target("assets").resolvedJobs.filter((job) => job.name === "embedfiles");
    for (const job of embedJobs) {
        await job.func(job.inputs, job.outputs, job.args);
    }

    // copy sources and dependencies into the staging directory
    const copyFiles = async (srcDir: string, dstDir: string, names: string[]) => {
        for (const name of names) {
            const dst = `${stageDir}/${dstDir}/${name}`;
            ensureDirSync(dirname(dst));
            await copy(`${srcDir}/${name}`, dst);
        }
    };
    const srcDir = `${p.dir()}/src`;
    await copyFiles(srcDir, "src", [
        ...commonSources,
        "res/fonts.h",
        "res/markdown.h",
        "res/iconsfontawesome4_c.h",
        "res/readme.txt",
    ]);
    for (const exe of distroExes) {
        await copyFiles(srcDir, "src", ["segdefs.c", "segdefs.h", "nodenames.c", "nodenames.h", "nodegroups.h"].map((f) => `${exe.dir}/${f}`));
    }
    await copyFiles(`${p.dir()}/ext/asmx`, "ext/asmx", ["README.txt", "src"]);
    await copyFiles(`${p.dir()}/ext`, "ext", ["chips", "imgui_markdown", "perfect6502", "pystring", "texteditor"]);
    await copyFiles(`${p.dir()}/ext/sokol`, "ext/sokol", ["sokol.c"]);
    await copyFiles(p.importDir("sokol"), "ext/sokol", [
        "LICENSE",
        "sokol_app.h",
        "sokol_gfx.h",
        "sokol_time.h",
        "sokol_args.h",
        "sokol_log.h",
        "sokol_glue.h",
    ]);
    await copyFiles(`${p.importDir("sokol")}/util`, "ext/sokol", ["sokol_imgui.h", "sokol_gfx_imgui.h", "sokol_app_imgui.h"]);
    await copyFiles(p.importDir("dcimgui"), "ext/dcimgui", ["LICENSE"]);
    for await (const entry of Deno.readDir(`${p.importDir("dcimgui")}/src-docking`)) {
        await copyFiles(`${p.importDir("dcimgui")}/src-docking`, "ext/dcimgui", [entry.name]);
    }

    // compile shaders for all macOS and Linux backends into a single header (selected at runtime via sg_query_backend())
    const shdcRes = await util.runCmd(getShdcPath(p), {
        args: [
            "--input", `${srcDir}/gfx.glsl`,
            "--output", `${stageDir}/src/gfx.glsl.h`,
            "--slang", "metal_macos:glsl410:glsl300es",
            "--format", "sokol",
        ],
        cwd: p.dir(),
        winUseCmd: true,
    });
    if (shdcRes.exitCode !== 0) {
        throw new Error(`sokol-shdc failed with exit code ${shdcRes.exitCode}`);
    }

    const gitRes = await util.runCmd("git", {
        args: ["rev-parse", "--short", "HEAD"],
        cwd: p.dir(),
        stdout: "piped",
        showCmd: false,
    });
    const commit = gitRes.exitCode === 0 ? gitRes.stdout.trim() : "unknown";
    await Deno.writeTextFile(`${stageDir}/Makefile`, makefile(commonSources));
    await Deno.writeTextFile(`${stageDir}/README.md`, readme(commit));

    // zip the staging directory
    const zip = new ZipWriter(new BlobWriter("application/zip"));
    let numFiles = 0;
    for await (const entry of walk(stageDir, { includeDirs: false })) {
        const path = relative(workDir, entry.path).replaceAll("\\", "/");
        await zip.add(path, new Uint8ArrayReader(await Deno.readFile(entry.path)));
        numFiles++;
    }
    const blob = await zip.close();
    await Deno.writeFile(zipPath, new Uint8Array(await blob.arrayBuffer()));
    log.info(`# wrote ${zipPath} (${numFiles} files)`);
    if (args[1] !== undefined) {
        ensureDirSync(args[1]);
        await copy(zipPath, `${args[1]}/v6502r-src.zip`, { overwrite: true });
        log.info(`# copied to ${args[1]}/v6502r-src.zip`);
    }
}

function getShdcPath(p: Project): string {
    let dir;
    if (p.isHostWindows()) {
        dir = "win32";
    } else if (p.isHostMacOS()) {
        dir = (p.hostArch() === "arm64") ? "osx_arm64" : "osx";
    } else {
        dir = (p.hostArch() === "arm64") ? "linux_arm64" : "linux";
    }
    return `${p.importDir("sokol-shdc")}/bin/${dir}/sokol-shdc`;
}

function makefile(commonSources: string[]): string {
    const list = (items: string[]) => items.join(" \\\n    ");
    const compiled = (items: string[]) => items.filter((f) => /\.(c|cc|cpp)$/.test(f));
    const libSources = [
        "ext/sokol/sokol.c",
        ...["cimgui.cpp", "cimgui_internal.cpp", "imgui_demo.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp", "imgui.cpp"].map((f) => `ext/dcimgui/${f}`),
        "ext/texteditor/TextEditor.cpp",
        "ext/pystring/pystring.cpp",
    ];
    const exeTargets = distroExes.map((exe) => {
        const sources = ["ext/asmx/src/asmx.c", "ext/perfect6502/netlist_sim.c", ...exe.sources];
        return `$(eval $(call EXE_template,${exe.name},${exe.dir},${exe.defines},${sources.join(" ")}))`;
    });
    const exeNames = distroExes.map((exe) => exe.name).join(" ");
    return `# Makefile for the v6502r source distro (macOS and Linux)
#
# machine generated by 'fibs build-source-distro', don't edit!
#
#   make            build ${exeNames} into $(BUILD_DIR)/bin
#   make v6502r     build a single executable
#   make clean      delete $(BUILD_DIR)
#
# On Linux, sokol-gfx uses GLES3 via EGL on ARM CPUs (e.g. Raspberry Pi 4/5),
# and desktop GL on all other CPUs. Override with 'make GLES3=1' or 'make GLES3=0'.

BUILD_DIR ?= build
OPT ?= -O2 -DNDEBUG

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Darwin)
    BACKEND := -DSOKOL_METAL
    LIBS := -framework Metal -framework QuartzCore -framework AppKit
    # sokol.c contains the sokol_app.h implementation which must be compiled as Objective-C
    SOKOL_LANG := -x objective-c
else ifeq ($(UNAME_S),Linux)
    ifneq ($(filter arm% aarch64,$(UNAME_M)),)
        GLES3 ?= 1
    else
        GLES3 ?= 0
    endif
    ifeq ($(GLES3),1)
        BACKEND := -DSOKOL_GLES3
        LIBS := -lGLESv2 -lEGL
    else
        BACKEND := -DSOKOL_GLCORE
        LIBS := -lGL
    endif
    LIBS += -lX11 -lXi -lXcursor -ldl -lm -lpthread
else
    $(error unsupported platform '$(UNAME_S)', only macOS and Linux are supported)
endif

INCLUDES := -Iext -Iext/sokol -Iext/dcimgui -Iext/texteditor -Iext/pystring -Iext/asmx/src -Iext/perfect6502
ALL_CPPFLAGS = $(INCLUDES) $(BACKEND) -MMD -MP $(CPPFLAGS)
ALL_CFLAGS = -std=gnu11 $(OPT) $(CFLAGS)
ALL_CXXFLAGS = -std=gnu++17 $(OPT) $(CXXFLAGS)

# sources shared by all executables
LIB_SRCS := ${list(libSources)}
LIB_OBJS := $(addprefix $(BUILD_DIR)/obj/lib/,$(addsuffix .o,$(LIB_SRCS)))
DEPS := $(LIB_OBJS:.o=.d)

# sources compiled separately for each executable with chip-specific defines
COMMON_SRCS := ${list(compiled(commonSources).map((f) => `src/${f}`))}

.PHONY: all clean ${exeNames}
all: ${exeNames}

clean:
\trm -rf $(BUILD_DIR)

$(BUILD_DIR)/obj/lib/ext/sokol/sokol.c.o: SRC_LANG := $(SOKOL_LANG)

$(BUILD_DIR)/obj/lib/%.c.o: %.c
\t@mkdir -p $(@D)
\t$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(SRC_LANG) -c $< -o $@

$(BUILD_DIR)/obj/lib/%.cpp.o: %.cpp
\t@mkdir -p $(@D)
\t$(CXX) $(ALL_CPPFLAGS) $(ALL_CXXFLAGS) -c $< -o $@

# $(1): executable name, $(2): chip source dir, $(3): chip defines, $(4): chip-specific sources
define EXE_template
$(1)_OBJS := $$(addprefix $$(BUILD_DIR)/obj/$(1)/,$$(addsuffix .o,$$(COMMON_SRCS) src/$(2)/segdefs.c src/$(2)/nodenames.c $(4)))
DEPS += $$($(1)_OBJS:.o=.d)

$(1): $$(BUILD_DIR)/bin/$(1)

$$(BUILD_DIR)/bin/$(1): $$($(1)_OBJS) $$(LIB_OBJS)
\t@mkdir -p $$(@D)
\t$$(CXX) -o $$@ $$^ $$(LDFLAGS) $$(LIBS)

$$(BUILD_DIR)/obj/$(1)/%.c.o: %.c
\t@mkdir -p $$(@D)
\t$$(CC) $$(ALL_CPPFLAGS) -Isrc/$(2) $(3) $$(ALL_CFLAGS) -c $$< -o $$@

$$(BUILD_DIR)/obj/$(1)/%.cc.o: %.cc
\t@mkdir -p $$(@D)
\t$$(CXX) $$(ALL_CPPFLAGS) -Isrc/$(2) $(3) $$(ALL_CXXFLAGS) -c $$< -o $$@
endef

${exeTargets.join("\n")}

-include $(DEPS)
`;
}

function readme(commit: string): string {
    return `# v6502r source distro

Visual6502, VisualZ80 and Visual2A03 Remixed, packaged with all dependencies
for building with a plain Makefile on macOS and Linux (including Raspberry Pi 4/5).

Created from https://github.com/floooh/v6502r at commit ${commit}.

## Build

\`\`\`
make -j8
\`\`\`

This builds the executables \`build/bin/v6502r\`, \`build/bin/vz80r\` and \`build/bin/v2a03r\`.

On macOS the Xcode command line tools are required (\`xcode-select --install\`).

On Linux a C/C++ toolchain and the X11 and OpenGL development packages are required, e.g.
on Debian, Ubuntu or Raspberry Pi OS:

\`\`\`
sudo apt install build-essential libx11-dev libxi-dev libxcursor-dev libgl-dev libgles-dev libegl-dev
\`\`\`

On ARM Linux (e.g. Raspberry Pi 4 and 5) rendering goes through GLES3 and EGL,
on all other Linux machines through desktop GL 4.3. To override this:

\`\`\`
make GLES3=1    # force GLES3 via EGL
make GLES3=0    # force desktop GL
\`\`\`

Raspberry Pi 3 and older are not supported (no GLES3). On Wayland desktops
the X11 compatibility layer (XWayland) is used.

Other useful make variables: \`CC\`, \`CXX\`, \`OPT\` (default: \`-O2 -DNDEBUG\`),
\`CFLAGS\`, \`CXXFLAGS\`, \`LDFLAGS\` and \`BUILD_DIR\` (default: \`build\`).

## Licenses

See the license files in the \`ext/\` subdirectories, and the original project
at https://github.com/floooh/v6502r for a list of all used software.
`;
}
