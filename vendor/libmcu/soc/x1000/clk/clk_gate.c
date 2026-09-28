#include <soc/clk.h>
#include <soc/cpm.h>

int clk_gate_enable(int id, int on)
{
    int bit;
    unsigned int clkgr;

    if (id >= 32) {
        bit = id - 32;
        clkgr = CPM_CLKGR1;
    } else {
        bit = id;
        clkgr = CPM_CLKGR;
    }

    if (on)
        cpm_clear_bit(bit, clkgr);
    else
        cpm_set_bit(bit, clkgr);

    return 0;
}
