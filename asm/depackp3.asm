;
; decompression code megaLZ courtesy of mayHem http://lvd.nm.ru/MegaLZ/
; machine code microdrive file loader code courtesy of jp@drumfu.com
;
ERR_SP	EQU $5C3D	; address of item on machine stack to be used as err return

LASTOUT EQU $5B5C	; sysvar for last out of $77fd
ATTR_P	EQU $5C8D	;
_BORDCR	EQU $5C48	;

HD_00	EQU $5CE6	; file type
HD_0B	EQU $5CE7	; length of data
HD_0D	EQU $5CE9	; start of data
HD_0F	EQU $5CEB	; program length
HD_11	EQU $5CED  	; line number
COPIES	EQU $5CEF	; number of copies made by save

DOSINIT EQU $0100
DOSOPEN EQU $0106	; open a file for read/write
DOSREAD EQU $0112	; read bytes from a file
DOSCLOSE EQU $0109	; close a file by file number

PROG	EQU $5C53
VARS	EQU $5C4B
ELINE	EQU $5C59

NEWSTART	EQU 25000


; load_decomp+screen+1+3	robocop
; br 0x9ba3

	ORG 27000
INIT
	DI
CLEARBASIC
	LD HL,(PROG)		; This bit apparently originates from Andrew Pennell's * MOVE utility
	LD (VARS),HL		; It's a quick way of clearing out BASIC but leaving machine in an other wise
	LD (HL),$80			; stable enough state to still call Microdrive ROM functions
	INC HL				; Thanks to Jim P. for this suggestion, it cured launching the code
	LD (ELINE),HL		; from a large BASIC program
	CALL $16B0			; SET-MIN
SETNEWSTACK
	LD SP,NEWSTART		; set stack to new ramptop
	LD (ERR_SP),SP		; make sure the stack gets set properly for a ret in case of error
	; note that we haven't actually set an error routine yet, we'll push this later
INITDOS
	CALL DOSINIT
RELOCATE	; relocate code as low as possible, this buys us an additional 2kish
	LD HL,MAIN
	LD DE,NEWSTART
	LD BC,SCREENDATA - MAIN
	LDIR
	LD SP,NEWSTART
CALLNEWMAIN
	JP NEWSTART
	
; Commenting out as it seems to screw up the compiler
	; ORG NEWSTART
MAIN
	LD HL,NEWSTART + (ERROR - MAIN)
	PUSH HL				; top of stack is now the error func
	; normally here the IF1 sys vars need to be added to the system area
	; but since this file should already in theory have been loaded from
	; a cartridge, we can assume it is setup
	;RST $08
	;DEFB $31

	; Now we call our loader routine via the IF1 so it's paged in properly
	;LD HL,NEWSTART + (LOADALLDATA - MAIN)
	;LD (HD_11),HL	; execute LOADER
	;RST $08			; by using the if1
	;DEFB $32		; hook code to call any shadow rom address

	;DI
	;IM 2			; set to correct IM
	CALL NEWSTART + (LOADALLDATA - MAIN)
	JP NEWSTART + (FINALDECOMP - MAIN)
	
ERROR
	LD A,$02			; red
	LD C,0xFE
	OUT (C),A			; set Border
	JR ERROR			; halt execution forcing a reset to continue
NOERROR
	LD A,$04			; green
	LD C,0xFE
	OUT (C),A			; set Border
	JR NOERROR			; halt execution forcing a reset to continue

FNLEN
	DEFW 0x0008
FILENAME
	DEFB "ROBOCOP"
	DEFB 0xFF
	
DOLOAD
	DI
	LD BC,0x77FD
	LD A,0x00
	RES 4,A
	OR %00000111
	LD (LASTOUT),A
	OUT (C),A		; select Bank 0, Bank 5 as screen, 128 ROM
;	LD BC,0x1FFD
;	LD A,0x04	; select +3DOS rom
;	OUT (C),A

	LD DE,0x0001	; error if not exists
	LD BC,0x0305	; file #3, access mode shared-read
	LD HL,NEWSTART + (FILENAME - MAIN)

	CALL DOSOPEN
	JP NC, NEWSTART + (ERROR - MAIN)

	LD BC,0x0303	; file #5, bank 0
	LD DE,1000	; just testing need to replace with real value via DOS REF HEAD
	LD HL,0x6978	; load to 27000
	CALL DOSREAD
	JP NC, NEWSTART + (ERROR - MAIN)
	
	LD B,0x50
	CALL DOSCLOSE

	RET
