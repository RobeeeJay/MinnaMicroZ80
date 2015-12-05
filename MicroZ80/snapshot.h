enum snapVersion
{
	SNAP_VERSION_UNKNOWN,
	SNAP_VERSION_ONE,
	SNAP_VERSION_TWO,
	SNAP_VERSION_THREE,
	SNAP_VERSION_THREEA
};

enum snapType
{
	SNAP_TYPE_UNKNOWN,
	SNAP_TYPE_48K,
	SNAP_TYPE_128K
};

#pragma pack(show)
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)
#pragma pack(show)

struct snapHeaderV1
{
    UBYTE A;
    UBYTE F;
    UWORD BC;
    UWORD HL;
    UWORD PC;
    UWORD SP;
    UBYTE InterruptRegister;
    UBYTE RefreshRegister;
    UBYTE Flags1;
    UWORD DE;
    UWORD BC_Dash;
    UWORD DE_Dash;
    UWORD HL_Dash;
    UBYTE A_Dash;
    UBYTE F_Dash;
    UWORD IY;
    UWORD IX;
    UBYTE InterruptFlipFlop;
    UBYTE IFF2;
    UBYTE Flags2;
};

struct snapHeaderV2
{
    UWORD Length;
	UWORD PC;
    UBYTE HardwareMode;
	UBYTE LastOutRam;
	UBYTE InterfaceOne;
	UBYTE Flags1;
	UBYTE LastOutSound;
	UBYTE AYReg[16];
};

struct snapHeaderV3
{
	UWORD LowTState;
	UBYTE HighTState;
	UBYTE FlagSpectator;
	UBYTE MgtRom;
	UBYTE MultifaceRom;
	UBYTE RomIsRam;
	UBYTE RamIsRom;
	UBYTE KeyboardMap[10];
	UBYTE AsciiMap[10];
	UBYTE MGTType;
	UBYTE DiscipleInhibit;
	UBYTE DiscipleFlag;
	UBYTE LastOut2ARam;
};

struct snapPage
{
    UWORD Length;
	UBYTE Page;
    UBYTE Data[4];
};

//Depacker notes...
//Register usage: AF,AF',BC,DE,HL. Must be CALL'ed, return is done by RET.
//Provide extra stack location for store 2 bytes (1 word). Depacker does not
//disable or enable interrupts, as well as could be interrupted at any time

struct snapBase
{
	// Following registers get set post-depack (Refresh needs to be calculated properly)
    UBYTE A;
    UBYTE F;
    UBYTE A_Dash;
    UBYTE F_Dash;
    UWORD BC;
    UWORD DE;
    UWORD HL;
    UWORD PC;
    UWORD SP;
	UBYTE InterruptEnable;
    UBYTE RefreshRegister;
	UBYTE Border;
	// Following registers can be set pre-depack
	UWORD BC_Dash;
    UWORD DE_Dash;
    UWORD HL_Dash;
    UWORD IY;
    UWORD IX;
    UBYTE InterruptRegister;
	UBYTE InterruptMode;
	UWORD intScreenSize;
	UWORD intDataSize;
	UBYTE RamOut;
};

struct snapFourtyEight : public snapBase
{
	UWORD CompLength;
	UBYTE BankFive[16384];		// 4000-7fff
	UBYTE BankTwo[16384];		// 8000-bfff
	UBYTE BankZero[16384];		// c000-ffff
	UBYTE Compressed[49152];
};

struct snapOneTwoEight : public snapBase
{
	UWORD CompOneLength;
	UWORD CompTwoLengthBankSeven;
	UWORD CompTwoLength;
	UWORD CompThreeLength;

	UBYTE Filename[10];
	UBYTE FilenameLen;

	UBYTE BankOne[16384];
	UBYTE BankThree[16384];

	UBYTE BankSeven[16384];
	UBYTE BankFour[16384];
	UBYTE BankSix[16384];
	
	UBYTE BankFive[16384];
	UBYTE BankTwo[16384];
	UBYTE BankZero[16384];

	UBYTE CompressedOne[49152];
	UBYTE CompressedTwo[49152];
	UBYTE CompressedThree[49152];
};

struct snapBanks
{
	snapPage *pageThree;
	snapPage *pageFour;
	snapPage *pageFive;
	snapPage *pageSix;
	snapPage *pageSeven;
	snapPage *pageEight;
	snapPage *pageNine;
	snapPage *pageTen;
};

struct tzxHeader
{
	UBYTE Signature[7];	// Should equal "ZXTape!"
	UBYTE Marker;		// Should be 0x1A (ctrl-z)
	UBYTE MajorRevision;
	UBYTE MinorRevision;
};

