/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>


static int is_batch_mode = false;


void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;  // 标记为“正常退出”
  return -1;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  uint64_t n = 1;
  if (arg != NULL) {
    if(arg[0] == '-') {
      printf("Invalid argument: %s\n", arg);
      return 0;
    }
    bool success = true;
    n = expr(arg, &success);

    if (!success) {
      printf("Invalid expression: %s\n", arg);
      return 0;
    }
  }

  cpu_exec(n);
  return 0;
}


/*static int cmd_x(char *args) {
  char *arg1 = strtok(NULL, " ");
  if (arg1 == NULL) {
    printf("Invalid argument\n");
    return 0;
  }
  char *arg2 = strtok(NULL, " ");
  if (arg2 == NULL) {
    printf("Invalid argument\n");
    return 0;
  }
  uint64_t N = 0;
  if (sscanf(arg1, "%lu", &N) != 1 || N <= 0) {
    printf("Invalid number \"%s\".\n", arg1);
    return 0;
  }

  bool success = true;
  word_t EXPR = expr(arg2, &success);
  if(!success) {
    printf("Invalid expression: %s\n", arg2);
    return 0;
  }

  for(uint64_t i = 0; i < N; ++i) {
    word_t data = vaddr_read(EXPR + i * 4, 4);
    printf("0x%08x: 0x%08x\n", EXPR + i * 4, data);
  }
  return 0;  
}*/
word_t vaddr_read(vaddr_t, int);

static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Argument required (starting display address).\n");
    return 0;
  }
  int64_t n = 1;
  char *expr = strtok(NULL, " ");
  if (expr != NULL) {
    n = atol(arg);  // n 为负数，向后打印
  } else {
    expr = arg;
  }
  if (expr[0] == '0' && (expr[1] == 'x' || expr[1] == 'X')) {
    expr += 2;
  } else {
    printf ("Invalid number \"%s\".\n", expr);
    return 0;
  }
  size_t len = strlen(expr);
  uint64_t address = 0;
  for (int i = 0; i < len; ++i) {
    address <<= 4;
    if (isdigit(expr[i])) {
      address += (expr[i] - '0');
    } else if (expr[i] >= 'a' && expr[i] <= 'f') {
      address += (expr[i] - 'a' + 10);
    } else if (expr[i] >= 'A' && expr[i] <= 'F') {
      address += (expr[i] - 'A' + 10);
    } else {
      printf ("Invalid number \"%s\".\n", expr-2);
      return 0;
    }
  }
  // 读取数据并打印
  int direct = n > 0 ? 4 : -4;    // 地址增加的方向
  n = n > 0 ? n : -n;
  for ( ; n > 0; --n) {
    word_t ret = vaddr_read(address, 4);  // paddr_read已经做了地址合法性的检查
    printf(ANSI_FMT("0x%lx: ", ANSI_FG_BLUE), address);
    printf("0x%08x\n",ret);   // 4字节，16进制就有8个字符，右对其，高位补0
    address += direct;
  }
  return 0;
}

static int cmd_p(char* args) {
  bool success;
  word_t res = expr(args, &success);
  if (!success) {
    puts("invalid expression");
  } else {
    printf("%u\n", res);
  }
  return 0;
}






static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: info r (registers) or info w (watchpoints)\n");
  } else {
    if (strcmp(arg, "r") == 0) {
      isa_reg_display();
    } else if (strcmp(arg, "w") == 0) {
      wp_iterate();
    } else {
      printf("Usage: info r (registers) or info w (watchpoints)\n");
    }
  }
  
  return 0;
}

static int cmd_w(char* args) {
  if (!args) {
    printf("Usage: w EXPR\n");
    return 0;
  }
  bool success;
  word_t res = expr(args, &success);
  if (!success) {
    puts("invalid expression");
  } else {
    wp_watch(args, res);
  }
  return 0;
}

static int cmd_d(char* args) {
  char *arg = strtok(NULL, "");
  if (!arg) {
    printf("Usage: d N\n");
    return 0;
  }
  int no = strtol(arg, NULL, 10);
  wp_remove(no);
  return 0;
}



static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "step instruction", cmd_si },
  { "info", "info r/w", cmd_info },
  { "x",    "x N EXPR : scan memory from EXPR for N bytes", cmd_x},
  {"p", "Usage: p EXPR. Calculate the expression, e.g. p $eax + 1", cmd_p },
  { "w", "Usage: w EXPR. Watch for the variation of the result of EXPR, pause at variation point", cmd_w },
  { "d", "Usage: d N. Delete watchpoint of wp.NO=N", cmd_d },

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void test_expr() {
  FILE *fp = fopen("/home/fish/ics2024/nemu/tools/gen-expr/input", "w+r");
  if (fp == NULL) {
    // 打印具体错误原因（需要包含 <stdio.h>）
    perror("Failed to open test input file");  // 会显示如 "No such file or directory"
    // 优雅退出测试，避免后续崩溃
    printf("Skip expression test due to file error\n");
    return;  // 若在函数中，直接返回；若在主逻辑中，可使用 exit(0)
  }

  char *e = NULL;
  word_t correct_res;
  size_t len = 0;
  ssize_t read;
  bool success = false;

  while (true) {
    if(fscanf(fp, "%u ", &correct_res) == -1) break;
    read = getline(&e, &len, fp);
    e[read-1] = '\0';
    
    word_t res = expr(e, &success);
    
    assert(success);
    if (res != correct_res) {
      puts(e);
      printf("expected: %u, got: %u\n", correct_res, res);
      assert(0);
    }
  }

  fclose(fp);
  if (e) free(e);

  Log("expr test pass");
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  /* test math expression calculation */
  test_expr();
  /* Initialize the watchpoint pool. */
  init_wp_pool();
}



