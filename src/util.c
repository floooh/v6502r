//------------------------------------------------------------------------------
//  util.c
//------------------------------------------------------------------------------
#include "v6502r.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

// FIXME
#include <stdio.h>

#if defined(__EMSCRIPTEN__)
EM_JS(void, emsc_js_init, (void), {
    Module['ccall'] = ccall;
});


EM_JS(void, emsc_js_download_string, (const char* c_filename, const char* c_content), {
    var filename = UTF8ToString(c_filename);
    var content = UTF8ToString(c_content);
    var elm = document.createElement('a');
    elm.setAttribute('href', 'data:text/plain;charset=utf-8,'+encodeURIComponent(content));
    elm.setAttribute('download', filename);
    elm.style.display='none';
    document.body.appendChild(elm);
    elm.click();
    document.body.removeChild(elm);
});

EM_JS(void, emsc_js_download_binary, (const char* c_filename, const uint8_t* ptr, int num_bytes), {
    var filename = UTF8ToString(c_filename);
    var binary = "";
    for (var i = 0; i < num_bytes; i++) {
        binary += String.fromCharCode(HEAPU8[ptr+i]);
    }
    console.log(btoa(binary));
    var elm = document.createElement('a');
    elm.setAttribute('href', 'data:application/octet-stream;base64,'+btoa(binary));
    elm.setAttribute('download', filename);
    elm.style.display='none';
    document.body.appendChild(elm);
    elm.click();
    document.body.removeChild(elm);
});

EM_JS(void, emsc_js_load, (void), {
    document.getElementById('picker').click();
});

EM_JS(int, emsc_js_is_mac, (void), {
    if (navigator.userAgent.includes('Macintosh')) {
        return 1;
    }
    else {
        return 0;
    }
});

EMSCRIPTEN_KEEPALIVE int util_emsc_loadfile(const char* name, const uint8_t* data, int size) {
    ui_asm_put_source(name, data, size);
    app.ui.asm_open = true;
    ui_asm_assemble();
    return 1;
}
#endif

void util_init(void) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_init();
    #endif
}

void util_html5_download_string(const char* filename, const char* content) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_download_string(filename, content);
    #endif
}

void util_html5_download_binary(const char* filename, const uint8_t* content, uint32_t num_bytes) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_download_binary(filename, content, num_bytes);
    #endif
}

void util_html5_load(void) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_load();
    #endif
}

bool util_is_mac(void) {
    #if defined(__EMSCRIPTEN__)
    return 0 != emsc_js_is_mac();
    #elif defined(__APPLE__)
    return true;
    #else
    return false;
    #endif
}

