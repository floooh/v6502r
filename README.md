# v6502r

Visual6502 Remixed (WIP)

WASM Version: https://floooh.github.io/visual6502remix/

TL;DR: Take the [visual6502](https://github.com/trebonian/visual6502) project
and turn it into a native+wasm app via
[perfect6502](https://github.com/mist64/perfect6502), [Dear
ImGui](https://github.com/ocornut/imgui/) and
[Sokol](https://github.com/floooh/sokol).

## To Build:

Make sure ```python``` and ```cmake``` is in the path.

To get an idea about additional required tools, first run

```
> ./fips diag tools
```

To build and run the native version, run:

```
> ./fips build
...
> ./fips run v6502r
```

Linux may require additional development packages for X11 and OpenGL development.

To build the WASM version:

```
> ./fips setup emscripten
> ./fips set config wasm-ninja-release
> ./fips build
> ./fips run v6502r
```

## Software used in this project:

Many thanks to:

- **visual6502**: https://github.com/trebonian/visual6502
- **perfect6502**: https://github.com/mist64/perfect6502
- **Dear ImGui**: https://github.com/ocornut/imgui/
- **ImGuiColorTextEdit**: https://github.com/BalazsJako/ImGuiColorTextEdit
- **ImGuiMarkdown**: https://github.com/juliettef/imgui_markdown 
- **asmx**: http://xi6.com/projects/asmx/
- **The Sokol Headers**: https://github.com/floooh/sokol
- **The Chips Headers (UI)**: https://github.com/floooh/chips
- **Tripy**: https://github.com/linuxlewis/tripy
- **IconFontCppHeaders**: https://github.com/juliettef/IconFontCppHeaders
- **Font Awesome 4**: https://github.com/FortAwesome/Font-Awesome/tree/fa-4

Please be aware of the various licenses in the respective
github repositories, subdirectories and files.





