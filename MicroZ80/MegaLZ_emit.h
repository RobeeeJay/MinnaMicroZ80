// output buffer size
#define OUBUFSIZE 131072

ULONG emit_init(void);
ULONG emit_code(struct lzinfo *);
ULONG emit_finish(void);

void emit_bits(ULONG,ULONG);
