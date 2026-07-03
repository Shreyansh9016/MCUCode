/*
 * i2c_start_condition.c
 *
 * Original address : 0x009DD2  /  FUN_009dd2
 *
 * Controls I2C_CR2 bit 0 = START (generate START condition).
 *
 * Confirmed disassembly:
 *   tnz A
 *   jreq → bres 0x5211, #0   (clear START bit)
 *   else → bset 0x5211, #0   (set   START bit → generate START)
 *
 * STM8 I2C_CR2 (0x5211):
 *   bit 0 = START  — generate START condition on bus
 *   bit 1 = STOP   — generate STOP  condition on bus
 *   bit 2 = ACK    — acknowledge enable
 *   bit 3 = POS    — ACK/PEC position (for 2-byte receive)
 *   bit 7 = SWRST  — software reset
 *
 * Used in every I2C read/write transaction (FUN_009288, FUN_00935E).
 * Called with 1 to begin a START, with 0 to clear after transaction.
 */
#include <stdint.h>

#define I2C_CR2  (*(volatile uint8_t *)0x5211u)

void i2c_set_start_condition(uint8_t start)
{
    if (start != 0u)
        I2C_CR2 |=  0x01u;   /* set   START bit → generate START */
    else
        I2C_CR2 &= ~0x01u;   /* clear START bit                   */
}
