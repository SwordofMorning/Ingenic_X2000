#include <stdarg.h>
#include <stdio.h>
#include <driver/console.h>

static int hex2asc(int n)
{
    n &= 15;
    if(n > 9){
        return ('a' - 10) + n;
    } else {
        return '0' + n;
    }
}

int simple_vsprintf(char *str,const char *fmt, va_list ap)
{
    char scratch[16];

    for(;;){
        switch(*fmt){
            case 0:
                *str++ = 0;
                return 0;
            case '%':
                switch(fmt[1]) {
                    case 'p':
                    case 'X':
                    case 'x': {
                        unsigned n = va_arg(ap, unsigned);
                        char *p = scratch + 15;
                        *p = 0;
                        do {
                            *--p = hex2asc(n);
                            n = n >> 4;
                        } while(n != 0);
                        while(p > (scratch + 7)) *--p = '0';
                        while (*p) *str++ = *p++;
                        fmt += 2;
                        continue;
                    }
                    case 'd': {
                        int n = va_arg(ap, int);
                        char *p = scratch + 15;
                        *p = 0;
                        if(n < 0) {
                            *str++ = ('-');
                            n = -n;
                        }
                        do {
                            *--p = (n % 10) + '0';
                            n /= 10;
                        } while(n != 0);
                        while (*p) *str++ = (*p++);
                        fmt += 2;
                        continue;
                    }
                    case 's': {
                        char *s = va_arg(ap, char*);
                        if(s == 0) s = "(null)";
                        while (*s) *str++ = (*s++);
                        fmt += 2;
                        continue;
                    }
                }
                *str++ = (*fmt++);
                break;
            case '\n':
                *str++ = ('\r');
            default:
                *str++ = (*fmt++);
        }
    }
}

int putchar(int c)
{
    char s[2] = {(unsigned char)c, 0};

    console_puts(s);

    return (unsigned char)c;
}

int puts(const char *s)
{
    console_puts(s);
    return 0;
}

int printf(const char *fmt, ...)
{
    char buf[256];
    char *p = buf;

    buf[sizeof(buf) - 1] = 0xa5;

    va_list args;
    va_start(args, fmt);
    simple_vsprintf(buf, fmt, args);
    va_end(args);

    // if (buf[sizeof(buf) - 1] != 0xa5)
    //     puts("printf stack is outof size! please check\n");
    // else
        puts(p);

    return 0;
}
