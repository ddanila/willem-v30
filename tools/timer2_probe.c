/* Timing qualification only: no LPT accesses and no chip power changes. */
#include <stdio.h>
extern void dos_timer2_50ms(void);
int main(void)
{
    unsigned i;
    for (i = 0; i < 100U; i++) dos_timer2_50ms();
    puts("100 PIT2 waits completed; expected about 5 seconds; no LPT accesses");
    return 0;
}
