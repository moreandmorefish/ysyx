#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#include <stdarg.h>
#include <limits.h> 

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
int itoa(int value, char *str, int base); 
int vsprintf(char *out, const char *fmt, va_list ap);

int printf(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int ans = vsprintf(buffer, fmt, args);
    for(int i = 0; i < ans; i++)
        putch(buffer[i]);
    va_end(args);
    return ans;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    char *buffer = out;
    const char *p = fmt;
    const int MAX_LEN = 1023;

    while (*p && (buffer - out) < MAX_LEN - 1) {
        if (*p != '%') {
            *buffer++ = *p++;
            continue;
        }

        p++; // skip '%'
        switch (*p) {
            case 'd': {
                int val = va_arg(ap, int);
                buffer += itoa(val, buffer, 10);
                break;
            }
            case 'x': {
                unsigned int val = va_arg(ap, unsigned int);
                buffer += itoa(val, buffer, 16);
                break;
            }
            case 's': {
                char *str = va_arg(ap, char *);
                if (!str) str = "(null)";
                while (*str && (buffer - out) < MAX_LEN - 1)
                    *buffer++ = *str++;
                break;
            }
            case 'c': {
                int ch = va_arg(ap, int);
                *buffer++ = (char)ch;
                break;
            }
            default:
                *buffer++ = '%';
                if (*p && (buffer - out) < MAX_LEN - 1)
                    *buffer++ = *p;
                break;
        }
        p++;
    }

    *buffer = '\0';
    return buffer - out;
}


int sprintf(char *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ans = vsprintf(out, fmt, args);
    va_end(args);
    return ans;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  char *buffer = out;
    const char *buf_fmt = fmt;
    va_list args;

    va_start(args, fmt);

    while (*buf_fmt != '\0') {
        // 溢出检查：限制最大写入长度 n（留 1 字节给 '\0'）
        if (buffer - out >= n - 1) {
            break;
        }

        if (*buf_fmt != '%') {
            // 普通字符：直接拷贝
            *buffer++ = *buf_fmt++;
        } else {
            buf_fmt++;  // 跳过 '%'
            switch (*buf_fmt) {
                case 'd': {
                    int val = va_arg(args, int);
                    // 调用修复后的 itoa，不会无限循环
                    char tmp_buf[12]; // 临时缓冲区存储整数转换结果
                    int len = itoa(val, tmp_buf, 10);
                    // 将 tmp_buf 内容复制到 buffer，注意长度限制
                    for(int i = 0; i < len && (buffer - out) < n - 1; i++) {
                        *buffer++ = tmp_buf[i];
                    }
                    buf_fmt++;
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char *);
                    // 处理空指针（避免访问 NULL）
                    if (str == NULL) {
                        str = "(null)";
                    }
                    // 限制最大写入长度（避免无限循环）
                    int max_len = n - (buffer - out) - 1;
                    while (*str != '\0' && max_len > 0) {
                        *buffer++ = *str++;
                        max_len--;
                    }
                    buf_fmt++;
                    break;
                }
                default:
                    // 未知格式符：拷贝 '%' 和当前字符
                    *buffer++ = '%';
                    if ((buffer - out) < n - 1) *buffer++ = *buf_fmt++;
                    break;
            }
        }
    }

    // 正确添加字符串结束符
    *buffer = '\0';

    va_end(args);

    // 返回写入的字符数（不含 '\0'）
    return buffer - out;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int itoa(int value, char *str, int base) {
    // 1. 检查基数合法性
    if (base < 2 || base > 32) {
        *str = '\0';
        return 0;
    }

    char *start = str;  // 记录字符串起始位置（用于反转）
    char digits[] = "0123456789abcdefghijklmnopqrstuv";
    int is_negative = 0;

    // 2. 处理 INT_MIN（用 unsigned int 避免溢出）
    unsigned int num;
    if (value < 0 && base == 10) {
        is_negative = 1;
        num = (unsigned int)(-value);  // 无符号数范围更大，不会溢出
    } else {
        num = (unsigned int)value;  // 正数直接转为无符号数
    }

    // 3. 处理 0 的特殊情况
    if (num == 0) {
        *str++ = '0';
        *str = '\0';
        return 1;
    }

    // 4. 逆序存储数字的 ASCII 字符（比如 -123 → 先存 '3' '2' '1'）
    while (num > 0) {
        int rem = num % base;
        *str++ = digits[rem];
        num /= base;
    }

    // 5. 添加负号（若需要）
    if (is_negative) {
        *str++ = '-';
    }

    // 6. 补充字符串结束符（此时 str 指向最后一个字符的下一位）
    *str = '\0';

    // 7. 反转字符串（数字+负号部分）
    char *left = start;
    char *right = str - 1;  // 指向最后一个字符（负号或最后一个数字）
    while (left < right) {
        char tmp = *left;
        *left = *right;
        *right = tmp;
        left++;
        right--;
    }

    // 返回字符串长度（含负号，不含 '\0'）
    return str - start;
}

#endif
