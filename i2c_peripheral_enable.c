/*
 * i2c_peripheral_enable.c
 *
 * Original address : 0x009DB6  /  FUN_009db6
 *
 * Controls I2C_CR1 bit 0 = PE (Peripheral Enable).
 *
 * Confirmed disassembly:
 *   tnz A
 *   jreq → bres 0x5210, #0   (clear PE = disable I2C)
 *   else → bset 0x5210, #0   (set   PE = enable  I2C)
 *
 * STM8 I2C_CR1 (0x5210):
 *   bit 0 = PE   — peripheral enable (1=on, 0=off/reset)
 *   bit 6 = ENGC — general call enable
 *   bit 7 = NOSTRETCH — clock stretching disable
 *
 * Called from init_measurement_buffers() with param=1
 * to enable the I2C bus after full configuration.
 */
#include <stdint.h>

#define I2C_CR1  (*(volatile uint8_t *)0x5210u)

void i2c_set_peripheral_enable(uint8_t enable)
{
    if (enable != 0u)
        I2C_CR1 |=  0x01u;   /* set   PE bit — enable I2C  */
    else
        I2C_CR1 &= ~0x01u;   /* clear PE bit — disable I2C */
}
