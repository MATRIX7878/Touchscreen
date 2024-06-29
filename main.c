#include <stdint.h>
#include <stdbool.h>
#include "touchfunc.h"

void main(void)
{
    int pressureThreshhold = 10;

    uart_init();

    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTK;
    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTK;
    SYSCTL[SYSCTL_RCGCADC] |= 0x1;

    SysTick[STCTRL] = 0;
    SysTick[STRELOAD] = 24;
    SysTick[STCURRENT] = 0;
    SysTick[STCTRL] = 0x5;

    while(true){
        p1 = getPoint();

        if (p1.z > pressureThreshhold){
            uart_outstring("X: ");
            uart_dec((uint32_t)p1.x);
            uart_outstring("    Y: ");
            uart_dec((uint32_t)p1.y);
            uart_outstring("    Z: ");
            uart_dec((uint32_t)p1.z);
            out_crlf();
        }
    }
}
