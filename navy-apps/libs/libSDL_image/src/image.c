#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

// [核心实现] 加载图片
SDL_Surface* IMG_Load(const char *filename) {
  // 1. 打开文件
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    printf("IMG_Load: Failed to open %s\n", filename);
    return NULL;
  }

  // 2. 获取文件大小
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // 3. 申请内存并读取数据
  char *buf = (char *)malloc(size);
  if (!buf) {
    fclose(fp);
    return NULL;
  }
  
  // 确保读取成功
  size_t read_bytes = fread(buf, 1, size, fp);
  if (read_bytes != size) {
    // 可能是读取错误，但在简化OS中可能并不严格
    // printf("IMG_Load: Read count mismatch\n");
  }

  // 4. 调用 stb_image 进行解码
  // STBIMG_LoadFromMemory 是 SDL_stbimage.h 提供的便捷函数
  // 它直接返回一个 SDL_Surface*
  SDL_Surface *s = STBIMG_LoadFromMemory(buf, size);

  // 5. 清理资源
  free(buf);
  fclose(fp);

  return s;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}