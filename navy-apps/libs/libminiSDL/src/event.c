#include <NDL.h>
#include <SDL.h>
#include <string.h>
#include <stdlib.h> // for malloc/free if needed

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

// 这是一个辅助函数，用来把按键名称 (字符串) 转换成 SDL 的 KeyCode
// 因为 NDL 返回给我们的是 "kd RETURN" 这样的字符串
static uint8_t key_state[sizeof(keyname) / sizeof(keyname[0])] = {0};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  char buf[64];
  
  // 1. 调用 NDL 询问是否有事件
  if (NDL_PollEvent(buf, sizeof(buf))) {
    // 读到了数据，例如 "kd RETURN\n" 或 "ku A\n"
    
    // 2. 解析字符串
    char type[4];
    char key_str[32];
    sscanf(buf, "%s %s", type, key_str);
    
    // 3. 填充 SDL_Event type
    if (strcmp(type, "kd") == 0) {
      ev->type = SDL_KEYDOWN;
    } else if (strcmp(type, "ku") == 0) {
      ev->type = SDL_KEYUP;
    } else {
      // 应该是不会发生的
      return 0; 
    }

    // 4. 查找并填充 KeyCode
    // 我们遍历 keyname 数组，找到匹配的名字，它的下标就是 keycode
    // 注意：这里的 keyname 数组来源于 NDL/AM 的定义，通常和 SDL 的定义是一致的
    // 为了简单，我们假设 keyname 数组的顺序就是 keycode 的顺序
    
    // 这里的 keyname 数组需要和 AM 中的定义一致。
    // 为了省事，我们可以简单暴力的遍历。
    // 在真正的 SDL 中这里会查表。
    
    for (int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
      if (strcmp(key_str, keyname[i]) == 0) {
        ev->key.keysym.sym = i; // 这里的 i 对应 AM_KEY_xxx
        
        // 维护按键状态数组 (虽然 NSlider 可能不用，但很多游戏需要)
        key_state[i] = (ev->type == SDL_KEYDOWN) ? 1 : 0;
        break;
      }
    }
    
    return 1; // 成功获取一个事件
  }
  
  return 0; // 没有事件
}

int SDL_WaitEvent(SDL_Event *event) {
  // 简单粗暴的实现：死循环轮询
  // 只要没读到事件，就一直读
  while (SDL_PollEvent(event) == 0) {
    // 可以在这里插入 yield() 来让出 CPU，避免由死循环导致的卡顿
    // 但目前是在单任务环境，不加也行
  }
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  // 返回键盘状态数组
  // 很多游戏通过这个函数判断某个键是否一直被按着
  if (numkeys) *numkeys = sizeof(key_state);
  return key_state;
}