;	BIT 4,(IY+124)	; signify load operation
;	LD A,3
;	LD (HD_00),A	; signify a code file
;	LD HL,NEWSTART + (FILENAME - MAIN)
;	LD (T_STR1),HL	; address of filename
;	LD HL,(NEWSTART + (FNLEN - MAIN))
;	LD (N_STR1),HL	; length of filename

LOADFILE
;	CALL NEWSTART + (OPTEMPM - MAIN)	; create temporary channel
	; after the above call IX becomes the temp M channel area address
;	PUSH IX
;	POP HL
;	ADD HL,DE
;	LD DE,HD_00
;	LD BC,$0009
;	LDIR

;	LD L,(IX+$55)	; retrieve code start
;	LD H,(IX+$56)
;	LD (HD_0D),HL
;	LD E,(IX+$53)	; retrieve code length
;	LD D,(IX+$54)
;	INC DE
;	ADD HL,DE

;	CALL NEWSTART + (LVMCH - MAIN)	; dive into microdrive load/verify block
;	CALL NEWSTART + (CLOSEM2 - MAIN)	; close file

	RET

PAGEBANKZERO
	LD A,0x10
PAGEBANK
	LD BC,0x77FD
	LD (LASTOUT),A
	OUT (C),A
	RET
BANKMIDTOHIGH
	LD HL,32768
	LD DE,49152
	LD BC,16384
	LDIR
	RET
BANKHIGHTOMID
	LD HL,49152
	LD DE,32768
	LD BC,16384
	LDIR
	RET
UPSHIFT
	LD HL,65535 - 10;
	LD DE,65535
	LD BC,32768
	LDDR
	RET

LOADALLDATA
	; screen+1+3
	; CALL NEWSTART + (DOLOAD - MAIN)
; DEBUG LINE
	JP NEWSTART + (LOAD2 - MAIN)
;DEBUIG LINE
DECOMPSCREEN
	; decompress screen
	LD HL,27000 + (SCREENDATA - INIT) ; data start
	LD DE,16384
	CALL NEWSTART + (DEC40 - MAIN)
	; copy data to top of bank 0->2
	LD HL,27000 + (SCREENDATA - INIT) + 3794 + 18115 - 1 ; Set to end of data
	LD DE,65535
	LD BC,18115	; Set to data length
	LDDR
	; decompress data(1+3) to bank 2+0
	LD HL,65536 - 18115	; Set to new start of data
	LD DE,32768 - 10	; Destination
	CALL NEWSTART + (DEC40 - MAIN)
	; shift decompressed data to its proper position
	CALL NEWSTART + (UPSHIFT - MAIN)
	; page in bank 1
	LD A,0x01
	CALL NEWSTART + (PAGEBANK - MAIN)
	; copy bank 2(1) to bank 1
	CALL NEWSTART + (BANKMIDTOHIGH - MAIN)
	; page in bank 0
	CALL NEWSTART + (PAGEBANKZERO - MAIN)
	; copy bank 0(3) to bank 2
	CALL NEWSTART + (BANKHIGHTOMID - MAIN)
	; page in bank 3
	LD A,0x03
	CALL NEWSTART + (PAGEBANK - MAIN)
	; copy bank 2(3) to bank 3
	CALL NEWSTART + (BANKMIDTOHIGH - MAIN)
	; page in bank 0
	CALL NEWSTART + (PAGEBANKZERO - MAIN)
	; load next group
LOAD2
	LD HL,NEWSTART + (FILENAME - MAIN)
	LD (HL), "_"
	PUSH HL
	CALL NEWSTART + (DOLOAD - MAIN)
