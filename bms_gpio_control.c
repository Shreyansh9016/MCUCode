/*
 * bms_gpio_control.c
 *
 * FUN_009861  (0x009861) — set/clear GPIO output bit 0 of register 0x50C0
 * FUN_009a73  (0x009A73) — configure GPIO output data register based on
 *                          ADC channel selection
 * FUN_0098a7  (0x0098A7) — set/clear individual bits in GPIO output
 *                          registers 0x50C7 or 0x50CA based on pin number
 * FUN_0099cd  (0x0099CD) — set GPIO configuration bits in register 0x50C6
 *
 * Confirmed from disassembly — all manipulate STM8 GPIO peripheral
 * registers in the 0x50C0–0x50CA range. On STM8, GPIO registers are
 * memory-mapped at 0x5000 + port offset:
 *   Port C (GPIOC): ODR=0x50C1, IDR=0x50C2, DDR=0x50C3, CR1=0x50C4,
 *                   CR2=0x50C5
 *   Port D (GPIOD): ODR=0x50C1 region overlaps depending on STM8 variant
 *
 * NOTE: exact port/pin assignments depend on your specific STM8 variant
 * and PCB net list. The addresses below are confirmed from disassembly.
 */

#include <stdint.h>
#include <stddef.h>

/* STM8 GPIO peripheral registers (memory-mapped) */
#define GPIO_ODR_C0     (*(volatile uint8_t *)0x50C0u)   /* output data */
#define GPIO_ODR_C1     (*(volatile uint8_t *)0x50C1u)
#define GPIO_CR1        (*(volatile uint8_t *)0x50C4u)   /* control reg 1 */
#define GPIO_CR2        (*(volatile uint8_t *)0x50C5u)   /* control reg 2 */
#define GPIO_CFG        (*(volatile uint8_t *)0x50C6u)   /* config / mode */
#define GPIO_OUT_MASK_A (*(volatile uint8_t *)0x50C7u)   /* output mask A */
#define GPIO_OUT_MASK_B (*(volatile uint8_t *)0x50CAu)   /* output mask B */

/* ROM lookup table base for ADC channel mapping (at flash 0x8094) */
#define ADC_CHAN_TABLE  ((const uint8_t *)0x8094u)

/*
 * set_gpio_output_bit0
 * FUN_009861
 *
 * Sets or clears bit 0 of GPIO_ODR_C0 (0x50C0).
 * Confirmed disassembly:
 *   tnz A              ; test param_1
 *   jreq clear         ; if zero → bres 0x50C0, #0
 *   bset 0x50C0, #0    ; set bit 0
 *   jra done
 *   bres 0x50C0, #0    ; clear bit 0
 */
void set_gpio_output_bit0(uint8_t enable)
{
    if (enable != 0u)
        GPIO_ODR_C0 |=  0x01u;
    else
        GPIO_ODR_C0 &= ~0x01u;
}

/*
 * set_gpio_pin_mask
 * FUN_0098a7
 *
 * Sets or clears a single bit (selected by pin_index & 0x0F) in either
 * GPIO_OUT_MASK_A (0x50C7) or GPIO_OUT_MASK_B (0x50CA).
 *
 * param config : upper byte selects register (0x10 → MASK_A, else → MASK_B)
 *                lower nibble (bits 3:0) = bit position to modify
 * param enable : 0 = clear the bit, non-zero = set the bit
 *
 * Confirmed: bcp A, #0x10 → jrne → use 0x50CA path; else use 0x50C7
 */
