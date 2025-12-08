#include <am.h>

void __am_timer_init() {
}
#define RTC_ADDR     0xa0000048
void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 1. 读取高 32 位和低 32 位
  // 注意：使用 volatile 确保编译器真的去访问内存，而不是读取缓存或优化掉
  volatile uint32_t *rtc_addr = (volatile uint32_t *)RTC_ADDR;
  
  // 你的 ram_dpi.c 中逻辑是：
  // 0xa0000048 -> 低 32 位 (RTC_ADDR_L)
  // 0xa000004c -> 高 32 位 (RTC_ADDR_H)
  // 在指针运算中，(uint32_t *) + 1 会自动增加 4 字节
  
  uint32_t low  = rtc_addr[0];
  uint32_t high = rtc_addr[1];

  // 2. 拼接成 64 位时间 (微秒)
  // 注意：必须先将 high 强转为 uint64_t，否则移位操作可能会溢出或被截断
  uptime->us = ((uint64_t)high << 32) | low;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