; 7+4+6					_obocop  1
	; page in 7
	LD A,0x07
	CALL NEWSTART + (PAGEBANK - MAIN)
	; decompress data(7) to bank 7
	LD HL,27000 ; Set to new start of data
	LD DE,49152 ; Destination
	CALL NEWSTART + (DEC40 - MAIN)
	; page in bank 0
	CALL NEWSTART + (PAGEBANKZERO - MAIN)
	; copy data(4+6) to top of bank 0->2
	LD HL,27000 + 10098 + 19232 - 1 ; Set to end of data
	LD DE,65535
	LD BC,19232	; Set to data length
	LDDR
	; decompress data(4+6) to bank 2+0
	LD HL,65536 - 19232 ; Set to new start of data
	LD DE,32768 - 10; Destination
	CALL NEWSTART + (DEC40 - MAIN)
	; shift decompressed data to its proper position
	CALL NEWSTART + (UPSHIFT - MAIN)
	; page in bank 4
	LD A,0x04
	CALL NEWSTART + (PAGEBANK - MAIN)
	; copy bank 2(4) to bank 4
	CALL NEWSTART + (BANKMIDTOHIGH - MAIN)
	; page in bank 0
	CALL NEWSTART + (PAGEBANKZERO - MAIN)
	; copy bank 0(6) to bank 2
	CALL NEWSTART + (BANKHIGHTOMID - MAIN)
	; page in bank 6
	LD A,0x06
	CALL NEWSTART + (PAGEBANK - MAIN)
	; copy bank 2(6) to bank 6
	CALL NEWSTART + (BANKMIDTOHIGH - MAIN)
	; page in bank 0
	CALL NEWSTART + (PAGEBANKZERO - MAIN)
	; load next group
LOAD3
	POP HL
	INC HL
	LD (HL), "_"
	CALL NEWSTART + (DOLOAD - MAIN)
; 5(after screen)+2+0		_obocop  2
	; copy data to memtop
	LD HL,27000 + 30263 - 1 ; Set to end of data
	LD DE,65535
	LD BC,30263	; Set to data length
	LDDR
	
	RET
	
FINALDECOMP
	; set regs
LOADREG
	LD SP,NEWSTART + (REGDATASTACK1 - MAIN)
	POP BC		; BC'
	POP DE		; DE'
	POP HL		; HL'
	EXX
	POP IX		; IX
	POP IY		; IY
	POP AF		; IR
	LD I,A
	; relocate to screen	
SHIFTDEPACK
	LD HL,NEWSTART + (NEWDEPACK - MAIN)
	LD DE,16384
	LD BC,END_DEC40 - NEWDEPACK
	LDIR
	LD SP,16384 + (REGDATASTACK2 - NEWDEPACK)
CALLDEPACK
	LD HL,65536 - 30263	; Set to new start of data
	LD DE,16384 + 6912	; Destination
JUMPTONEWDEPACK
	JP 16384
	; decomp pages 5+2+0 to banks 5+2+0
NEWDEPACK
	CALL 16384 + (DEC40 - NEWDEPACK)
	; fill last 10 bytes
	LD HL,16384 + (LASTBYTES - NEWDEPACK)
	LD DE,65536 - 10
	LD BC,10
	LDIR
	; set last regs
SETLASTREGS
	POP DE		; DE
	POP HL		; HL
	POP AF		; AF'
	EX AF,AF'	; '
	LD A,0x00	; Set ram/rom pages
	LD BC,0x77FD
	OUT (C),A
	POP BC		; R and Border
	LD A,C
	LD C,0xFE
	OUT (C),A	; set Border
	LD A,B
	LD R,A		; set R
	POP BC		; BC
	POP AF		; AF
	LD SP,0xAA45	; SP
	EI		; NOP out if DI
RUNPROG
	JP 0x9BA3	; PC
	DEFW 0x0000	; for depacker stack usage
	DEFW 0x0000	; for DE call
REGDATASTACK2
	DEFW 0x4000	; DE
	DEFW 0x5C08	; HL
	DEFW 0x5F4D	; AF'
	DEFB 0x02	; Border Colour
	DEFB 0x1	; R (R minus 5)
	DEFW 0xC05F	; BC
	DEFW 0x2108	; AF
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
        EX      AF,AF'	; '
MS      LDI
M0      LD      BC,0x02FF
M1      EX      AF,AF'	; '
M1X     ADD     A,A
        JR      NZ,M2
        LD      A,(HL)
        INC     HL
        RLA
M2      RL      C
        JR      NC,M1X
        EX      AF,AF'	; '
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
        EX      AF,AF'	; '
        INC     B
N5      RR      C
        RET     C
;        OUT		(0xFE),A	; Added to toggle border during decompression, costs 3 bytes but makes noise
        RL      B
        ADD     A,A
        JR      NZ,N6
        LD      A,(HL)
        INC     HL
        RLA
N6      JR      NC,N5
        EX      AF,AF'	; '
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
	DEFW 0x0000	; BC'
	DEFW 0x0001	; DE'
	DEFW 0x2758	; HL'
	DEFW 0xCA98	; IX
	DEFW 0xE258	; IY
	DEFW 0x8310	; IR
SCREENDATA
