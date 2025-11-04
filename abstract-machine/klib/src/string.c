#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  panic("Not implemented");
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
  panic("Not implemented");
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
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  unsigned char* ptr = (unsigned char*)s;
  for (size_t i = 0; i < n; i++) {
    ptr[i] = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
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
