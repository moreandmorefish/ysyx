#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
// void call_main(uintptr_t *args) {
//   char *empty[] =  {NULL };
//   environ = empty;
//   exit(main(0, empty, empty));
//   assert(0);
// }

// [修复] 修改函数签名，匹配 loader 传入的寄存器 a0(argc), a1(argv)
// 注意：start.S 是直接 jal call_main，所以寄存器值会直接透传过来
void call_main(int argc, char *argv[], char *envp[]) {
  environ = envp;
  
  // [修复] 将接收到的真实参数传递给 main
  exit(main(argc, argv, envp));
  
  assert(0);
}