struct tzxStandardBlock
{
	UBYTE Id;
	UWORD Pause;
	UWORD Length;
	UBYTE Data[1];
};

struct tzxDataHeader
{
	UBYTE Flag; // Always 0 for standard ROM loaded header
	UBYTE Type; // Always 3 for a CODE header
	UBYTE Filename[10];
	UWORD Length; // LSB MSB
	UWORD Start; // LSB MSB
	UWORD Unused; // LSB MSB and 32768
	UBYTE Checksum; // All bytes XORed
};

struct tzxDataBody
{
	UBYTE Flag; // Always 255 for standard ROM loaded data
	UBYTE Data[1];
	UBYTE Checksum; // All bytes XORed
};

struct depackCode48
{
	UBYTE diim[2];
	UBYTE interruptmode;
	UBYTE codeblock1[22];
	UBYTE codeblock2[9];
	UWORD datapackend;	// Set to start of pack data PACK_START + screen packed size
	UBYTE codeblock3[4];
	UWORD datapacksize;		// Set to packed data size
	UBYTE codeblock4[17];
	UWORD datanewpackstart;	// Set to start of pack data (65536 - packed size)
	UBYTE codeblock5[36];
	UWORD snapsp;	// Set to snapshot stack pointer
	UBYTE ei;	// nop out if we dont enable interrupts
	UBYTE jp;
	UWORD pc;			// Set to snapshot program counter
// Snapshot register values
	UWORD stack_blank[2];
	UWORD stack_de;
	UWORD stack_hl;
	UWORD stack_afd;
	UBYTE stack_border;
	UBYTE stack_r;
	UWORD stack_bc;
	UWORD stack_af;
	UBYTE safetyblock[10];
// Dec 40 code
	UBYTE dec40[110];
// Remaining snapshot register values
	UWORD stack_bcd;
	UWORD stack_ded;
	UWORD stack_hld;
	UWORD stack_ix;
	UWORD stack_iy;
	UBYTE stack_unused;
	UBYTE stack_i;
};

struct depackCode128
{
	UBYTE codeblock1[39 + 15];
	UBYTE interruptmode;
	UBYTE codeblock2[19];
	UWORD filenamelen;
	UBYTE filename[10];
	UBYTE codeblock3[171];
	UBYTE codeblock4[7];
	UWORD databank13top;  // LD HL,27000 + depack128_size + datascreenlen + databank13len - 1 ; Set to end of data
	UBYTE codeblock5[4];
	UWORD databank13len;	// Set to bank 1 + 3 data length
	UBYTE codeblock6[3];
	UWORD databank13newstart; // 65536 - databank13len
	UBYTE codeblock7[61];
	UWORD databank46top;	// LD HL,27000 + bank7len + bank46len - 1 ; Set to end of data
	UBYTE codeblock8[4];
	UWORD databank46len;
	UBYTE codeblock9[3];
	UWORD databank46newstart;
	UBYTE codeblock10[42];
	UWORD databank520top;
	UBYTE codeblock11[4];
	UWORD databank520len;
	UBYTE codeblock12[32];
	UWORD databank520newstart;	// 65536 - databank520len
	UBYTE codeblock13[25];
	UBYTE dataramrompages;
	UBYTE codeblock14[17];
	UWORD snapsp;	// Set to snapshot stack pointer
	UBYTE ei;		// nop out if we dont enable interruptss
	UBYTE jp;
	UWORD pc;			// Set to snapshot program counter
// Snapshot register values
	UWORD stack_blank[2];
	UWORD stack_de;
	UWORD stack_hl;
	UWORD stack_afd;
	UBYTE stack_border;
	UBYTE stack_r;
	UWORD stack_bc;
	UWORD stack_af;
	UBYTE safetyblock[10];
	UBYTE dec40[110];
// Remaining snapshot register values
	UWORD stack_bcd;
	UWORD stack_ded;
	UWORD stack_hld;
	UWORD stack_ix;
	UWORD stack_iy;
	UBYTE stack_unused;
	UBYTE stack_i;
};

#define ASM_START	27000
#define ASM_SHIFTED_START	25000
#define ASM_LEN		sizeof(depackCode)
#define PACK_START	(ASM_START + ASM_LEN)
#define MAXSNAP48SIZE	(65536 - ASM_START)
#define MAXSNAP128SIZE	(65536 - ASM_START)
#define MAXSNAP128TOTAL 92160

#pragma pack(pop)   /* restore original alignment from stack */
