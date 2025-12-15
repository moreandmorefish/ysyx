#include <stdio.h>
#include <NDL.h>

int main() {
  NDL_Init(0);
  printf("Timer Test Start...\n");

  uint32_t start = NDL_GetTicks(); // 获取初始时间

  while (1) {
    uint32_t now = NDL_GetTicks();
    if (now - start >= 10) { // 间隔 500ms (0.5秒)
      printf("Hello from timer-test at %d ms\n", now);
      start = now; // 更新上一次打印的时间
    }
  }
  return 0;
}