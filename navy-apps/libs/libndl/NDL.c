#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;

static int evt_fd = -1;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  // 调用 Newlib 的 gettimeofday，它会发起 SYS_gettimeofday 系统调用
  gettimeofday(&tv, NULL);
  
  // 转换：秒*1000 + 微秒/1000 = 毫秒
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  // 1. 第一次调用时打开文件
  if (evt_fd == -1) {
    evt_fd = open("/dev/events", O_RDONLY);
    if (evt_fd == -1) return 0; // 打开失败
  }

  // 2. 尝试读取
  // 注意：因为 device.c 里 events_read 在没按键时返回 0
  // 所以这里如果读不到数据，read 会返回 0
  int ret = read(evt_fd, buf, len);

  // 3. 返回 1 表示读到了事件，0 表示没读到
  return (ret > 0) ? 1 : 0;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
