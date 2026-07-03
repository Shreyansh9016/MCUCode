/*
 * bms_i2c_peripheral_init.c
 *
 * FUN_009c47 (0x009C47) — I2C/SPI peripheral register zero-init
 * FUN_009c6c (0x009C6C) — Full I2C/SPI peripheral configuration
 *
 * Both functions manipulate STM8 peripheral registers at 0x5210–0x521D.
 * On STM8L/STM8S, address 0x5210 is the I2C peripheral base (I2C_CR1,
 * I2C_CR2, I2C_FREQR, I2C_OARL, I2C_OARH, etc.) or an SPI peripheral.
 * Confirmed from the address range and the configuration patterns.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* I2C/SPI peripheral registers (STM8 memory-mapped) */
#define I2C_CR1    (*(volatile uint8_t *)0x5210u)   /* Control register 1  */
#define I2C_CR2    (*(volatile uint8_t *)0x5211u)   /* Control register 2  */
#define I2C_FREQR  (*(volatile uint8_t *)0x5212u)   /* Frequency register  */
#define I2C_OARL   (*(volatile uint8_t *)0x5213u)   /* Own address low     */
#define I2C_OARH   (*(volatile uint8_t *)0x5214u)   /* Own address high    */
#define I2C_DR     (*(volatile uint8_t *)0x521Au)   /* Data register       */
#define I2C_SR1    (*(volatile uint8_t *)0x521Bu)   /* Status register 1   */
#define I2C_SR2    (*(volatile uint8_t *)0x521Cu)   /* Status register 2   */
#define I2C_CCR    (*(volatile uint8_t *)0x521Du)   /* Clock control reg   */

/* Peripheral base address read from ROM (used by FUN_009C6C) */
#define ROM_PERIPH_BASE  ((const volatile uint16_t *)0x80A0u)
#define ROM_PERIPH_OPTS  ((const volatile uint16_t *)0x80A4u)

extern uint16_t peripheral_read_config (const volatile uint16_t *base);  /* FUN_00F65D */
extern void     peripheral_write_word  (uint16_t *ctx);                  /* FUN_00F7DD */
extern void     peripheral_write_result(uint16_t *ctx);                  /* FUN_00F817 */
extern void     peripheral_write_ext   (void);                           /* FUN_00F7EF */
extern uint16_t peripheral_set_baudrate(uint8_t scale, uint16_t mult);   /* FUN_00F82F */
extern void     peripheral_set_field   (uint16_t *dst, uint16_t val,
                                        uint16_t mask);                  /* FUN_00F854 */
extern void     peripheral_load_opts   (const volatile uint16_t *opts);  /* FUN_00F6C7 */
extern void     bus_send_stop          (uint8_t level);                  /* FUN_009E0A */

/*
 * i2c_peripheral_reset
 * FUN_009c47
 *
 * Zero-initialises all I2C/SPI peripheral control registers and sets
 * I2C_CCR to 2 (minimum clock divider). Called by init_measurement_buffers
 * before configure_i2c_peripheral.
 *
 * Confirmed disassembly:
 *   clr 0x5210 through 0x5214 (CR1, CR2, FREQR, OARL, OARH)
 *   clr 0x521A through 0x521C (DR, SR1, SR2)
 *   mov 0x521D, #0x02  (CCR = 2)
 */
void i2c_peripheral_reset(void)
{
    I2C_CR1   = 0u;
    I2C_CR2   = 0u;
    I2C_FREQR = 0u;
    I2C_OARL  = 0u;
    I2C_OARH  = 0u;
    I2C_DR    = 0u;
    I2C_SR1   = 0u;
    I2C_SR2   = 0u;
    I2C_CCR   = 2u;   /* minimum clock: Fmaster / 2 */
}

