/*
 * i2c_status_check.c
 *
 * FUN_009e6a (0x009E6A) — check specific I2C status flag
 * FUN_009eee (0x009EEE) — check I2C bus busy flag
 *
 * STM8 I2C status registers:
 *   0x5217 = I2C_SR1  — status register 1 (TxE, RxNE, STOPF, BTF, ADDR, SB)
 *   0x5218 = I2C_SR2  — status register 2 (WUFH, OVR, AF, ARLO, BERR, BUSY)
 *   0x5219 = I2C_SR3  — status register 3 (DUALF, GENCALL, TRA, BUSY, MSL)
 */
#include <stdint.h>
#include <stdbool.h>

#define I2C_SR1  (*(volatile uint8_t *)0x5217u)
#define I2C_SR2  (*(volatile uint8_t *)0x5218u)
#define I2C_SR3  (*(volatile uint8_t *)0x5219u)

/*
 * i2c_check_flag  (FUN_009e6a)
 *
 * Checks whether the specified I2C status flag is set.
 *
 * param_1 encoding (16-bit):
 *   high byte = flag mask to AND with the register
 *   low  byte = selects which register to read:
 *     0x04     → read I2C_SR2 bit 2 (AF — acknowledge failure)
 *     anything → read I2C_SR1 & I2C_SR3 combined
 *
 * Returns true if the masked register value matches param_1.
 *
 * Known call sites (confirmed from afe_read_register FUN_009288):
 *   check_flag(0x0302) → bus busy: SR1 with mask 0x03
 *   check_flag(0x0301) → ACK/SB:  SR1 bit 0 (START sent)
 *   check_flag(0x0782) → ADDR:    SR1 bit 1 with mask 0x07
 *   check_flag(0x0784) → BTF:     SR1 bit 2 (byte transfer finished)
 *   check_flag(0x0340) → RxNE:    SR1 bit 6 with mask 0x03
 */
bool i2c_check_flag(uint16_t param_1)
{
    uint8_t flag_hi = (uint8_t)(param_1 >> 8u);   /* mask / register select */
    uint8_t flag_lo = (uint8_t)(param_1 & 0xFFu); /* expected value          */
    uint8_t sr_val;
    uint8_t sr3_val = 0u;

    if (param_1 == 4u)
    {
        /* Special case: check I2C_SR2 bit 2 (AF - acknowledge failure) */
        sr_val  = I2C_SR2 & 0x04u;
    }
    else
    {
        /* General case: read SR1 and SR3 */
        sr_val  = I2C_SR1;
        sr3_val = I2C_SR3;
    }

    /* Match: (sr3_val & flag_hi) == flag_hi AND (sr_val & flag_lo) == flag_lo */
    return (bool)(((sr3_val & flag_hi) == flag_hi) &&
                  ((sr_val  & flag_lo) == flag_lo));
}

/*
 * i2c_check_bus_busy  (FUN_009eee)
 *
 * Checks the I2C bus busy flag.
 *
 * param_1 high byte selects which status register:
 *   0x01 → I2C_SR1  (0x5217)
 *   0x02 → I2C_SR2  (0x5218)
 *   0x03 → I2C_SR3  (0x5219)
 *   other → returns false (0)
 *
 * Returns true if the selected register ANDed with the low byte
 * of param_1 (from stack context) is non-zero.
 *
 * Used in afe_read_register as the bus-idle poll:
 *   while (i2c_check_bus_busy(0x0302)) {} → wait while SR2 bit 1 (BUSY) set
 */
bool i2c_check_bus_busy(uint16_t param_1)
{
    uint8_t reg_sel = (uint8_t)(param_1 >> 8u);
    uint8_t reg_val;

    switch (reg_sel)
    {
        case 0x01u: reg_val = I2C_SR1; break;
        case 0x02u: reg_val = I2C_SR2; break;
        case 0x03u: reg_val = I2C_SR3; break;
        default:    return false;
    }

    /* low byte (in_stack_00000000) = the bit mask, from the caller's stack */
    uint8_t mask = (uint8_t)(param_1 & 0xFFu);
    return (bool)((reg_val & mask) != 0u);
}
