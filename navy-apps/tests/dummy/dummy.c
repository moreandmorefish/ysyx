#ifdef __ISA_NATIVE__
#error can not support ISA=native
#endif

#define SYS_yield 1


#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  // printf("Dummy Main: argc = %d\n", argc);

  // for (int i = 0; i < argc; i++) {
  //   printf("Dummy Main: argv[%d] = %s\n", i, argv[i]);
  // }

  // return 0;
  _exit(42);
  return 0; // 不会执行到这里
}