// MicroZ80.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "z80_48code.h"
#include "z80_128code.h"

int validCmdLine(int argc, char *argv[]);
void printUsage(void);
UBYTE *readSnapshot(char *strFilename);
snapVersion checkSnapshotVersion(UBYTE *bSnapshot, UBYTE **bData);
snapType checkSnapshotType(snapVersion snapVer, snapHeaderV2 *v2Header);
void copyMachineStat(snapHeaderV1 *v1Header, snapHeaderV2 *v2Header, snapBase *spSnapBase);
UWORD decompBank(snapPage *bank, UBYTE *dest);
void setBankSize(snapPage *snPage);

int checkBanks48(UBYTE *bData, snapBanks *snapBankManager);
void decompressBanks48(snapBanks *snapBankManager, snapFourtyEight *spFourtyEight);
void compressSnapshot48(snapFourtyEight *spFourtyEight);
void setDecompressRegisters48(snapFourtyEight *spFourtyEight);
void writeTZXFile48(char *strInputFilename, char *strSpecFilename, snapFourtyEight *spFourtyEight);

int checkBanks128(UBYTE *bData, snapBanks *snapBankManager);
void decompressBanks128(snapBanks *snapBankManager, snapOneTwoEight *spOneTwoEight);
void compressSnapshot128(snapOneTwoEight *spOneTwoEight);
void setDecompressRegisters128(snapOneTwoEight *spOneTwoEight);
void writeTZXFile128(char *strInputFilename, char *strSpecFilename, snapOneTwoEight *spOneTwoEight);

UWORD swapLsbMsb(UWORD word);
UBYTE checksumXor(UBYTE seed, UBYTE *ptr, UWORD len);
UBYTE checksumXor2(UBYTE seed, UBYTE *ptr, UWORD len);


int main(int argc, char *argv[])
{
	UBYTE *bSnapshot, *bData;
	snapVersion snapVer;

	printf("MinnaMicroZ80 v0.22 - The Z80 Snapshot to Microdrive Converter\n\n");
	//printf("THIS IS A DEBUG VERSION - DO NOT DISTRIBUTE!!!!111one1one!11eleven\n\n");
	//printf("depack48 size: %u, struct size: %u\n", depack48_size, (unsigned int)sizeof(depackCode48));
	//printf("depack128 size: %u, struct size: %u\n", depack128_size, (unsigned int)sizeof(depackCode128));

	// Validate command line
	if (!validCmdLine(argc, argv))
		return 0;

	// Load first arg as file into memory
	bSnapshot = readSnapshot(argv[1]);
	if (!bSnapshot)
		return 0;

	snapVer = checkSnapshotVersion(bSnapshot, &bData);
	if (snapVer > SNAP_VERSION_TWO)
	{
		snapHeaderV1 *v1Header;
		snapHeaderV2 *v2Header;
		snapHeaderV3 *v3Header;
		snapType snapMachine;
		snapBanks snapBankManager;
		
		v1Header = (snapHeaderV1 *)bSnapshot;
		v2Header = (snapHeaderV2 *)(bSnapshot + 30);
		v3Header = (snapHeaderV3 *)(bSnapshot + 55);
		
		snapMachine = checkSnapshotType(snapVer, v2Header);
		
		if (snapMachine == SNAP_TYPE_48K)
		{
			snapFourtyEight spFourtyEight;

			// Copy registers etc into structure
			copyMachineStat(v1Header, v2Header, &spFourtyEight);

			// Verify banks and find their offsets
			if (checkBanks48(bData, &snapBankManager))
			{
				// Now decompress each page into the struct
				decompressBanks48(&snapBankManager, &spFourtyEight);

				compressSnapshot48(&spFourtyEight);
				setDecompressRegisters48(&spFourtyEight);

				printf("Total snapshot size on Microdrive will be %lu bytes\n", spFourtyEight.CompLength);

				// If size > MAXSNAP48SIZE error
				if (spFourtyEight.CompLength > MAXSNAP48SIZE)
				{
					printf("SNAPSHOT TOO BIG!\n");
					printf("Unfortunately any snapshot that fails to compress down to %i doesn't leave enough room for our basic program to launch it :(\n", MAXSNAP48SIZE);
				}
				else
				{
					printf("Writing TZX file...\n");
					writeTZXFile48(argv[1], argv[2], &spFourtyEight);
					printf("TZX created, now copy the file inside it straight onto a real Microdrive equipped Speccy along with the launcher!\n");
				}
			}
		}
		else if (snapMachine == SNAP_TYPE_128K)
		{
			snapOneTwoEight spOneTwoEight;

			// Copy registers etc into structure
			copyMachineStat(v1Header, v2Header, &spOneTwoEight);

			// Verify banks and find their offsets
			if (checkBanks128(bData, &snapBankManager))
			{
				ULONG ulTotal;

				// Now decompress each page into the struct
				decompressBanks128(&snapBankManager, &spOneTwoEight);

				// Copy filename
				spOneTwoEight.FilenameLen = strlen(argv[2]);
				if (spOneTwoEight.FilenameLen > 10)
					spOneTwoEight.FilenameLen = 10;
				memcpy(spOneTwoEight.Filename, argv[2], spOneTwoEight.FilenameLen);
				if (spOneTwoEight.FilenameLen < 10)
					memset(spOneTwoEight.Filename + spOneTwoEight.FilenameLen, 32, 10 - spOneTwoEight.FilenameLen);
				
				compressSnapshot128(&spOneTwoEight);
				// Set decomp registers
				setDecompressRegisters128(&spOneTwoEight);

				ulTotal = spOneTwoEight.CompOneLength + spOneTwoEight.CompTwoLength + spOneTwoEight.CompThreeLength;
				printf("Total snapshot size on Microdrive will be %lu bytes\n", ulTotal);

				// If size > MAXSNAP48SIZE error
				if (spOneTwoEight.CompOneLength > MAXSNAP48SIZE)
				{
					printf("SNAPSHOT TOO BIG!\n");
					printf("Unfortunately any snapshot that fails to compress down to %i doesn't leave enough room for our basic program to launch it :(\n", MAXSNAP48SIZE);
				}
				else
				{
					if (ulTotal > MAXSNAP128TOTAL)
					{
						printf("*****************************\n");
						printf("WARNING!!!! SNAPSHOT TOO BIG!\n");
						printf("*****************************\n");
						printf("This snapshot exceeds the Microdrive size limit of %i and may not fit! :(\n", MAXSNAP128TOTAL);
					}
					
					if ((spOneTwoEight.CompTwoLength > MAXSNAP128SIZE) || (spOneTwoEight.CompThreeLength > MAXSNAP128SIZE))
					{
						printf("SNAPSHOT TOO BIG!\n");
						printf("Unfortunately any snapshot segment that fails to compress down to %i won't leave room for the decompression code :(\n", MAXSNAP128SIZE);
					}
					else
					{
						printf("Writing TZX file...\n");
						writeTZXFile128(argv[1], argv[2], &spOneTwoEight);
						printf("TZX created, now copy the files inside it straight onto a real Microdrive equipped Speccy along with the launcher!\n");
					}
				}
			}
		}
		else
		{
			printf("Doing nothing\n");
		}
	}
	else
		printf("This snapshot version is not currently supported, sorry!\n");
	
	// Tidy up
	free(bSnapshot);

	return 1;
}