const char* util_opcode_to_str(uint8_t opcode) {
    switch (opcode) {
        case 0x0:  return "BRK        ";
        case 0x1:  return "ORA (zp,X) ";
        case 0x2:  return "*INV       ";
        case 0x3:  return "*SLO (zp,X)";
        case 0x4:  return "*NOP zp    ";
        case 0x5:  return "ORA zp     ";
        case 0x6:  return "ASL zp     ";
        case 0x7:  return "*SLO zp    ";
        case 0x8:  return "PHP        ";
        case 0x9:  return "ORA #      ";
        case 0xa:  return "ASLA       ";
        case 0xb:  return "*ANC #     ";
        case 0xc:  return "*NOP abs   ";
        case 0xd:  return "ORA abs    ";
        case 0xe:  return "ASL abs    ";
        case 0xf:  return "*SLO abs   ";
        case 0x10: return "BPL #      ";
        case 0x11: return "ORA (zp),Y ";
        case 0x12: return "*INV       ";
        case 0x13: return "*SLO (zp),Y";
        case 0x14: return "*NOP zp,X  ";
        case 0x15: return "ORA zp,X   ";
        case 0x16: return "ASL zp,X   ";
        case 0x17: return "*SLO zp,X  ";
        case 0x18: return "CLC        ";
        case 0x19: return "ORA abs,Y  ";
        case 0x1a: return "*NOP       ";
        case 0x1b: return "*SLO abs,Y ";
        case 0x1c: return "*NOP abs,X ";
        case 0x1d: return "ORA abs,X  ";
        case 0x1e: return "ASL abs,X  ";
        case 0x1f: return "*SLO abs,X ";
        case 0x20: return "JSR        ";
        case 0x21: return "AND (zp,X) ";
        case 0x22: return "*INV       ";
        case 0x23: return "*RLA (zp,X)";
        case 0x24: return "BIT zp     ";
        case 0x25: return "AND zp     ";
        case 0x26: return "ROL zp     ";
        case 0x27: return "*RLA zp    ";
        case 0x28: return "PLP        ";
        case 0x29: return "AND #      ";
        case 0x2a: return "ROLA       ";
        case 0x2b: return "*ANC #     ";
        case 0x2c: return "BIT abs    ";
        case 0x2d: return "AND abs    ";
        case 0x2e: return "ROL abs    ";
        case 0x2f: return "*RLA abs   ";
        case 0x30: return "BMI #      ";
        case 0x31: return "AND (zp),Y ";
        case 0x32: return "*INV       ";
        case 0x33: return "*RLA (zp),Y";
        case 0x34: return "*NOP zp,X  ";
        case 0x35: return "AND zp,X   ";
        case 0x36: return "ROL zp,X   ";
        case 0x37: return "*RLA zp,X  ";
        case 0x38: return "SEC        ";
        case 0x39: return "AND abs,Y  ";
        case 0x3a: return "*NOP       ";
        case 0x3b: return "*RLA abs,Y ";
        case 0x3c: return "*NOP abs,X ";
        case 0x3d: return "AND abs,X  ";
        case 0x3e: return "ROL abs,X  ";
        case 0x3f: return "*RLA abs,X ";
        case 0x40: return "RTI        ";
        case 0x41: return "EOR (zp,X) ";
        case 0x42: return "*INV       ";
        case 0x43: return "*SRE (zp,X)";
        case 0x44: return "*NOP zp    ";
        case 0x45: return "EOR zp     ";
        case 0x46: return "LSR zp     ";
        case 0x47: return "*SRE zp    ";
        case 0x48: return "PHA        ";
        case 0x49: return "EOR #      ";
        case 0x4a: return "LSRA       ";
        case 0x4b: return "*ASR #     ";
        case 0x4c: return "JMP        ";
        case 0x4d: return "EOR abs    ";
        case 0x4e: return "LSR abs    ";
        case 0x4f: return "*SRE abs   ";
        case 0x50: return "BVC #      ";
        case 0x51: return "EOR (zp),Y ";
        case 0x52: return "*INV       ";
        case 0x53: return "*SRE (zp),Y";
        case 0x54: return "*NOP zp,X  ";
        case 0x55: return "EOR zp,X   ";
        case 0x56: return "LSR zp,X   ";
        case 0x57: return "*SRE zp,X  ";
        case 0x58: return "CLI        ";
        case 0x59: return "EOR abs,Y  ";
        case 0x5a: return "*NOP       ";
        case 0x5b: return "*SRE abs,Y ";
        case 0x5c: return "*NOP abs,X ";
        case 0x5d: return "EOR abs,X  ";
        case 0x5e: return "LSR abs,X  ";
        case 0x5f: return "*SRE abs,X ";
        case 0x60: return "RTS        ";
        case 0x61: return "ADC (zp,X) ";
        case 0x62: return "*INV       ";
        case 0x63: return "*RRA (zp,X)";
        case 0x64: return "*NOP zp    ";
        case 0x65: return "ADC zp     ";
        case 0x66: return "ROR zp     ";
        case 0x67: return "*RRA zp    ";
        case 0x68: return "PLA        ";
        case 0x69: return "ADC #      ";
        case 0x6a: return "RORA       ";
        case 0x6b: return "*ARR #     ";
        case 0x6c: return "JMPI       ";
        case 0x6d: return "ADC abs    ";
        case 0x6e: return "ROR abs    ";
        case 0x6f: return "*RRA abs   ";
        case 0x70: return "BVS #      ";
        case 0x71: return "ADC (zp),Y ";
        case 0x72: return "*INV       ";
        case 0x73: return "*RRA (zp),Y";
        case 0x74: return "*NOP zp,X  ";
        case 0x75: return "ADC zp,X   ";
        case 0x76: return "ROR zp,X   ";
        case 0x77: return "*RRA zp,X  ";
        case 0x78: return "SEI        ";
        case 0x79: return "ADC abs,Y  ";
        case 0x7a: return "*NOP       ";
        case 0x7b: return "*RRA abs,Y ";
        case 0x7c: return "*NOP abs,X ";
        case 0x7d: return "ADC abs,X  ";
        case 0x7e: return "ROR abs,X  ";
        case 0x7f: return "*RRA abs,X ";
        case 0x80: return "*NOP #     ";
        case 0x81: return "STA (zp,X) ";
        case 0x82: return "*NOP #     ";
        case 0x83: return "*SAX (zp,X)";
        case 0x84: return "STY zp     ";
        case 0x85: return "STA zp     ";
        case 0x86: return "STX zp     ";
        case 0x87: return "*SAX zp    ";
        case 0x88: return "DEY        ";
        case 0x89: return "*NOP #     ";
        case 0x8a: return "TXA        ";
        case 0x8b: return "*ANE #     ";
        case 0x8c: return "STY abs    ";
        case 0x8d: return "STA abs    ";
        case 0x8e: return "STX abs    ";
        case 0x8f: return "*SAX abs   ";
        case 0x90: return "BCC        ";
        case 0x91: return "STA (zp),Y ";
        case 0x92: return "*INV       ";
        case 0x93: return "*SHA (zp),Y";
        case 0x94: return "STY zp,X   ";
        case 0x95: return "STA zp,X   ";
        case 0x96: return "STX zp,Y   ";
        case 0x97: return "*SAX zp,Y  ";
        case 0x98: return "TYA        ";
        case 0x99: return "STA abs,Y  ";
        case 0x9a: return "TXS        ";
        case 0x9b: return "*SHS abs,Y ";
        case 0x9c: return "*SHY abs,X ";
        case 0x9d: return "STA abs,X  ";
        case 0x9e: return "*SHX abs,Y ";
        case 0x9f: return "*SHA abs,Y ";
        case 0xa0: return "LDY #      ";
        case 0xa1: return "LDA (zp,X) ";
        case 0xa2: return "LDX #      ";
        case 0xa3: return "*LAX (zp,X)";
        case 0xa4: return "LDY zp     ";
        case 0xa5: return "LDA zp     ";
        case 0xa6: return "LDX zp     ";
        case 0xa7: return "*LAX zp    ";
        case 0xa8: return "TAY        ";
        case 0xa9: return "LDA #      ";
        case 0xaa: return "TAX        ";
        case 0xab: return "*LXA #     ";
        case 0xac: return "LDY abs    ";
        case 0xad: return "LDA abs    ";
        case 0xae: return "LDX abs    ";
        case 0xaf: return "*LAX abs   ";
        case 0xb0: return "BCS #      ";
        case 0xb1: return "LDA (zp),Y ";
        case 0xb2: return "*INV       ";
        case 0xb3: return "*LAX (zp),Y";
        case 0xb4: return "LDY zp,X   ";
        case 0xb5: return "LDA zp,X   ";
        case 0xb6: return "LDX zp,Y   ";
        case 0xb7: return "*LAX zp,Y  ";
        case 0xb8: return "CLV        ";
        case 0xb9: return "LDA abs,Y  ";
        case 0xba: return "TSX        ";
        case 0xbb: return "*LAS abs,Y ";
        case 0xbc: return "LDY abs,X  ";
        case 0xbd: return "LDA abs,X  ";
        case 0xbe: return "LDX abs,Y  ";
        case 0xbf: return "*LAX abs,Y ";
        case 0xc0: return "CPY #      ";
        case 0xc1: return "CMP (zp,X) ";
        case 0xc2: return "*NOP #     ";
        case 0xc3: return "*DCP (zp,X)";
        case 0xc4: return "CPY zp     ";
        case 0xc5: return "CMP zp     ";
        case 0xc6: return "DEC zp     ";
        case 0xc7: return "*DCP zp    ";
        case 0xc8: return "INY        ";
        case 0xc9: return "CMP #      ";
        case 0xca: return "DEX        ";
        case 0xcb: return "*SBX #     ";
        case 0xcc: return "CPY abs    ";
        case 0xcd: return "CMP abs    ";
        case 0xce: return "DEC abs    ";
        case 0xcf: return "*DCP abs   ";
        case 0xd0: return "BNE #      ";
        case 0xd1: return "CMP (zp),Y ";
        case 0xd2: return "*INV       ";
        case 0xd3: return "*DCP (zp),Y";
        case 0xd4: return "*NOP zp,X  ";
        case 0xd5: return "CMP zp,X   ";
        case 0xd6: return "DEC zp,X   ";
        case 0xd7: return "*DCP zp,X  ";
        case 0xd8: return "CLD        ";
        case 0xd9: return "CMP abs,Y  ";
        case 0xda: return "*NOP       ";
        case 0xdb: return "*DCP abs,Y ";
        case 0xdc: return "*NOP abs,X ";
        case 0xdd: return "CMP abs,X  ";
        case 0xde: return "DEC abs,X  ";
        case 0xdf: return "*DCP abs,X ";
        case 0xe0: return "CPX #      ";
        case 0xe1: return "SBC (zp,X) ";
        case 0xe2: return "*NOP #     ";
        case 0xe3: return "*ISB (zp,X)";
        case 0xe4: return "CPX zp     ";
        case 0xe5: return "SBC zp     ";
        case 0xe6: return "INC zp     ";
        case 0xe7: return "*ISB zp    ";
        case 0xe8: return "INX        ";
        case 0xe9: return "SBC #      ";
        case 0xea: return "NOP        ";
        case 0xeb: return "*SBC #     ";
        case 0xec: return "CPX abs    ";
        case 0xed: return "SBC abs    ";
        case 0xee: return "INC abs    ";
        case 0xef: return "*ISB abs   ";
        case 0xf0: return "BEQ #      ";
        case 0xf1: return "SBC (zp),Y ";
        case 0xf2: return "*INV       ";
        case 0xf3: return "*ISB (zp),Y";
        case 0xf4: return "*NOP zp,X  ";
        case 0xf5: return "SBC zp,X   ";
        case 0xf6: return "INC zp,X   ";
        case 0xf7: return "*ISB zp,X  ";
        case 0xf8: return "SED        ";
        case 0xf9: return "SBC abs,Y  ";
        case 0xfa: return "*NOP       ";
        case 0xfb: return "*ISB abs,Y ";
        case 0xfc: return "*NOP abs,X ";
        case 0xfd: return "*SBC abs,X ";
        case 0xfe: return "INC abs,X  ";
        case 0xff: return "*ISB abs,X ";
        default: return "???";
    }
}
