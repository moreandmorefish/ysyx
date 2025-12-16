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

// 格式输出: "WIDTH:400\nHEIGHT:300"
size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  // 读取 AM 显卡配置
  AM_GPU_CONFIG_T cfg;
  ioe_read(AM_GPU_CONFIG, &cfg); // 注意传地址
  
  // 格式化屏幕大小信息
  // 显卡信息是静态的，offset 只有在第一次读取时有用，
  // 但为了简化，我们假设用户一次读完，或者我们每次都从头写
  // (严谨的做法是配合 offset，但这里简化处理)
  int ret = snprintf((char *)buf, len, "WIDTH:%d\nHEIGHT:%d\n", 
                     cfg.width, cfg.height);
  return ret;
}

// nanos-lite/src/device.c

size_t fb_write(const void *buf, size_t offset, size_t len) {
  // 1. 获取屏幕宽度 (用于计算坐标)
  AM_GPU_CONFIG_T cfg;
  ioe_read(AM_GPU_CONFIG, &cfg);
  
  // 2. 转换坐标
  // 显存中每 4 字节代表一个像素
  int offset_pixel = offset / 4;
  int x = offset_pixel % cfg.width;
  int y = offset_pixel / cfg.width;
  
  // 写入的像素数量
  int len_pixel = len / 4;

  // 3. [修正] 构造绘图结构体
  // 你的 AM 版本要求通过结构体传递参数
  AM_GPU_FBDRAW_T ctl;
  ctl.x = x;
  ctl.y = y;
  ctl.pixels = (void *)buf; // 像素数据指针
  ctl.w = len_pixel;        // 宽度 (写入的像素数)
  ctl.h = 1;                // 高度 (默认为 1 行)
  ctl.sync = true;          // 立即同步到屏幕

  // 4. [修正] 传递结构体地址
  ioe_write(AM_GPU_FBDRAW, &ctl);

  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
