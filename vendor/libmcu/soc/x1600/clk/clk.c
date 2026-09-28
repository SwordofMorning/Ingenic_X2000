#include <driver/clk.h>

int clk_gate_enable(int id, int on);

void clk_init(void)
{

}

int clk_enable(enum clk_type id, int on)
{

    if (id < CLK_NUMS) {

        return clk_gate_enable(id - CLK_GATE_0, on);
    }
    return -1;
}
