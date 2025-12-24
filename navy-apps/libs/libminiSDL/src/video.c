#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 辅助宏：构建 32位颜色 (00RRGGBB)
#define OFF_COLOR(r, g, b) ((((uint32_t)(r)) << 16) | (((uint32_t)(g)) << 8) | ((uint32_t)(b)))

void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(dst && src);

  // 1. 确定源矩形
  int s_x = (srcrect == NULL) ? 0 : srcrect->x;
  int s_y = (srcrect == NULL) ? 0 : srcrect->y;
  int w   = (srcrect == NULL) ? src->w : srcrect->w;
  int h   = (srcrect == NULL) ? src->h : srcrect->h;

  // 2. 确定目标矩形
  int d_x = (dstrect == NULL) ? 0 : dstrect->x;
  int d_y = (dstrect == NULL) ? 0 : dstrect->y;

  // 3. 裁剪 (Clipping)
  if (d_x < 0) { s_x -= d_x; w += d_x; d_x = 0; }
  if (d_y < 0) { s_y -= d_y; h += d_y; d_y = 0; }
  if (d_x + w > dst->w) w = dst->w - d_x;
  if (d_y + h > dst->h) h = dst->h - d_y;

  if (w <= 0 || h <= 0) return;

  // 4. 核心拷贝逻辑
  // Case 1: 32位 -> 32位 (真彩色图片)
  if (src->format->BitsPerPixel == 32 && dst->format->BitsPerPixel == 32) {
    uint32_t *src_pixels = (uint32_t *)src->pixels;
    uint32_t *dst_pixels = (uint32_t *)dst->pixels;
    for (int i = 0; i < h; i++) {
      memcpy(&dst_pixels[(d_y + i) * dst->w + d_x], 
             &src_pixels[(s_y + i) * src->w + s_x], 
             w * 4);
    }
  } 
  // Case 2: 8位 -> 32位 (字体渲染、PAL)
  else if (src->format->BitsPerPixel == 8 && dst->format->BitsPerPixel == 32) {
    uint8_t *src_pixels = (uint8_t *)src->pixels;
    uint32_t *dst_pixels = (uint32_t *)dst->pixels;
    SDL_Color *palette = src->format->palette->colors;

    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        uint8_t index = src_pixels[(s_y + i) * src->w + (s_x + j)];
        // 索引 0 为透明色
        if (index != 0) {
          SDL_Color color = palette[index];
          dst_pixels[(d_y + i) * dst->w + (d_x + j)] = OFF_COLOR(color.r, color.g, color.b);
        }
      }
    }
  }
}

void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  assert(dst);

  int x, y, w, h;
  if (dstrect == NULL) {
    x = 0; y = 0; w = dst->w; h = dst->h;
  } else {
    x = dstrect->x; y = dstrect->y; w = dstrect->w; h = dstrect->h;
  }

  // 裁剪
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x >= dst->w || y >= dst->h) return;
  if (x + w > dst->w) w = dst->w - x;
  if (y + h > dst->h) h = dst->h - y;
  
  if (w <= 0 || h <= 0) return;

  // 填充逻辑
  if (dst->format->BitsPerPixel == 32) {
    uint32_t *pixels = (uint32_t *)dst->pixels;
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        pixels[(y + i) * dst->w + (x + j)] = color;
      }
    }
  } else if (dst->format->BitsPerPixel == 8) {
    uint8_t *pixels = (uint8_t *)dst->pixels;
    for (int i = 0; i < h; i++) {
      memset(&pixels[(y + i) * dst->w + x], (uint8_t)color, w);
    }
  }
}

static uint32_t static_draw_buf[400 * 300];

