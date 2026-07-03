/*
 * bms_timer_delay.c
 *
 * Original address : 0x0097F6 (thunk_FUN_009802) / 0x009802 (FUN_009802)
 *
 * Confirmed disassembly:
 *   thunk entry (0x0097F6): jumps to 0x009802 with an initial
 *     dec-then-check loop using FUN_00F6B1 (timer tick/countdown)
 *   FUN_009802 (0x009802):
 *     addw X, #0x0004   ; X = &counter (from stack)
 *     callf 0x00F804    ; check_timer_expired(counter)
 *     jrne  loop        ; loop while not expired (Z flag not set)
 *     retf
 *
 *  FUN_00F804: checks if a counter has reached zero (timer expired).
 *  FUN_00F6B1: decrements a counter by 1 each call.
 *
 *  Called as: delay_us(1000, 0) or delay_us(300, 0) throughout firmware.
 *  param_1 = tick count (number of iterations to wait)
 *  param_2 = always 0 (padding / second word of 32-bit counter)
 *
 *  This is a software busy-wait timer used after every I2C transaction
 *  to allow the BQ76952 time to complete internal processing.
 */

#include <stdint.h>
#include <stdbool.h>

extern bool    timer_expired    (uint16_t *counter);  /* FUN_00F804 */
extern void    timer_decrement  (uint8_t amount,      /* FUN_00F6B1 */
                                 uint16_t *counter);

/*
 * delay_ticks
 *
 * Busy-wait for the specified number of timer ticks.
 * param ticks : countdown value (1000 ≈ post-read delay, 300 ≈ post-write)
 * param pad   : always 0 (upper word, unused in practice)
 */
void delay_ticks(uint16_t ticks, uint16_t pad)
{
    uint16_t counter = ticks;
    (void)pad;

    /* decrement first, then check — matches the thunk entry at 0x0097F6 */
    do
    {
        timer_decrement(1u, &counter);
    }
    while (!timer_expired(&counter));
}
