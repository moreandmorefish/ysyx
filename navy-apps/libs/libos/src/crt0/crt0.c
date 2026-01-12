#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;

// [修复] 参数改回 uintptr_t *args (指向栈顶)
void call_main(uintptr_t *args) {
  // 栈布局: [argc] [argv0] [argv1] ... [NULL] [envp0] ...
  // args[0] 就是 argc
  int argc = (int)args[0];
  
  // args[1] 开始是 argv 数组
  char **argv = (char **)(args + 1);
  
  // envp 在 argv 数组的 NULL 后面
  // 因为我们现在 Loader 填的全是 0，所以 argc=0，argv[0]=NULL
  // args + 0 + 2 指向的位置就是 envp 的开始
  char **envp = (char **)(args + argc + 2);

  // 设置全局 environ 变量
  // 即使 envp 指向的是一堆 0，它也是一个有效的栈地址，不是 NULL！
  // 这样 libc 就不会崩了。
  environ = envp;

  exit(main(argc, argv, envp));
  assert(0);
}