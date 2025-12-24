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
static int canvas_w = 0, canvas_h = 0;
static int canvas_x = 0, canvas_y = 0; // [新增] 画布起始坐标

static int evt_fd = -1;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  // 调用 Newlib 的 gettimeofday，它会发起 SYS_gettimeofday 系统调用
  gettimeofday(&tv, NULL);
  static int count = 0;
  if (count++ % 1000 == 0) printf("Tick: %d\n", tv.tv_sec * 1000 + tv.tv_usec / 1000);
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
  // 1. 获取屏幕大小 (这一步必须最先做，或者保证 screen_w/h 有值)
  // 虽然 NWM_APP 里可能已经设了，但为了保险，总是去读 dispinfo 也没坏处
  // 或者你保留原来的逻辑结构，但要把 dispinfo 的读取提到前面
  
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    write(fbctl, buf, len);
    while (1) {
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  } else {
    // 非 NWM_APP 环境 (比如现在的 Nanos-lite)
    // 必须读取 /proc/dispinfo 获取屏幕大小
    int fd = open("/proc/dispinfo", O_RDONLY);
    char buf[64];
    read(fd, buf, sizeof(buf));
    close(fd);
    sscanf(buf, "WIDTH:%d\nHEIGHT:%d", &screen_w, &screen_h);
  }

  // 2. 确定画布大小
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }
  
  // [关键修复] 更新全局变量 canvas_w 和 canvas_h
  canvas_w = *w;
  canvas_h = *h;

  // [关键修复] 计算居中起始坐标 (无论何种环境都要计算)
  // 如果画布比屏幕大，坐标可能为负，但这在 bmp-test 里不会发生
  canvas_x = (screen_w - canvas_w) / 2;
  canvas_y = (screen_h - canvas_h) / 2;
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  int fd = open("/dev/fb", O_WRONLY);

  for (int i = 0; i < h; i++) {
    // [修改] 计算显存偏移量时，加上 canvas_x 和 canvas_y
    // 实际屏幕坐标 X = canvas_x + x
    // 实际屏幕坐标 Y = canvas_y + y
    // 显存 offset = (Y * screen_w + X) * 4
    int offset = ((canvas_y + y + i) * screen_w + (canvas_x + x)) * 4;
    
    lseek(fd, offset, SEEK_SET);
    write(fd, pixels + i * w, w * 4);
  }

  close(fd);
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