int validCmdLine(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("ERROR: Please specify source and spectrum filenames!\n", argc);
		printUsage();
		return false;
	}

	if ((strstr(argv[1], ".z80") == NULL) && (strstr(argv[1], ".Z80") == NULL))
	{
		printf("ERROR: Please specify a z80 snapshot!\n", argc);
		printUsage();
		return false;
	}

	if (strlen(argv[2]) > 10)
	{
		printf("ERROR: Spectrum filenames must be 10 characters or less!\n", argc);
		printUsage();
		return false;
	}
	return true;
}

void printUsage(void)
{
	printf("\nMicroZ80.exe snapshot.z80 spectrumfilename\n\n");
}

UBYTE *readSnapshot(char *strFilename)
{
	FILE *fhSnapshot;
	ULONG ulFileLen;
	UBYTE *bSnapshot;

	printf("Loading %s...\n", strFilename);

	fhSnapshot = fopen(strFilename, "rb");
	fseek(fhSnapshot, 0, SEEK_END);
	ulFileLen = ftell(fhSnapshot);
	fseek(fhSnapshot, 0, SEEK_SET);

	if (ulFileLen < 100)
	{
		printf("ERROR: File is too small to be a valid z80 snapshot!\n");
		fclose(fhSnapshot);
		return NULL;
	}
	if (ulFileLen > (130 * 1024))
	{
		printf("ERROR: File is too large to be a z80 snapshot!\n");
		fclose(fhSnapshot);
		return NULL;
	}
	
	bSnapshot = (UBYTE *)malloc(ulFileLen);
	if (ulFileLen != fread(bSnapshot, 1, ulFileLen, fhSnapshot))
	{
		printf("ERROR: Problem reading z80 snapshot!\n");
		fclose(fhSnapshot);
		free(bSnapshot);
		return NULL;
	}

	fclose(fhSnapshot);

	return bSnapshot;
}

