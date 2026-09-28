#ifndef _DRIVER_CLK_H_
#define _DRIVER_CLK_H_

#include <soc/clk.h>

void clk_init(void);

int clk_enable(enum clk_type id, int on);

#endif /* _DRIVER_CLK_H_ */
