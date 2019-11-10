	PROCESSOR 6502U

        BRK
        ORA ($FF,X)
        DB  $02
        SLO ($FF,X)
        NOP3
        ORA $FF
        ASL $FF
        SLO $FF

        PHP
        ORA #$FF
        ASL A
        DB  $0B
        DB  $0C
        ORA $FFFF
        ASL $FFFF
        SLO $FFFF

        BPL *
        ORA ($FF),Y
        DB  $12
        SLO ($FF),Y
        DB  $14
        ORA $FF,X
        ASL $FF,X
        SLO $FF,X

        CLC
        ORA $FFFF,Y
        DB  $1A
        SLO $FFFF,Y
        DB  $1C
        ORA $FFFF,X
        ASL $FFFF,X
        SLO $FFFF,X

        JSR $FFFF
        AND ($FF,X)
        DB  $22
        RLA ($FF,X)
        BIT $FF
        AND $FF
        ROL $FF
        RLA $FF

        PLP
        AND #$FF
        ROL A
        ANC #$FF
        BIT $FFFF
        AND $FFFF
        ROL $FFFF
        RLA $FFFF

        BMI *
        AND ($FF),Y
        DB  $32
        RLA ($FF),Y
        DB  $34
        AND $FF,X
        ROL $FF,X
        RLA $FF,X

        SEC
        AND $FFFF,Y
        DB  $3A
        RLA $FFFF,Y
        DB  $3C
        AND $FFFF,X
        ROL $FFFF,X
        RLA $FFFF,X

        RTI
        EOR ($FF,X)
        DB  $42
        SRE ($FF,X)
        DB  $44
        EOR $FF
        LSR $FF
        SRE $FF

        PHA
        EOR #$FF
        LSR A
        ASR #$FF
        JMP $FFFF
        EOR $FFFF
        LSR $FFFF
        SRE $FFFF

        BVC *
        EOR ($FF),Y
        DB  $52
        SRE ($FF),Y
        DB  $54
        EOR $FF,X
        LSR $FF,X
        SRE $FF,X

        CLI
        EOR $FFFF,Y
        DB  $5A
        SRE $FFFF,Y
        DB  $5C
        EOR $FFFF,X
        LSR $FFFF,X
        SRE $FFFF,X

        RTS
        ADC ($FF,X)
        DB  $62
        RRA ($FF,X)
        DB  $64
        ADC $FF
        ROR $FF
        RRA $FF

        PLA
        ADC #$FF
        ROR A
        ARR #$FF
        JMP ($FFFF)
        ADC $FFFF
        ROR $FFFF
        RRA $FFFF

        BVS *
        ADC ($FF),Y
        DB  $72
        RRA ($FF),Y
        DB  $74
        ADC $FF,X
        ROR $FF,X
        RRA $FF,X

        SEI
        ADC $FFFF,Y
        DB  $7A
        RRA $FFFF,Y
        DB  $7C
        ADC $FFFF,X
        ROR $FFFF,X
        RRA $FFFF,X

        DB  $80
        STA ($FF,X)
        DB  $82
        SAX ($FF,X)
        STY $FF
        STA $FF
        STX $FF
        SAX $FF

        DEY
        DB  $89
        TXA
        DB  $8B
        STY $FFFF
        STA $FFFF
        STX $FFFF
        SAX $FFFF

        BCC *
        STA ($FF),Y
        DB  $92
        DB  $93
        STY $FF,X
        STA $FF,X
        STX $FF,Y
        SAX $FF,Y

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
        LAX ($FF,X)
        LDY $FF
        LDA $FF
        LDX $FF
        LAX $FF

        TAY
        LDA #$FF
        TAX
        DB  $AB
        LDY $FFFF
        LDA $FFFF
        LDX $FFFF
        LAX $FFFF

        BCS *
        LDA ($FF),Y
        DB  $B2
        LAX ($FF),Y
        LDY $FF,X
        LDA $FF,X
        LDX $FF,Y
        LAX $FF,Y

        CLV
        LDA $FFFF,Y
        TSX
        DB  $BB
        LDY $FFFF,X
        LDA $FFFF,X
        LDX $FFFF,Y
        LAX $FFFF,Y

        CPY #$FF
        CMP ($FF,X)
        DB  $C2
        DCP ($FF,X)
        CPY $FF
        CMP $FF
        DEC $FF
        DCP $FF

        INY
        CMP #$FF
        DEX
        SBX #$FF
        CPY $FFFF
        CMP $FFFF
        DEC $FFFF
        DCP $FFFF

        BNE *
        CMP ($FF),Y
        DB  $D2
        DCP ($FF),Y
        DB  $D4
        CMP $FF,X
        DEC $FF,X
        DCP $FF,X

        CLD
        CMP $FFFF,Y
        DB  $DA
        DCP $FFFF,Y
        DB  $DC
        CMP $FFFF,X
        DEC $FFFF,X
        DCP $FFFF,X

        CPX #$FF
        SBC ($FF,X)
        DB  $E2
        ISB ($FF,X)
        CPX $FF
        SBC $FF
        INC $FF
        ISB $FF

        INX
        SBC #$FF
        NOP
        DB  $EB
        CPX $FFFF
        SBC $FFFF
        INC $FFFF
        ISB $FFFF

        BEQ *
        SBC ($FF),Y
        DB  $F2
        ISB ($FF),Y
        DB  $F4
        SBC $FF,X
        INC $FF,X
        ISB $FF,X

        SED
        SBC $FFFF,Y
        DB  $FA
        ISB $FFFF,Y
        DB  $FC
        SBC $FFFF,X
        INC $FFFF,X
        ISB $FFFF,X

        END
