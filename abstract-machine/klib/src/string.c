#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  // 遍历字符串，直到遇到终止符 '\0'
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  char* original_dst = dst;
  while (*src != '\0') {
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0';
  return original_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  // 第一步：拷贝 src 中的字符（直到 src 结束或拷贝满 n 个）
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  // 第二步：若 src 长度 < n，剩余部分用 '\0' 填充
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  char* original_dst = dst;
  while (*dst != '\0') {
    dst++;
  }
  while (*src != '\0') {
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0';
  return original_dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s2 != '\0') {
    if (*s1 != *s2) {
      // 找到不同字符，返回 ASCII 码差值（正数/负数）
      return (unsigned char)*s1 - (unsigned char)*s2;
    }
    // 字符相同，继续遍历下一个
    s1++;
    s2++;
  }
  // 返回剩余字符的 ASCII 码差值（都到尾则返回 0）
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
      // 用 unsigned char 确保 ASCII 码无符号比较（避免负数字符判断错误）
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char* ptr = (unsigned char*)s;
  for (size_t i = 0; i < n; i++) {
    ptr[i] = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  if (d == s) { // 源地址和目标地址相同，直接返回
    return dst;
  }

  // 没有重叠部分，可以直接从前往后拷贝
  if (d < s || d >= s + n) {
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else { // 情况2：dst 在 src 后面（重叠）→ 从后往前拷贝
    for (size_t i = n; i > 0; i--) {
      d[i-1] = s[i-1];
    }
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *dst = (unsigned char *)out;
  const unsigned char *src = (const unsigned char *)in;

  // 直接复制就行
  for (size_t i = 0; i < n; i++) {
    dst[i] = src[i];
  }

  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  //panic("Not implemented");
  int ans = 0;
  const unsigned char* p1 = (const unsigned char*)s1;
  const unsigned char* p2 = (const unsigned char*)s2;
  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      ans = p1[i] - p2[i];
      break; 
    }
  }
  return ans;
}

#endif
