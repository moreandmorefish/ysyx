#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  // 1. 去掉换行符 (如果 cmd 末尾有 '\n'，把它变成 '\0')
  char *cmd_end = (char *)cmd + strlen(cmd);
  if (cmd_end > cmd && *(cmd_end - 1) == '\n') {
    *(cmd_end - 1) = '\0';
  }

  // 2. 如果是空命令，直接返回
  if (strlen(cmd) == 0) return;

  // 3. 解析参数 (简单的 strtok 切割)
  // 注意：cmd 是 const char*，我们需要复制一份来切割
  char buf[128];
  strcpy(buf, cmd);
  
  char *argv[16]; // 最多支持 16 个参数
  int argc = 0;
  
  char *token = strtok(buf, " ");
  while (token != NULL && argc < 15) {
    argv[argc++] = token;
    token = strtok(NULL, " ");
  }
  argv[argc] = NULL; // 参数列表以 NULL 结尾

  // 4. 处理内置命令 (可选，比如 echo)
  if (strcmp(argv[0], "exit") == 0) {
    exit(0);
  }
  else if (strcmp(argv[0], "echo") == 0) {
    for (int i = 1; i < argc; i++) {
      sh_printf("%s ", argv[i]);
    }
    sh_printf("\n");
    return;
  }

  // 5. 执行外部程序
  // 使用 execvp，它会自动去 PATH 环境变量指定的目录下寻找 argv[0]
  execvp(argv[0], argv);

  // 6. 如果 execvp 返回了，说明执行失败
  sh_printf("Unknown command '%s'\n", argv[0]);
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}