#include <common.h>
#include <stdio.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

// 串口写函数
// buf: 要写入的数据
// offset: 串口是字符设备，没有偏移量的概念，忽略它
// len: 写入长度
size_t serial_write(const void *buf, size_t offset, size_t len) {
  // 把 buf 中的 len 个字符通过 putch 输出
  const char *p = (const char *)buf;
  for (size_t i = 0; i < len; i++) {
    putch(p[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  // 1. 先定义变量
  AM_INPUT_KEYBRD_T ev;
  
  // 2. 将变量的地址 (&ev) 传给 ioe_read
  ioe_read(AM_INPUT_KEYBRD, &ev);

  // 3. 后续逻辑不变
  if (ev.keycode == AM_KEY_NONE) {
    return 0;
  }

  int ret = snprintf((char *)buf, len, "%s %s\n", 
                     ev.keydown ? "kd" : "ku", 
                     keyname[ev.keycode]);

  return ret;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