void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  assert(s);
  if (w == 0 && h == 0) { w = s->w; h = s->h; }

  // [修改] 不再使用 malloc，使用预分配的静态缓冲区
  uint32_t *buf = static_draw_buf;
 if (w > 0 && h > 0) {
      if (s->format->BitsPerPixel == 8) {
          // 检查中间的一个像素
          int mid_idx = (h/2) * s->w + (w/2);
          uint8_t pixel = ((uint8_t*)s->pixels)[mid_idx];
          SDL_Color c = s->format->palette->colors[pixel];
          
          printf("UpdateRect(8bpp): w=%d, h=%d, mid_pixel_idx=%d, palette_color=(%d,%d,%d)\n", 
                 w, h, pixel, c.r, c.g, c.b);
      } else {
          printf("UpdateRect(32bpp): w=%d, h=%d\n", w, h);
      }
  }
  // 简单的越界保护
  if (w * h > 400 * 300) {
    printf("SDL_UpdateRect: Rect too large! %dx%d\n", w, h);
    return;
  }

  if (s->format->BitsPerPixel == 32) {
    uint32_t *src = (uint32_t *)s->pixels;
    for (int i = 0; i < h; i++) {
      memcpy(&buf[i * w], &src[(y + i) * s->w + x], w * 4);
    }
  } else if (s->format->BitsPerPixel == 8) {
    uint8_t *src = (uint8_t *)s->pixels;
    SDL_Color *palette = s->format->palette->colors;
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        uint8_t index = src[(y + i) * s->w + (x + j)];
        SDL_Color color = palette[index];
        buf[i * w + j] = OFF_COLOR(color.r, color.g, color.b);
      }
    }
  }

  NDL_DrawRect(buf, x, y, w, h);
  // [修改] 不需要 free(buf)
}

// === 下方为已有的辅助函数，保持原样 ===

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
    case 0x000000ff: return 0;
    case 0x0000ff00: return 8;
    case 0x00ff0000: return 16;
    case 0xff000000: return 24;
    case 0x00000000: return 24; 
    default: assert(0);
  }
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
    uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask; s->format->Rshift = maskToShift(Rmask); s->format->Rloss = 0;
    s->format->Gmask = Gmask; s->format->Gshift = maskToShift(Gmask); s->format->Gloss = 0;
    s->format->Bmask = Bmask; s->format->Bshift = maskToShift(Bmask); s->format->Bloss = 0;
    s->format->Amask = Amask; s->format->Ashift = maskToShift(Amask); s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
    memset(s->pixels, 0, s->pitch * height);
  }

  return s;
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
    int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
      Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL) free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s);
  }
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE) NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
      DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if(w == dstrect->w && h == dstrect->h) {
    SDL_Rect rect; rect.x = x; rect.y = y; rect.w = w; rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  }
  else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);
  printf("SDL_SetPalette: ncolors=%d, first_color_rgb=(%d,%d,%d)\n", 
         ncolors, colors[0].r, colors[0].g, colors[0].b);

  s->format->palette->ncolors = ncolors;
  //memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);
  for (int i = 0; i < ncolors; i++) {
      // 仙剑的颜色是 0-63，我们需要把它拉伸到 0-255
      // 简单的做法是 左移 2 位 (*4)
      s->format->palette->colors[i].r = colors[i].r << 2;
      s->format->palette->colors[i].g = colors[i].g << 2;
      s->format->palette->colors[i].b = colors[i].b << 2;
      s->format->palette->colors[i].a = colors[i].a;
  }

  if(s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t (*pdst)[4] = dst;
  uint8_t (*psrc)[4] = src;
  union { uint8_t val8[4]; uint32_t val32; } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i) \
    tmp.val32 = *((uint32_t *)psrc[i]); \
    *((uint32_t *)pdst[i]) = tmp.val32; \
    pdst[i][0] = tmp.val8[2]; \
    pdst[i][2] = tmp.val8[0];

    macro(i + 0); macro(i + 1); macro(i + 2); macro(i + 3);
    macro(i + 4); macro(i + 5); macro(i + 6); macro(i + 7);
    macro(i + 8); macro(i + 9); macro(i +10); macro(i +11);
    macro(i +12); macro(i +13); macro(i +14); macro(i +15);
  }
  for (; i < len; i ++) { macro(i); }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface* ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
    fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask) p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) { return 0; }
void SDL_UnlockSurface(SDL_Surface *s) {}