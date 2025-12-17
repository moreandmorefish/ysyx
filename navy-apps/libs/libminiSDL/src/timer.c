#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  return 1;
}

// [修改] 获取系统启动后的毫秒数
uint32_t SDL_GetTicks() {
  return NDL_GetTicks();
}

// [修改] 延迟指定的毫秒数
void SDL_Delay(uint32_t ms) {
  uint32_t start = NDL_GetTicks();
  while (NDL_GetTicks() - start < ms) {
    // 等待...
    // 如果支持多任务，这里应该调用 yield()
  }
}