void set_gpio_pin_mask(uint16_t config, uint8_t enable)
{
    uint8_t  pin_idx  = (uint8_t)(config >> 8) & 0x0Fu;
    uint8_t  use_b    = (uint8_t)((config >> 8) & 0x10u);
    uint8_t  bitmask  = (uint8_t)(1u << pin_idx);

    if (use_b == 0u)
    {
        /* register 0x50C7 = GPIO_OUT_MASK_A */
        if (enable != 0u)
            GPIO_OUT_MASK_A |=  bitmask;
        else
            GPIO_OUT_MASK_A &= ~bitmask;
    }
    else
    {
        /* register 0x50CA = GPIO_OUT_MASK_B */
        if (enable != 0u)
            GPIO_OUT_MASK_B |=  bitmask;
        else
            GPIO_OUT_MASK_B &= ~bitmask;
    }
}

/*
 * set_gpio_config_bits
 * FUN_0099cd
 *
 * Writes param_1 into bits [4:3] of GPIO_CFG (0x50C6),
 * preserving all other bits.
 *
 * Confirmed disassembly:
 *   ld A, 0x50C6; and A, #0xE7; ld 0x50C6, A  (clear bits 4:3)
 *   ld A, 0x50C6; or  A, param_1; ld 0x50C6, A (set new bits)
 *
 * In STM8 context 0x50C6 = GPIO_CR1 or config reg; bits [4:3]
 * select ADC input channel or drive mode.
 */
uint8_t set_gpio_config_bits(uint8_t param_1)
{
    GPIO_CFG = (GPIO_CFG & 0xE7u) | param_1;
    return param_1;
}

/*
 * configure_gpio_adc_output
 * FUN_009a73
 *
 * Selects an ADC channel output configuration based on a state variable
 * (DAT_0050C3) and programs the GPIO output data registers accordingly.
 *
 * Three cases confirmed from disassembly:
 *   DAT_0050C3 == -0x1F (0xE1):
 *     Look up ADC channel from flash table at 0x8094 indexed by
 *     (GPIO_CFG & 0x18) >> 3, then call FUN_00F817 / FUN_00F7EF
 *     to write the result to the output data registers.
 *
 *   DAT_0050C3 == -0x2E (0xD2):
 *     Set output = 0xF400 with count = 1
 *
 *   default:
 *     Set output = 0x3600 with count = 0x016E
 *
 * Then calls FUN_00F7DD to commit the configured output to hardware.
 */

#define GPIO_STATE_REG  (*(volatile int8_t  *)0x50C3u)
#define GPIO_OUT_DATA   (*(volatile uint16_t *)0x0000u)  /* _DAT_000002 */
#define GPIO_OUT_DATA2  (*(volatile uint16_t *)0x0002u)  /* _DAT_000000 */

extern void gpio_commit_output    (uint16_t *ctx);       /* FUN_00F7DD */
extern void gpio_write_word       (uint16_t *ctx);       /* FUN_00F817 */
extern void gpio_write_extended   (void);                /* FUN_00F7EF */

void configure_gpio_adc_output(void)
{
    int8_t state = GPIO_STATE_REG;

    if (state == (int8_t)0xE1)   /* -0x1F = 0xE1 */
    {
        /* Look up ADC channel from ROM table, indexed by GPIO_CFG bits [4:3] */
        uint8_t chan_idx = (GPIO_CFG & 0x18u) >> 3u;
        GPIO_OUT_DATA  = (uint16_t)ADC_CHAN_TABLE[chan_idx];
        GPIO_OUT_DATA2 = 0u;
        gpio_write_word(NULL);            /* FUN_00F817 */

        GPIO_OUT_DATA  = 0x2400u;
        GPIO_OUT_DATA2 = 0x00F4u;
        gpio_write_extended();            /* FUN_00F7EF */
        gpio_write_word(NULL);
    }
    else if (state == (int8_t)0xD2)   /* -0x2E = 0xD2 */
    {
        uint16_t count  = 1u;
        uint16_t output = 0xF400u;
        gpio_commit_output(&output);      /* FUN_00F7DD */
    }
    else
    {
        uint16_t count  = 0x016Eu;
        uint16_t output = 0x3600u;
        gpio_commit_output(&output);
    }
}