snapVersion checkSnapshotVersion(UBYTE *bSnapshot, UBYTE **bData)
{
	snapVersion snapVer;
	snapHeaderV1 *v1Header;
	snapHeaderV2 *v2Header;

	// Point correct z80 struct at file
	v1Header = (snapHeaderV1 *)bSnapshot;
	snapVer = SNAP_VERSION_ONE;

	if (v1Header->Flags1 == 255)
	{
		printf("Version 1 file format\n");
	}
	else if (v1Header->PC == 0)
	{
		v2Header = (snapHeaderV2 *)(bSnapshot + 30);
		if (v2Header->Length == 23)
		{
			printf("Version 2 file format\n");
			*bData = bSnapshot + 55;
			snapVer = SNAP_VERSION_TWO;
		}
		else
		{
			if (v2Header->Length == 54)
			{
				printf("Version 3 file format\n");
				*bData = bSnapshot + 86;
				snapVer = SNAP_VERSION_THREE;
			}
			else if (v2Header->Length == 55)
			{
				printf("Version 3 2A/+3 file format\n");
				*bData = bSnapshot + 87;
				snapVer = SNAP_VERSION_THREEA;
			}
			else
			{
				printf("ERROR: Corrupt z80 snapshot!\n");
				return SNAP_VERSION_UNKNOWN;
			}
		}
	}

	return snapVer;
}

snapType checkSnapshotType(snapVersion snapVer, snapHeaderV2 *v2Header)
{
	if (v2Header->HardwareMode == 0)
	{
		printf("Snapshot is from a 48k machine\n");
		return SNAP_TYPE_48K;
	}
	else if (v2Header->HardwareMode == 1)
	{
		printf("Snapshot is from a 48k machine with Interface 1\n");
		return SNAP_TYPE_48K;
	}
	else if ((snapVer == SNAP_VERSION_TWO) && (v2Header->HardwareMode == 3))
	{
		printf("Snapshot is from a 128k machine\n");
		return SNAP_TYPE_128K;
	}
	else if ((snapVer == SNAP_VERSION_TWO) && (v2Header->HardwareMode == 4))
	{
		printf("Snapshot is from a 128k machine with Interface 1\n");
		return SNAP_TYPE_128K;
	}
	else if ((snapVer > SNAP_VERSION_TWO) && (v2Header->HardwareMode == 4))
	{
		printf("Snapshot is from a 128k machine\n");
		return SNAP_TYPE_128K;
	}
	else if ((snapVer > SNAP_VERSION_TWO) && (v2Header->HardwareMode == 5))
	{
		printf("Snapshot is from a 128k machine with Interface 1\n");
		return SNAP_TYPE_128K;
	}
	
	printf("NOTE: The hardware this Snapshot represents is Unsupported - Results may be unexpected :(\n");

	return SNAP_TYPE_UNKNOWN;
}

void copyMachineStat(snapHeaderV1 *v1Header, snapHeaderV2 *v2Header, snapBase *spSnapBase)
{
	spSnapBase->A = v1Header->A;
	spSnapBase->F = v1Header->F;
	spSnapBase->A_Dash = v1Header->A_Dash;
	spSnapBase->F_Dash = v1Header->F_Dash;
	spSnapBase->BC = v1Header->BC;
	spSnapBase->DE = v1Header->DE;
	spSnapBase->HL = v1Header->HL;
	spSnapBase->PC = v2Header->PC;
	spSnapBase->SP = v1Header->SP;
	spSnapBase->InterruptEnable = (v1Header->InterruptFlipFlop == 0)? 0 : 1;
	spSnapBase->RefreshRegister = v1Header->RefreshRegister;
	spSnapBase->BC_Dash = v1Header->BC_Dash;
	spSnapBase->DE_Dash = v1Header->DE_Dash;
	spSnapBase->HL_Dash = v1Header->HL_Dash;
	spSnapBase->IY = v1Header->IY;
	spSnapBase->IX = v1Header->IX;
	spSnapBase->InterruptRegister = v1Header->InterruptRegister;
	spSnapBase->Border = (v1Header->Flags1 >> 1) & 0x07;
	spSnapBase->InterruptMode = v1Header->Flags2 & 0x03;
	spSnapBase->RamOut = v2Header->LastOutRam;
}

int checkBanks48(UBYTE *bData, snapBanks *snapBankManager)
{
	snapBankManager->pageFour = (snapPage *)bData;
	setBankSize(snapBankManager->pageFour);
	if (snapBankManager->pageFour->Page != 4)
	{
		printf("ERROR: First page not 4!\n");
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageFour->Page, snapBankManager->pageFour->Length);
	
	snapBankManager->pageFive = (snapPage *)((BYTE *)snapBankManager->pageFour + snapBankManager->pageFour->Length + 3);
	setBankSize(snapBankManager->pageFive);
	if (snapBankManager->pageFive->Page != 5)
	{
		printf("ERROR: Second page not 5!\n");
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageFive->Page, snapBankManager->pageFive->Length);

	snapBankManager->pageEight = (snapPage *)((BYTE *)snapBankManager->pageFive + snapBankManager->pageFive->Length + 3);
	setBankSize(snapBankManager->pageEight);
	if (snapBankManager->pageEight->Page != 8)
	{
		printf("ERROR: Second page not 8!\n");
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageEight->Page, snapBankManager->pageEight->Length);

	return true;
}

void setBankSize(snapPage *snPage)
{
	if (snPage->Length == 0xffff)
		snPage->Length = 16384;
}

