#include <common.h>

#define MAX_IRINGBUF 16

typedef struct {
  word_t pc;
  uint32_t inst;
} ItraceNode;

ItraceNode iringbuf[MAX_IRINGBUF];
int p_cur = 0;
bool full = false;

void trace_inst(word_t pc, uint32_t inst) {
  iringbuf[p_cur].pc = pc;
  iringbuf[p_cur].inst = inst;
  p_cur = (p_cur + 1) % MAX_IRINGBUF;
  full = full || p_cur == 0;
}

void display_inst() {
  if (!full && !p_cur) return;

  int end = p_cur;
  int i = full?p_cur:0;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  char buf[128]; // 128 should be enough!
  char *p;
  puts("Most recently executed instructions");
  do {
    p = buf;
    p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i+1)%MAX_IRINGBUF==end?" --> ":"     ", iringbuf[i].pc, iringbuf[i].inst);
    disassemble(p, buf+sizeof(buf)-p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

    if ((i+1)%MAX_IRINGBUF==end) printf(ANSI_FG_RED);
    puts(buf);
  } while ((i = (i+1)%MAX_IRINGBUF) != end);
  puts(ANSI_NONE);
}

char ringbuf[MAX_IRINGBUF][128];
int ringbuf_pcur = 0;
bool ringbuf_full = false;
void add_ringbuf(const char* logbuf) {
  p_cur = (p_cur + 1) % MAX_IRINGBUF;
  full = full || p_cur == 0;
  strcpy(ringbuf[p_cur], logbuf);
}
void display_ringbuf() {
  if (!ringbuf_full && !ringbuf_pcur) return;

  int end = ringbuf_pcur;
  int i = ringbuf_full?ringbuf_pcur:0;

  puts("Most recently executed instructions");
  do {
    printf("%s%s\n", (i+1)%MAX_IRINGBUF==end?" --> ":"     ", ringbuf[i]);

  } while ((i = (i+1)%MAX_IRINGBUF) != end);
}