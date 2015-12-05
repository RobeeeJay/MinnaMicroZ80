; Multiface RAM mapped to 8192
; IN A,(31)               ; Switch out Multiface ROM/RAM
; IN A,(159)			  ; Switch in Multiface ROM/RAM
; could also be 191 or 31 according to genie docs


	ORG 27000
MAIN
INIT
	DI
	IM 1	; ED5E
SET48K
	LD A,0x30	; Page in 48k ROM and disable further paging
	LD BC,0x77FD
	OUT (C),A
LOADREG
	LD SP,REGDATASTACK1
	POP BC		; BC'
	POP DE		; DE'
	POP HL		; HL'
	EXX
	POP IX		; IX
	POP IY		; IY
	POP AF		; IR
	LD I,A
DECOMPSCREEN
	LD HL,SCREENDATA
	LD DE,16384
	CALL DEC40
SHIFTDATA
	LD HL,SCREENDATA + 0x1010
	LD DE,65535
	LD BC,0x1010	; Set to data length
	LDDR
SHIFTDEPACK
	LD HL,NEWDEPACK
	LD DE,16384
	LD BC,END_DEC40 - NEWDEPACK
	LDIR
	LD SP,16384 + (REGDATASTACK2 - NEWDEPACK)
CALLDEPACK
	LD HL,65536 - 0x1010	; Set to new start of data
	LD DE,16384 + 6912	; Destination
JUMPTONEWDEPACK
	JP 16384
; We should make this start of screen shifted depack
NEWDEPACK
	CALL 16384 + (DEC40 - NEWDEPACK)
	; fill last 10 bytes
	LD HL,16384 + (LASTBYTES - NEWDEPACK)
	LD DE,65536 - 10
	LD BC,10
	LDIR
SETLASTREGS
	POP DE		; DE
	POP HL		; HL
	POP AF		; AF'
	EX AF,AF'	;'
	POP BC		; R and Border
	LD A,C
	LD C,0xFE
	OUT (C),A	; set Border
	LD A,B
	LD R,A		; set R
	POP BC		; BC
	POP AF		; AF
	LD SP,0xFFFF	; SP
	EI		; NOP out if DI
RUNPROG
	JP 0xFFFF	; PC
	DEFW 0x0000	; for depacker usage
	DEFW 0x0000	; for DE call
REGDATASTACK2
	DEFW 0x0606	; DE
	DEFW 0x0707	; HL
	DEFW 0x0808	; AF'
	DEFB 0x09	; R
	DEFB 0x01	; Border Colour
	DEFW 0x1010	; BC
	DEFW 0x1111	; AF
LASTBYTES
	DEFB "0123456789"

; Register usage: AF,AF',BC,DE,HL. Must be CALL'ed, return is done by RET.
; Provide extra stack location for store 2 bytes (1 word). Depacker does not
; - put starting address of packed block in HL,
; - put location where you want data to be depacked in DE,
;   (much like LDIR command, but without BC)
; - make CALL to depacker (DEC40).

DEC40
        LD      A,0x80
        EX      AF,AF'	;'
MS      LDI
M0      LD      BC,0x02FF
M1      EX      AF,AF'	;'
M1X     ADD     A,A
        JR      NZ,M2
        LD      A,(HL)
        INC     HL
        RLA
M2      RL      C
        JR      NC,M1X
        EX      AF,AF'	;'
        DJNZ    X2
        LD      A,2
        SRA     C
        JR      C,N1
        INC     A
        INC     C
        JR      Z,N2
        LD      BC,0x033F
        JR      M1

X2      DJNZ    X3
        SRL     C
        JR      C,MS
        INC     B
        JR      M1
X6
        ADD     A,C
N2
        LD      BC,0x04FF
        JR      M1
N1
        INC     C
        JR      NZ,M4
        EX      AF,AF'	;'
        INC     B
N5      RR      C
        RET     C
        RL      B
        ADD     A,A
        JR      NZ,N6
        LD      A,(HL)
        INC     HL
        RLA
N6      JR      NC,N5
        EX      AF,AF'	;'
        ADD     A,B
        LD      B,6
        JR      M1
X3
        DJNZ    X4
        LD      A,1
        JR      M3
X4      DJNZ    X5
        INC     C
        JR      NZ,M4
        LD      BC,0x051F
        JR      M1
X5
        DJNZ    X6
        LD      B,C
M4      LD      C,(HL)
        INC     HL
M3      DEC     B
        PUSH    HL
        LD      L,C
        LD      H,B
        ADD     HL,DE
        LD      C,A
        LD      B,0
        LDIR
        POP     HL
        JR      M0
END_DEC40
REGDATASTACK1
	DEFW 0x0101	; BC'
	DEFW 0x0202	; DE'
	DEFW 0x0303	; HL'
	DEFW 0x0404	; IX
	DEFW 0x0505	; IY
	DEFW 0xFFFF	; IR
SCREENDATA

