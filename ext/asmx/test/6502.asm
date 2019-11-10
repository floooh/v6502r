        BRK
        ORA ($FF,X)
        DB  $02
        DB  $03
        DB  $04
        ORA $FF
        ASL $FF
        DB  $07

        PHP
        ORA #$FF
        ASL A
        DB  $0B
        DB  $0C
        ORA $FFFF
        ASL $FFFF
        DB  $0F

        BPL *
        ORA ($FF),Y
        DB  $12
        DB  $13
        DB  $14
        ORA $FF,X
        ASL $FF,X
        DB  $17

        CLC
        ORA $FFFF,Y
        DB  $1A
        DB  $1B
        DB  $1C
        ORA $FFFF,X
        ASL $FFFF,X
        DB  $1F

        JSR $FFFF
        AND ($FF,X)
        DB  $22
        DB  $23
        BIT $FF
        AND $FF
        ROL $FF
        DB  $27

        PLP
        AND #$FF
        ROL A
        DB  $2B
        BIT $FFFF
        AND $FFFF
        ROL $FFFF
        DB  $2F

        BMI *
        AND ($FF),Y
        DB  $32
        DB  $33
        DB  $34
        AND $FF,X
        ROL $FF,X
        DB  $37

        SEC
        AND $FFFF,Y
        DB  $3A
        DB  $3B
        DB  $3C
        AND $FFFF,X
        ROL $FFFF,X
        DB  $3F

        RTI
        EOR ($FF,X)
        DB  $42
        DB  $43
        DB  $44
        EOR $FF
        LSR $FF
        DB  $47

        PHA
        EOR #$FF
        LSR A
        DB  $4B
        JMP $FFFF
        EOR $FFFF
        LSR $FFFF
        DB  $4F

        BVC *
        EOR ($FF),Y
        DB  $52
        DB  $53
        DB  $54
        EOR $FF,X
        LSR $FF,X
        DB  $57

        CLI
        EOR $FFFF,Y
        DB  $5A
        DB  $5B
        DB  $5C
        EOR $FFFF,X
        LSR $FFFF,X
        DB  $5F

        RTS
        ADC ($FF,X)
        DB  $62
        DB  $63
        DB  $64
        ADC $FF
        ROR $FF
        DB  $67

        PLA
        ADC #$FF
        ROR A
        DB  $6B
        JMP ($FFFF)
        ADC $FFFF
        ROR $FFFF
        DB  $6F

        BVS *
        ADC ($FF),Y
        DB  $72
        DB  $73
        DB  $74
        ADC $FF,X
        ROR $FF,X
        DB  $77

        SEI
        ADC $FFFF,Y
        DB  $7A
        DB  $7B
        DB  $7C
        ADC $FFFF,X
        ROR $FFFF,X
        DB  $7F

        DB  $80
        STA ($FF,X)
        DB  $82
        DB  $83
        STY $FF
        STA $FF
        STX $FF
        DB  $87

        DEY
        DB  $89
        TXA
        DB  $8B
        STY $FFFF
        STA $FFFF
        STX $FFFF
        DB  $8F

        BCC *
        STA ($FF),Y
        DB  $92
        DB  $93
        STY $FF,X
        STA $FF,X
        STX $FF,Y
        DB  $97

        TYA
        STA $FFFF,Y
        TXS
        DB  $9B
        DB  $9C
        STA $FFFF,X
        DB  $9E
        DB  $9F

        LDY #$FF
        LDA ($FF,X)
        LDX #$FF
        DB  $A3
        LDY $FF
        LDA $FF
        LDX $FF
        DB  $A7

        TAY
        LDA #$FF
        TAX
        DB  $AB
        LDY $FFFF
        LDA $FFFF
        LDX $FFFF
        DB  $AF

        BCS *
        LDA ($FF),Y
        DB  $B2
        DB  $B3
        LDY $FF,X
        LDA $FF,X
        LDX $FF,Y
        DB  $B7

        CLV
        LDA $FFFF,Y
        TSX
        DB  $BB
        LDY $FFFF,X
        LDA $FFFF,X
        LDX $FFFF,Y
        DB  $BF

        CPY #$FF
        CMP ($FF,X)
        DB  $C2
        DB  $C3
        CPY $FF
        CMP $FF
        DEC $FF
        DB  $C7

        INY
        CMP #$FF
        DEX
        DB  $CB
        CPY $FFFF
        CMP $FFFF
        DEC $FFFF
        DB  $CF

        BNE *
        CMP ($FF),Y
        DB  $D2
        DB  $D3
        DB  $D4
        CMP $FF,X
        DEC $FF,X
        DB  $D7

        CLD
        CMP $FFFF,Y
        DB  $DA
        DB  $DB
        DB  $DC
        CMP $FFFF,X
        DEC $FFFF,X
        DB  $DF

        CPX #$FF
        SBC ($FF,X)
        DB  $E2
        DB  $E3
        CPX $FF
        SBC $FF
        INC $FF
        DB  $E7

        INX
        SBC #$FF
        NOP
        DB  $EB
        CPX $FFFF
        SBC $FFFF
        INC $FFFF
        DB  $EF

        BEQ *
        SBC ($FF),Y
        DB  $F2
        DB  $F3
        DB  $F4
        SBC $FF,X
        INC $FF,X
        DB  $F7

        SED
        SBC $FFFF,Y
        DB  $FA
        DB  $FB
        DB  $FC
        SBC $FFFF,X
        INC $FFFF,X
        DB  $FF

        END
