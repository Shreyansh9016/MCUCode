/*
 * bms_fet_protection_control.c
 *
 * Original address : 0x00D76E  /  FUN_00d76e
 * Function name    : bms_fet_protection_control
 *
 * Controls charge/discharge FET enable pins (GPIO 0x500A and 0x500F)
 * based on active fault flags. Also manages enable flags for charge
 * and discharge paths.
 *
 * Confirmed disassembly — four decision blocks:
 *
 * Block 1 — Any fault present:
 *   if (OC_DSG_2 || OC_CHG_2 || OC_CHG || OC_DSG):
 *     if (OC_CHG_2 == 0 && OC_DSG_2 == 0):   [soft fault — toggle off]
 *       if (OC_CHG || OC_DSG):
 *         reg_toggle_bits(GPIO_C, 0x10)  → toggle bit 4 of 0x500F
 *         reg_toggle_bits(GPIO_B, 0x10)  → toggle bit 4 of 0x500A
 *         return 0xCE
 *     else:                                   [hard fault — set on]
 *       reg_set_bits(GPIO_C, 0x10)        → set bit 4 of 0x500F
 *       reg_set_bits(GPIO_B, 0x10)        → set bit 4 of 0x500A
 *       return 0x9F
 *
 * Block 2 — Overcurrent (CHG_2 or DSG_2 or short):
 *   if (OC_CHG_2 || OC_DSG_2 || SHORT_CIRCUIT):
 *     reg_set_bits(GPIO_B, 4)             → enable overcurrent bit
 *     FLAG_DSG_ENABLE = 0               → disable discharge
 *
 * Block 3 — Additional overcurrent (CHG_2 or DSG_2 or UC):
 *   if (OC_CHG_2 || OC_DSG_2 || UNDER_CURRENT):
 *     reg_set_bits(GPIO_B, 8)             → enable second OC bit
 *     FLAG_CHG_ENABLE = 0               → disable charge
 *
 * Block 4 — No fault condition:
 *   if (all fault flags clear):
 *     reg_clear_bits(GPIO_C, 0x10)       → clear bit 4 of 0x500F
 *     reg_clear_bits(GPIO_C, 0x10)       → (repeated clear)
 *     reg_clear_bits(GPIO_B, 0x10)       → clear bit 4 of 0x500A
 *   if (CHG_2 || DSG_2 || all clear):
 *     reg_clear_bits(GPIO_B, 4)          → clear OC bit
 *     FLAG_DSG_ENABLE = 1              → re-enable discharge
 *   if (all clear including UC):
 *     reg_clear_bits(GPIO_B, 8)          → clear second OC bit
 *     FLAG_CHG_ENABLE = 1              → re-enable charge
 *
 * GPIO registers:
 *   0x500A = GPIO_B (charge FET / overcurrent pins)
 *   0x500F = GPIO_C (discharge FET / fault indicator pins)
 *
 * Fault flags:
 *   0x01F8 = OC_DSG_2  (overcurrent discharge 2 — hard fault)
 *   0x01EE = OC_CHG_2  (overcurrent charge 2 — hard fault)
 *   0x01F6 = OC_CHG    (overcurrent charge 1 — soft fault)
 *   0x01FE = OC_DSG    (overcurrent discharge 1 — soft fault)
 *   0x01EA = SHORT_CIRCUIT
 *   0x01E8 = UNDER_CURRENT
 *   0x01E4 = FLAG_CHG_ENABLE (charge  FET enable state)
 *   0x01E6 = FLAG_DSG_ENABLE (discharge FET enable state)
 */
#include <stdint.h>
#include <stdbool.h>

/* GPIO registers */
#define GPIO_B  ((volatile uint8_t *)0x500Au)
#define GPIO_C  ((volatile uint8_t *)0x500Fu)

/* Fault flags */
#define OC_DSG_2       (*(volatile uint16_t *)0x01F8u)
#define OC_CHG_2       (*(volatile uint16_t *)0x01EEu)
#define OC_CHG         (*(volatile uint16_t *)0x01F6u)
#define OC_DSG         (*(volatile uint16_t *)0x01FEu)
#define SHORT_CIRCUIT  (*(volatile uint16_t *)0x01EAu)
#define UNDER_CURRENT  (*(volatile uint16_t *)0x01E8u)

/* FET enable flags */
#define FLAG_CHG_ENABLE (*(volatile uint16_t *)0x01E4u)
#define FLAG_DSG_ENABLE (*(volatile uint16_t *)0x01E6u)

/* Bit manipulation helpers */
extern uint8_t *reg_set_bits   (uint8_t *reg, uint8_t mask);  /* FUN_009C0D */
extern uint8_t *reg_clear_bits (uint8_t *reg, uint8_t mask);  /* FUN_009C14 */
extern uint8_t *reg_toggle_bits(uint8_t *reg, uint8_t mask);  /* FUN_009C1C */

uint8_t bms_fet_protection_control(uint8_t param_1)
{
    bool any_fault  = (OC_DSG_2 || OC_CHG_2 || OC_CHG || OC_DSG);

    /* ── Block 1: FET output control based on fault severity ── */
    if (any_fault)
    {
        if ((OC_CHG_2 == 0u) && (OC_DSG_2 == 0u))
        {
            /* Soft fault — toggle FET output bits */
            if (OC_CHG || OC_DSG)
            {
                reg_toggle_bits(GPIO_C, 0x10u);
                reg_toggle_bits(GPIO_B, 0x10u);
                return 0xCEu;
            }
        }
        else
        {
            /* Hard fault — force FET bits on */
            reg_set_bits(GPIO_C, 0x10u);
            reg_set_bits(GPIO_B, 0x10u);
            return 0x9Fu;
        }
    }

    /* ── Block 2: Overcurrent — disable discharge FET ── */
    if (OC_CHG_2 || OC_DSG_2 || SHORT_CIRCUIT)
    {
        reg_set_bits(GPIO_B, 0x04u);
        param_1        = 0xDAu;
        FLAG_DSG_ENABLE = 0u;
    }

    /* ── Block 3: Overcurrent / under-current — disable charge FET ── */
    if (OC_CHG_2 || OC_DSG_2 || UNDER_CURRENT)
    {
        reg_set_bits(GPIO_B, 0x08u);
        param_1        = 0xF7u;
        FLAG_CHG_ENABLE = 0u;
    }

    /* ── Block 4: No fault — clear FET bits, re-enable paths ── */
    if (!any_fault)
    {
        reg_clear_bits(GPIO_C, 0x10u);
        reg_clear_bits(GPIO_C, 0x10u);   /* confirmed: cleared twice */
        reg_clear_bits(GPIO_B, 0x10u);
    }

    if (!(OC_CHG_2 || OC_DSG_2 || SHORT_CIRCUIT))
    {
        reg_clear_bits(GPIO_B, 0x04u);
        param_1         = 0x5Cu;
        FLAG_DSG_ENABLE = 1u;
    }

    if (!(OC_CHG_2 || OC_DSG_2 || OC_CHG || UNDER_CURRENT || OC_DSG))
    {
        reg_clear_bits(GPIO_B, 0x08u);
        param_1         = 0x90u;
        FLAG_CHG_ENABLE = 1u;
    }

    return param_1;
}
