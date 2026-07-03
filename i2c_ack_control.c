/*
 * i2c_ack_control.c
 *
 * Original address : 0x009E0A  /  FUN_009e0a
 *
 * Controls I2C_CR2 bits 2 (ACK) and 3 (POS).
 *
 * Confirmed disassembly:
 *   push A; tnz A
 *   if A == 0: bres 0x5211, #2   → clear ACK (NACK after next byte)
 *   if A == 1: bres 0x5211, #3   → clear POS, bset 0x5211, #2 → set ACK
 *   else:      bset 0x5211, #3   → set POS,   bset 0x5211, #2 → set ACK
 *   pop A; retf
 *
 * STM8 I2C_CR2 bits used:
 *   bit 2 = ACK — acknowledge returned after byte received
 *   bit 3 = POS — controls which byte is ACK'd (current or next)
 *
 * Called with:
 *   0 → send NACK (end of receive, no more bytes wanted)
 *   1 → send ACK, clear POS (normal single-byte ACK)
 *   2+ → send ACK, set POS  (2-byte receive mode)
 *
 * Used in afe_read_register (FUN_009288) and afe_read_single_byte
 * (FUN_00935E) as the bus_send_stop() wrapper.
 */
#include <stdint.h>

#define I2C_CR2  (*(volatile uint8_t *)0x5211u)

uint8_t i2c_set_ack_mode(uint8_t mode)
{
    if (mode == 0u)
    {
        /* NACK: clear ACK bit — master will NACK next received byte */
        I2C_CR2 &= ~0x04u;
    }
    else if (mode == 1u)
    {
        /* Normal ACK: set ACK, clear POS */
        I2C_CR2  =  (I2C_CR2 & ~0x08u) | 0x04u;
    }
    else
    {
        /* 2-byte ACK: set both ACK and POS */
        I2C_CR2 |= 0x0Cu;
    }
    return mode;
}
