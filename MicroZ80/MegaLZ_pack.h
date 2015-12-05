
struct packinfo // contains various info for every byte of input file
{
	struct lzcode best;  // best LZ code for jumping to this pos. length=0 - OUTBYTE type

	ULONG price; // price - for finding optimal LZ codes chain
};

ULONG pack(void); // main function that pack data (generate packed symbols)

void make_LZ_codes(ULONG);

ULONG gen_output(void);

void update_price(ULONG,ULONG,struct lzcode *);
