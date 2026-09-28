#include <stdio.h>
#include <driver/clk.h>

void clk_init(void)
{

}

int clk_enable(enum clk_type id, int on)
{
    printf("clk_enable is deprecated\n");
    return -1;
}