int checkBanks128(UBYTE *bData, snapBanks *snapBankManager)
{
	snapBankManager->pageThree = (snapPage *)bData;
	setBankSize(snapBankManager->pageThree);
	if (snapBankManager->pageThree->Page != 3)
	{
		printf("ERROR: First page not 3! It's %u\n", snapBankManager->pageThree->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageThree->Page, snapBankManager->pageThree->Length);
	
	snapBankManager->pageFour = (snapPage *)((BYTE *)snapBankManager->pageThree + snapBankManager->pageThree->Length + 3);
	setBankSize(snapBankManager->pageFour);
	if (snapBankManager->pageFour->Page != 4)
	{
		printf("ERROR: Second page not 4! It's %u\n", snapBankManager->pageFour->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageFour->Page, snapBankManager->pageFour->Length);

	snapBankManager->pageFive = (snapPage *)((BYTE *)snapBankManager->pageFour + snapBankManager->pageFour->Length + 3);
	setBankSize(snapBankManager->pageFive);
	if (snapBankManager->pageFive->Page != 5)
	{
		printf("ERROR: Third page not 5! It's %u\n", snapBankManager->pageFive->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageFive->Page, snapBankManager->pageFive->Length);

	snapBankManager->pageSix = (snapPage *)((BYTE *)snapBankManager->pageFive + snapBankManager->pageFive->Length + 3);
	setBankSize(snapBankManager->pageSix);
	if (snapBankManager->pageSix->Page != 6)
	{
		printf("ERROR: Fourth page not 6! It's %u\n", snapBankManager->pageSix->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageSix->Page, snapBankManager->pageSix->Length);

	snapBankManager->pageSeven = (snapPage *)((BYTE *)snapBankManager->pageSix + snapBankManager->pageSix->Length + 3);
	setBankSize(snapBankManager->pageSeven);
	if (snapBankManager->pageSeven->Page != 7)
	{
		printf("ERROR: Fifth page not 7! It's %u\n", snapBankManager->pageSeven->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageSeven->Page, snapBankManager->pageSeven->Length);

	snapBankManager->pageEight = (snapPage *)((BYTE *)snapBankManager->pageSeven + snapBankManager->pageSeven->Length + 3);
	setBankSize(snapBankManager->pageEight);
	if (snapBankManager->pageEight->Page != 8)
	{
		printf("ERROR: Six page not 8! It's %u\n", snapBankManager->pageEight->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageEight->Page, snapBankManager->pageEight->Length);

	snapBankManager->pageNine = (snapPage *)((BYTE *)snapBankManager->pageEight + snapBankManager->pageEight->Length + 3);
	setBankSize(snapBankManager->pageNine);
	if (snapBankManager->pageNine->Page != 9)
	{
		printf("ERROR: Seven page not 9! It's %u\n", snapBankManager->pageNine->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageNine->Page, snapBankManager->pageNine->Length);

	snapBankManager->pageTen = (snapPage *)((BYTE *)snapBankManager->pageNine + snapBankManager->pageNine->Length + 3);
	setBankSize(snapBankManager->pageTen);
	if (snapBankManager->pageTen->Page != 10)
	{
		printf("ERROR: Eight page not 10! It's %u\n", snapBankManager->pageTen->Page);
		return false;
	}
	printf("Found page %u, Size: %u\n", (int)snapBankManager->pageTen->Page, snapBankManager->pageTen->Length);

	return true;
}

void compressSnapshot48(snapFourtyEight *spFourtyEight)
{
	// Add decompress code to start of CompLength
	memcpy(spFourtyEight->Compressed, depack48, depack48_size);
	spFourtyEight->CompLength = depack48_size;

	// Compress the screen on its own
	inlen = 6912;
	indata = spFourtyEight->BankFive;
	printf("Compressing screen with MegaLZ...\n");
	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	printf("MegaLZ compressed 6192 bytes down to %lu\n", ob_freepos);

	memcpy(spFourtyEight->Compressed + spFourtyEight->CompLength, oubuf, ob_freepos);
	spFourtyEight->CompLength += ob_freepos;
	spFourtyEight->intScreenSize = ob_freepos;

	// Now we compress all the remaining memory bar the last 10 bytes
	inlen = 49152 - 6912 - 10;
	indata = spFourtyEight->BankFive + 6912;

	printf("Compressing with MegaLZ...\n");
	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	// Add compressed memory to our final snapshot
	memcpy(spFourtyEight->Compressed + spFourtyEight->CompLength, oubuf, ob_freepos);
	spFourtyEight->CompLength += ob_freepos;
	spFourtyEight->intDataSize = ob_freepos;
	
	printf("MegaLZ compressed 42240 bytes down to %lu\n", ob_freepos);
}

void compressSnapshot128(snapOneTwoEight *spOneTwoEight)
{
	// load_decomp+screen+1+3	robocop
	// 7+4+6					_obocop  1
	// 5(after screen)+2+0		_obocop  2

	// Add decompress code to start of CompLength
	memcpy(spOneTwoEight->CompressedOne, depack128, depack128_size);
	spOneTwoEight->CompOneLength = depack128_size;

	// Compress the screen on its own
	inlen = 6912;
	indata = spOneTwoEight->BankFive;
	printf("Compressing screen with MegaLZ...\n");
	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	printf("MegaLZ compressed 6912 bytes down to %lu\n", ob_freepos);

	memcpy(spOneTwoEight->CompressedOne + spOneTwoEight->CompOneLength, oubuf, ob_freepos);
	spOneTwoEight->CompOneLength += ob_freepos;
	spOneTwoEight->intScreenSize = ob_freepos;


	// Now we compress banks 1 + 3
	inlen = 32768;
	indata = spOneTwoEight->BankOne;

	printf("Compressing Banks 1 + 3 with MegaLZ...\n");
	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	// Add compressed memory to our final snapshot
	memcpy(spOneTwoEight->CompressedOne + spOneTwoEight->CompOneLength, oubuf, ob_freepos);
	spOneTwoEight->CompOneLength += ob_freepos;
	spOneTwoEight->intDataSize = ob_freepos;
	
	printf("MegaLZ compressed 32768 bytes down to %lu\n", ob_freepos);


	// Now we compress banks 7 + 4 + 6
	// But we keep 7 seperate for decompression reasons
	inlen = 16384;
	indata = spOneTwoEight->BankSeven;

	printf("Compressing Banks 7 + 4 + 6 with MegaLZ...\n");
	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	// Add compressed memory to our final snapshot
	memcpy(spOneTwoEight->CompressedTwo, oubuf, ob_freepos);
	spOneTwoEight->CompTwoLength = ob_freepos;
	spOneTwoEight->CompTwoLengthBankSeven = ob_freepos;

	printf("MegaLZ compressed 16384 bytes down to %lu\n", ob_freepos);
	
	inlen = 32768;
	indata = spOneTwoEight->BankFour;

	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	// Add compressed memory to our final snapshot
	memcpy(spOneTwoEight->CompressedTwo + spOneTwoEight->CompTwoLength, oubuf, ob_freepos);
	spOneTwoEight->CompTwoLength += ob_freepos;

	printf("MegaLZ compressed 32768 bytes down to %lu\n", ob_freepos);

	// Finally we compress the remainder of 5 + 2 + 0 minus 10
	inlen = 49152 - 6912 - 10;
	indata = spOneTwoEight->BankFive + 6912;

	printf("Compressing Banks 5 + 2 + 0 with MegaLZ...\n");
	if (pack() == 0)
		printf("ERROR: MegaLZ reported a pack error :(\n");

	// Add compressed memory to our final snapshot
	memcpy(spOneTwoEight->CompressedThree, oubuf, ob_freepos);
	spOneTwoEight->CompThreeLength = ob_freepos;
	
	printf("MegaLZ compressed 42240 bytes down to %lu\n", ob_freepos);
}

void setDecompressRegisters48(snapFourtyEight *spFourtyEight)
{
	depackCode48 *dePacker;

	// Set regs etc in decompress code
	dePacker = (depackCode48 *)spFourtyEight->Compressed;
	//dePacker->datascreenstart = (UWORD)(ASM_START + depack48_size);
	dePacker->datapackend = (UWORD)(ASM_START + depack48_size + spFourtyEight->intScreenSize + spFourtyEight->intDataSize - 1);
	dePacker->datapacksize = (UWORD)spFourtyEight->intDataSize;
	dePacker->datanewpackstart = (UWORD)((int)65536 - (int)spFourtyEight->intDataSize);
	dePacker->snapsp = (UWORD)spFourtyEight->SP;
	if (spFourtyEight->InterruptEnable == 0)
		dePacker->ei = 0xF3;
	dePacker->pc = (UWORD)spFourtyEight->PC;
	dePacker->stack_de = (UWORD)spFourtyEight->DE;
	dePacker->stack_hl = (UWORD)spFourtyEight->HL;
	dePacker->stack_afd = spFourtyEight->F_Dash | (spFourtyEight->A_Dash << 8);
	dePacker->stack_r = (spFourtyEight->RefreshRegister - 5) & 0x7f;
	dePacker->stack_border = spFourtyEight->Border;
	dePacker->stack_bc = (UWORD)spFourtyEight->BC;
	dePacker->stack_af = spFourtyEight->F | (spFourtyEight->A << 8);
	dePacker->stack_bcd = (UWORD)spFourtyEight->BC_Dash;
	dePacker->stack_ded = (UWORD)spFourtyEight->DE_Dash;
	dePacker->stack_hld = (UWORD)spFourtyEight->HL_Dash;
	dePacker->stack_ix = (UWORD)spFourtyEight->IX;
	dePacker->stack_iy = (UWORD)spFourtyEight->IY;
	dePacker->stack_i = spFourtyEight->InterruptRegister;
	switch (spFourtyEight->InterruptMode)
	{
		case 1:
			dePacker->interruptmode = 0x56;
			break;
		case 2:
			dePacker->interruptmode = 0x5E;
			break;
		default:
			dePacker->interruptmode = 0x46;
	}

	memcpy(dePacker->safetyblock, spFourtyEight->BankZero + (16384 - 10), 10);
}

void setDecompressRegisters128(snapOneTwoEight *spOneTwoEight)
{
	depackCode128 *dePacker;

	// Set regs etc in decompress code
	dePacker = (depackCode128 *)spOneTwoEight->CompressedOne;

	memcpy(dePacker->filename, spOneTwoEight->Filename, 10);
	dePacker->filenamelen = spOneTwoEight->FilenameLen;
	dePacker->filename[0] = 0x5F;	// make first char _

	dePacker->databank13top = 27000 + spOneTwoEight->CompOneLength - 1;
	dePacker->databank13len = spOneTwoEight->CompOneLength - spOneTwoEight->intScreenSize - depack128_size;
	dePacker->databank13newstart = 65536 - dePacker->databank13len;
	dePacker->databank46top = 27000 + spOneTwoEight->CompTwoLength - 1;
	dePacker->databank46len = spOneTwoEight->CompTwoLength - spOneTwoEight->CompTwoLengthBankSeven;
	dePacker->databank46newstart = 65536 - dePacker->databank46len;
	dePacker->databank520top = 27000 + spOneTwoEight->CompThreeLength - 1;
	dePacker->databank520len = spOneTwoEight->CompThreeLength;
	dePacker->databank520newstart = 65536 - dePacker->databank520len;

	// note here we always ensure that bank 5 is paged in for the screen to stop our depack code being paged out
	dePacker->dataramrompages = spOneTwoEight->RamOut & 0xF7;

	dePacker->snapsp = (UWORD)spOneTwoEight->SP;
	if (spOneTwoEight->InterruptEnable == 0)
		dePacker->ei = 0xF3;
	dePacker->pc = (UWORD)spOneTwoEight->PC;
	dePacker->stack_de = (UWORD)spOneTwoEight->DE;
	dePacker->stack_hl = (UWORD)spOneTwoEight->HL;
	dePacker->stack_afd = spOneTwoEight->F_Dash | (spOneTwoEight->A_Dash << 8);
	dePacker->stack_r = (spOneTwoEight->RefreshRegister - 5) & 0x7f;
	dePacker->stack_border = spOneTwoEight->Border;
	dePacker->stack_bc = (UWORD)spOneTwoEight->BC;
	dePacker->stack_af = spOneTwoEight->F | (spOneTwoEight->A << 8);
	dePacker->stack_bcd = (UWORD)spOneTwoEight->BC_Dash;
	dePacker->stack_ded = (UWORD)spOneTwoEight->DE_Dash;
	dePacker->stack_hld = (UWORD)spOneTwoEight->HL_Dash;
	dePacker->stack_ix = (UWORD)spOneTwoEight->IX;
	dePacker->stack_iy = (UWORD)spOneTwoEight->IY;
	dePacker->stack_i = spOneTwoEight->InterruptRegister;
	switch (spOneTwoEight->InterruptMode)
	{
		case 1:
			dePacker->interruptmode = 0x56;
			break;
		case 2:
			dePacker->interruptmode = 0x5E;
			break;
		default:
			dePacker->interruptmode = 0x46;
	}

	memcpy(dePacker->safetyblock, spOneTwoEight->BankZero + (16384 - 10), 10);
}

void writeTZXFile48(char *strInputFilename, char *strSpecFilename, snapFourtyEight *spFourtyEight)
{
	// Create tzx file with code to copy to microdrive
	tzxHeader fileHeader;
	tzxStandardBlock fileBlock;
	tzxDataHeader codeHeader;
	tzxDataBody codeData;
	FILE *fhTzx;
	char *strFilename;

	memcpy(fileHeader.Signature, "ZXTape!", 7);
	fileHeader.Marker = 0x1A;
	fileHeader.MajorRevision = 1;
	fileHeader.MinorRevision = 20;

	strFilename = (char *)malloc(strlen(strInputFilename) + 1);
	strcpy(strFilename, strInputFilename);
	if (strlen(strFilename) > 4)
		strcpy(strFilename + strlen(strFilename) - 4, ".tzx");
	fhTzx = fopen(strFilename, "wb");
	free(strFilename);

	// Write out header
	fwrite(&fileHeader.Signature, 1, 10, fhTzx);

	// Write out standard block
	fileBlock.Id = 0x10;
	fileBlock.Pause = 1000;
	fileBlock.Length = 19;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);
	
	// Write out code header
	codeHeader.Flag = 0;
	codeHeader.Type = 3;
	memset(&codeHeader.Filename, 0x20, 10);
	memcpy(codeHeader.Filename, strSpecFilename, strlen(strSpecFilename));
	codeHeader.Length = spFourtyEight->CompLength;
	codeHeader.Start = ASM_START;
	codeHeader.Unused = 0x8040;
	codeHeader.Checksum = checksumXor(0, (UBYTE *)&codeHeader, 18);
	fwrite(&codeHeader.Flag, 1, 19, fhTzx);

	// Write out standard block
	fileBlock.Length = 2 + spFourtyEight->CompLength;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out code data
	codeData.Flag = 255;
	codeData.Checksum = checksumXor(255, spFourtyEight->Compressed, spFourtyEight->CompLength);
	fwrite(&codeData.Flag, 1, 1, fhTzx);
	fwrite(spFourtyEight->Compressed, 1, spFourtyEight->CompLength, fhTzx);
	fwrite(&codeData.Checksum, 1, 1, fhTzx);

	fclose(fhTzx);
}

void writeTZXFile128(char *strInputFilename, char *strSpecFilename, snapOneTwoEight *spOneTwoEight)
{
	// Create tzx file with code to copy to microdrive
	tzxHeader fileHeader;
	tzxStandardBlock fileBlock;
	tzxDataHeader codeHeader;
	tzxDataBody codeData;
	FILE *fhTzx;
	char *strFilename;

	memcpy(fileHeader.Signature, "ZXTape!", 7);
	fileHeader.Marker = 0x1A;
	fileHeader.MajorRevision = 1;
	fileHeader.MinorRevision = 20;

	strFilename = (char *)malloc(strlen(strInputFilename) + 1);
	strcpy(strFilename, strInputFilename);
	if (strlen(strFilename) > 4)
		strcpy(strFilename + strlen(strFilename) - 4, ".tzx");
	fhTzx = fopen(strFilename, "wb");
	free(strFilename);

	// Write out header
	fwrite(&fileHeader.Signature, 1, 10, fhTzx);

	// Write out standard block
	fileBlock.Id = 0x10;
	fileBlock.Pause = 1000;
	fileBlock.Length = 19;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out first code header
	codeHeader.Flag = 0;
	codeHeader.Type = 3;
	memset(&codeHeader.Filename, 0x20, 10);
	memcpy(codeHeader.Filename, strSpecFilename, strlen(strSpecFilename));
	codeHeader.Length = spOneTwoEight->CompOneLength;
	codeHeader.Start = ASM_START;
	codeHeader.Unused = 0x8040;
	codeHeader.Checksum = checksumXor(0, (UBYTE *)&codeHeader, 18);
	fwrite(&codeHeader.Flag, 1, 19, fhTzx);

	// Write out standard block
	fileBlock.Length = 2 + spOneTwoEight->CompOneLength;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out first code data
	codeData.Flag = 255;
	codeData.Checksum = checksumXor(255, spOneTwoEight->CompressedOne, spOneTwoEight->CompOneLength);
	fwrite(&codeData.Flag, 1, 1, fhTzx);
	fwrite(spOneTwoEight->CompressedOne, 1, spOneTwoEight->CompOneLength, fhTzx);
	fwrite(&codeData.Checksum, 1, 1, fhTzx);

	// Write out standard block
	fileBlock.Length = 19;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out second code header
	codeHeader.Filename[0] = '_';
	codeHeader.Length = spOneTwoEight->CompTwoLength;
	//codeHeader.Start = ASM_SHIFTED_START;
	codeHeader.Checksum = checksumXor(0, (UBYTE *)&codeHeader, 18);
	fwrite(&codeHeader.Flag, 1, 19, fhTzx);

	// Write out standard block
	fileBlock.Length = 2 + spOneTwoEight->CompTwoLength;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out second code data
	codeData.Checksum = checksumXor(255, spOneTwoEight->CompressedTwo, spOneTwoEight->CompTwoLength);
	fwrite(&codeData.Flag, 1, 1, fhTzx);
	fwrite(spOneTwoEight->CompressedTwo, 1, spOneTwoEight->CompTwoLength, fhTzx);
	fwrite(&codeData.Checksum, 1, 1, fhTzx);

	// Write out standard block
	fileBlock.Length = 19;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out third code header
	codeHeader.Filename[1] = '_';
	codeHeader.Length = spOneTwoEight->CompThreeLength;
	codeHeader.Checksum = checksumXor(0, (UBYTE *)&codeHeader, 18);
	fwrite(&codeHeader.Flag, 1, 19, fhTzx);

	// Write out standard block
	fileBlock.Length = 2 + spOneTwoEight->CompThreeLength;
	fwrite(&fileBlock.Id, 1, 5, fhTzx);

	// Write out third code data
	codeData.Checksum = checksumXor(255, spOneTwoEight->CompressedThree, spOneTwoEight->CompThreeLength);
	fwrite(&codeData.Flag, 1, 1, fhTzx);
	fwrite(spOneTwoEight->CompressedThree, 1, spOneTwoEight->CompThreeLength, fhTzx);
	fwrite(&codeData.Checksum, 1, 1, fhTzx);

	fclose(fhTzx);
}

void decompressBanks48(snapBanks *snapBankManager, snapFourtyEight *spFourtyEight)
{
	if (decompBank(snapBankManager->pageEight, spFourtyEight->BankFive) != 16384)
		printf("ERROR: Bank Five decompressed to something other than 16k!\n");
	if (decompBank(snapBankManager->pageFour, spFourtyEight->BankTwo) != 16384)
		printf("ERROR: Bank Two decompressed to something other than 16k!\n");
	if (decompBank(snapBankManager->pageFive, spFourtyEight->BankZero) != 16384)
		printf("ERROR: Bank Zero decompressed to something other than 16k!\n");
}

void decompressBanks128(snapBanks *snapBankManager, snapOneTwoEight *spOneTwoEight)
{
	if (decompBank(snapBankManager->pageThree, spOneTwoEight->BankZero) != 16384)
		printf("ERROR: Bank Zero decompressed to something other than 16k!\n");
	/*else
		printf("Bank Zero Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankZero, 16384));*/
	if (decompBank(snapBankManager->pageFour, spOneTwoEight->BankOne) != 16384)
		printf("ERROR: Bank One decompressed to something other than 16k!\n");
	/*else
		printf("Bank One Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankOne, 16384));*/
	if (decompBank(snapBankManager->pageFive, spOneTwoEight->BankTwo) != 16384)
		printf("ERROR: Bank Two decompressed to something other than 16k!\n");
	/*else
		printf("Bank Two Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankTwo, 16384));*/
	if (decompBank(snapBankManager->pageSix, spOneTwoEight->BankThree) != 16384)
		printf("ERROR: Bank Three decompressed to something other than 16k!\n");
	/*else
		printf("Bank Three Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankThree, 16384));*/
	if (decompBank(snapBankManager->pageSeven, spOneTwoEight->BankFour) != 16384)
		printf("ERROR: Bank Four decompressed to something other than 16k!\n");
	/*else
		printf("Bank Four Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankFour, 16384));*/
	if (decompBank(snapBankManager->pageEight, spOneTwoEight->BankFive) != 16384)
		printf("ERROR: Bank Five decompressed to something other than 16k!\n");
	/*else
		printf("Bank Five Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankFive, 16384));*/
	if (decompBank(snapBankManager->pageNine, spOneTwoEight->BankSix) != 16384)
		printf("ERROR: Bank Six decompressed to something other than 16k!\n");
	/*else
		printf("Bank Six Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankSix, 16384));*/
	if (decompBank(snapBankManager->pageTen, spOneTwoEight->BankSeven) != 16384)
		printf("ERROR: Bank Seven decompressed to something other than 16k!\n");
	/*else
		printf("Bank Seven Checksum: %u\n", (UWORD)checksumXor2(0, spOneTwoEight->BankSeven, 16384));*/
}

UWORD decompBank(snapPage *bank, UBYTE *dest)
{
	UWORD intLen, intPos, intRemaining;

	if (bank->Length == 16384)
	{
		printf("Copying page %u...\n", bank->Page);
		memcpy(dest, bank->Data, 16384);

		return 16384;
	}

	intLen = 0;
	intPos = 0;
	intRemaining = bank->Length;

	printf("Decompressing page %u...\n", bank->Page);
	
	while (intRemaining)
	{
		if (bank->Data[intPos] == 0xED)
		{
			if ((intRemaining > 1) && (bank->Data[intPos + 1] == 0xED))
			{
				UBYTE intRepeat;

				intRemaining -= 2;
				intPos += 2;
				intRepeat = bank->Data[intPos];
				intRemaining--;
				intPos++;
				
				while (intRepeat)
				{
					*dest++ = bank->Data[intPos];
					intRepeat--;
					intLen++;
				}

				intPos++;
				intRemaining--;
			}
			else
			{
				*dest++ = bank->Data[intPos++];
				intRemaining--;
				intLen++;
			}
		}
		else
		{
			*dest++ = bank->Data[intPos++];
			intRemaining--;
			intLen++;
		}
	}

	return intLen;
}

UWORD swapLsbMsb(UWORD word)
{
	UBYTE lsb, msb;

	lsb = word & 0xFF;
	msb = (word >> 8);

	return ((lsb << 8) | msb);
}

UBYTE checksumXor(UBYTE seed, UBYTE *ptr, UWORD len)
{
	UBYTE checksum;

	checksum = seed;
	while (len)
	{
		checksum ^= *ptr++;
		len--;
	}

	return checksum;
}

UBYTE checksumXor2(UBYTE seed, UBYTE *ptr, UWORD len)
{
	printf("Samples: ");
	for (int u = 0; u < 5; u++)
		printf("%u, ", (unsigned int)ptr[u]);

	printf("... ");
	for (int u = (len / 2); u < (len / 2) + 5; u++)
		printf("%u, ", (unsigned int)ptr[u]);

	printf("... ");
	for (int u = len - 5; u < len; u++)
		printf("%u, ", (unsigned int)ptr[u]);

	return checksumXor(seed, ptr, len);
}
