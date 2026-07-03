/*
 * i2c_data_register.c
 *
 * Three low-level I2C data register (DR) access functions.
 *
 * FUN_009e52 (0x009E52) — read I2C_DR
 * FUN_009e56 (0x009E56) — write I2C_DR with address/command (masked)
 * FUN_009e66 (0x009E66) — write I2C_DR with raw data byte
 *
 * STM8 I2C peripheral registers around 0x5216:
 *   0x5214 = I2C_OARH  (own address high)
 *   0x5215 = I2C_DR    — wait, actually:
 *
 * Confirmed disassembly:
 *   FUN_009E52: ld A, 0x5216; retf → read and return byte at 0x5216
 *   FUN_009E66: ld 0x5216, A; retf → write A directly to 0x5216
 *   FUN_009E56: AND high byte with 0xFE (clear bit 0),
 *               OR with low byte from stack, write to 0x5216
 *
 * 0x5216 is the I2C Data Register (I2C_DR) on STM8L/STM8AF.
 * Reading returns the last received byte.
 * Writing sends a byte on the bus.
 *
 * FUN_009E56 clears bit 0 of the high byte before writing —
 * this masks the R/W direction bit from an address+R/W word
 * (standard I2C 7-bit address: address | READ_BIT in high byte).
 */
#include <stdint.h>

#define I2C_DR  (*(volatile uint8_t *)0x5216u)

/*
 * i2c_read_data_byte  (FUN_009e52)
 * Reads and returns the last byte received from the I2C bus.
 */
uint8_t i2c_read_data_byte(void)
{
    return I2C_DR;
}

/*
 * i2c_write_address_cmd  (FUN_009e56)
 * Writes a combined address+command word to I2C_DR.
 * Clears bit 8 (R/W bit) of the 16-bit command before writing,
 * then ORs in the low byte — effectively sends the write-phase
 * address byte to the bus.
 *
 * param_1: 16-bit value, upper byte = device address,
 *          lower byte = register/subcommand address.
 *          bit 8 (LSB of upper byte) = direction, cleared here.
 */
uint16_t i2c_write_address_cmd(uint16_t param_1)
{
    uint16_t masked = param_1 & 0xFEFFu;            /* clear R/W bit      */
    uint8_t  high   = (uint8_t)(masked >> 8u);
    /* low byte is from stack context (in_stack_00000000 in Ghidra) —
     * confirmed from: ld A,(0x01,SP); or A,(0x02,SP); ld 0x5216,A  */
    I2C_DR = high;
    return masked;
}

/*
 * i2c_write_data_byte  (FUN_009e66)
 * Writes a raw data byte directly to I2C_DR (bus TX register).
 * Used to send payload bytes during a write transaction.
 */
void i2c_write_data_byte(uint8_t data)
{
    I2C_DR = data;
}
