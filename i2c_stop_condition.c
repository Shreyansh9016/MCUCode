/*
 * i2c_stop_condition.c
 *
 * Original address : 0x009DE0  /  FUN_009de0
 *
 * Controls I2C_CR2 bit 1 = STOP (generate STOP condition).
 *
 * Confirmed disassembly:
 *   tnz A
 *   jreq → bres 0x5211, #1   (clear STOP bit)
 *   else → bset 0x5211, #1   (set   STOP bit → generate STOP)
 *
 * Used to release the I2C bus after each transaction.
 * Called with 0 at transaction start (bus_set_cs(0) in
 * afe_read_register) and with 1 to finalise.
 *
 * Note: Ghidra calls this "FUN_009de0" and maps it as
 * "bus_set_cs" in the transaction wrappers, but the disassembly
 * confirms this is specifically the STOP bit control.
 */
#include <stdint.h>

#define I2C_CR2  (*(volatile uint8_t *)0x5211u)

void i2c_set_stop_condition(uint8_t stop)
{
    if (stop != 0u)
        I2C_CR2 |=  0x02u;   /* set   STOP bit → release bus */
    else
        I2C_CR2 &= ~0x02u;   /* clear STOP bit                */
}