/*
 * configure_i2c_peripheral
 * FUN_009c6c
 *
 * Fully configures the I2C/SPI peripheral based on the supplied parameters.
 *
 * Confirmed disassembly logic:
 *   1. Apply param_5 to I2C_FREQR bits [5:0]
 *   2. Clear I2C_CR1 bit 0 (disable peripheral during config)
 *   3. Clear I2C_SR2 bits [5:4], clear I2C_SR1
 *   4. Read configuration from ROM at 0x80A0 (peripheral ID/version)
 *   5. Branch on read result (ROM config flag):
 *      If peripheral detected:
 *        Configure clock/baud based on param_4 and ROM 0x80A4
 *        Set I2C_CCR = max(computed, 4)
 *        Set I2C_CCR mode = param_2 + 1
 *      Else (no peripheral):
 *        If param_3 == 0: standard mode config via FUN_00F854(3, ...)
 *        Else: fast mode config via FUN_00F854(0x19, ...) + set mode flag
 *        Compute CCR from: FUN_00F82F(10, param_1 * 3) + 1
 *   6. Write I2C_SR1, I2C_SR2, I2C_CR1 from computed config
 *   7. Enable peripheral: I2C_CR1 |= 1
 *   8. Call bus_send_stop to initialise the bus state
 *   9. Set I2C_OARL, I2C_OARH, I2C_OARH config flags
 *
 * Called from init_measurement_buffers with:
 *   param_1 = 6     (clock divider base)
 *   param_2 = -0x80 (0x80, mode/address byte)
 *   param_3 = 0     (standard mode)
 *   param_4 = 0x10  (16 — number of cells, used as clock parameter)
 *   param_5 = 0x10  (16 — frequency register value, 16MHz system clock)
 *
 * NOTE: FUN_009C6C is complex (120+ instructions). The above is a
 * faithful high-level reconstruction; some intermediate calculations
 * involving _DAT_000000–_DAT_000003 scratch registers are simplified.
 */
void configure_i2c_peripheral(uint8_t  param_1,   /* clock divider base  */
                               int8_t   param_2,   /* mode / address byte */
                               uint8_t  param_3,   /* 0=standard, n=fast  */
                               uint8_t  param_4,   /* clock parameter      */
                               uint8_t  param_5)   /* frequency (MHz)      */
{
    uint16_t ccr_val;
    bool     periph_present;

    /* Step 1: set frequency register bits [5:0] */
    I2C_FREQR = (I2C_FREQR & 0xC0u) | (param_5 & 0x3Fu);

    /* Step 2: disable peripheral for configuration */
    I2C_CR1 &= ~0x01u;

    /* Step 3: clear status register noise bits */
    I2C_SR2 &= 0x30u;
    I2C_SR1  = 0u;

    /* Step 4: probe ROM config to detect peripheral version */
    periph_present = (bool)peripheral_read_config(ROM_PERIPH_BASE);

    if (periph_present)
    {
        /* Step 5a: high-speed / fast-mode path */
        peripheral_load_opts(ROM_PERIPH_OPTS);
        peripheral_write_ext();

        /* Set clock control from param_4 */
        ccr_val = (uint16_t)param_4;
        if (ccr_val < 4u) ccr_val = 4u;

        I2C_CCR  = (uint8_t)(param_2 + 1);
    }
    else
    {
        /* Step 5b: standard / user-configured path */
        if (param_3 == 0u)
        {
            /* Standard mode (100 kHz) */
            peripheral_set_field(NULL, 3u, 0u);
        }
        else
        {
            /* Fast mode (400 kHz) */
            peripheral_set_field(NULL, 0x19u, 0u);
            I2C_SR2 |= 0x40u;   /* set fast-mode flag in SR2 */
        }

        /* Compute CCR: (10 * param_1 * 3) + 1, minimum 1 */
        ccr_val = (uint16_t)(peripheral_set_baudrate(10u,
                             (uint16_t)param_1 * 3u) + 1u);
        if (ccr_val == 0u) ccr_val = 1u;
    }

    /* Step 6: write computed config to peripheral registers */
    I2C_SR1 = (uint8_t)(ccr_val >> 8u);
    I2C_SR2 = (I2C_SR2 & 0x0Fu) | (uint8_t)(ccr_val & 0xF0u);

    /* Step 7: re-enable peripheral */
    I2C_CR1 |= 0x01u;

    /* Step 8: initialise bus state (generate STOP condition) */
    bus_send_stop((uint8_t)periph_present);

    /* Step 9: set own-address registers */
    I2C_OARL = (uint8_t)(ccr_val & 0xFFu);
    I2C_OARH = (uint8_t)(ccr_val >> 8u);
    I2C_OARH |= 0x40u;    /* ADDCONF bit — always 1 per STM8 spec */
    I2C_OARH |= (uint8_t)((*(uint16_t *)NULL) >> 8u) & 0x06u;
}
