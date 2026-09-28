#include <stdio.h>
#include <driver/console.h>
#include "xformatc.h"

int putchar(int c)
{
    char s[2] = {(unsigned char)c, 0};

    console_puts(s);

    return (unsigned char)c;
}

int puts(const char *s)
{
    console_puts(s);
    console_puts("\n");
    return 0;
}

static void m_printf_putchar(void *arg,char c)
{
    (void) arg;

    char buf[2] = {c, 0};

    console_puts(buf);
}

int printf(const char *__restrict fmt, ...)
{
    va_list list;
    unsigned int count;

    va_start(list, fmt);
    count = xvformat(m_printf_putchar, 0, fmt, list);
    va_end(list);

    return count;
}

static void m_sprintf_putchar(void *arg,char c)
{
    char *sp = *(char **)arg;

    *sp = c;
    (*(char **)arg)++;
}

int sprintf(char *__restrict s, const char *__restrict fmt, ...)
{
    va_list list;
    unsigned count;

    va_start(list, fmt);
    count = xvformat(m_sprintf_putchar, s, fmt, list);
    va_end(list);

    s[count] = 0;

    return count;
}

struct snprintf_t {
    size_t n;
    size_t index;
    char *s;
};

static void m_snprintf_putchar(void *arg,char c)
{
    struct snprintf_t *sp = *(struct snprintf_t **)arg;

    if (sp->index >= sp->n)
        return;

    sp->index++;

    *(sp->s++) = c;
}

int snprintf(char *__restrict s, size_t n, const char *__restrict fmt, ...)
{
    va_list list;
    unsigned count;
    struct snprintf_t sp;

    sp.s = s;
    sp.n = n;
    sp.index = 0;

    va_start(list, fmt);
    count = xvformat(m_snprintf_putchar, &sp, fmt, list);
    va_end(list);

    s[count] = 0;

    return count;
}
