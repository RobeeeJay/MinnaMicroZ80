// Set these for pack
extern ULONG inlen; //length of input file
extern UBYTE * indata; // input data, loaded into mem

extern ULONG mode; // mode of working:
#define MODE_PACK_OPTIMAL 1
#define MODE_PACK_GREEDY  2
#define MODE_DEPACK       3

// Read these after pack
#define OUBUFSIZE 131072
extern UBYTE oubuf[OUBUFSIZE];
extern ULONG ob_freepos;