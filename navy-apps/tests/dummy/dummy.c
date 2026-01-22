#ifdef __ISA_NATIVE__
#error can not support ISA=native
#endif

#define SYS_yield 1


#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
   printf("Dummy Main: argc = %d\n", argc);
   int j = 0;
  while(1)
  {
    printf("Dummy Running... %d\n", j++);
  }
  return 0; // 不会执行到这里
}