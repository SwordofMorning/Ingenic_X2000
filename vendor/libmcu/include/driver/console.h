#ifndef _DRIVER_CONSOLE_H_
#define _DRIVER_CONSOLE_H_

void console_puts(const char *str);

void console_put_char(char ch);

char console_get_char(void);

void console_init(void);

#endif /* _DRIVER_CONSOLE_H_ */
