#ifdef __ISA_NATIVE__
#error can not support ISA=native
#endif

#define SYS_yield 1


#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
   printf("Dummy Main: argc = %d\n", argc);
   int i,j = 0;
  while(1)
  {
    j ++;
    if (j == 10000) {
      printf("dummydummydummy for the %dth time!\n", i ++);
      j = 0;
    }
  }
  return 0; // 不会执行到这里
}