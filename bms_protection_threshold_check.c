/*
 * bms_protection_threshold_check.c
 *
 * Original address : 0x00D898  /  FUN_00d898
 * Function name    : bms_protection_threshold_check
 *
 * Compares all BMS measurements against protection thresholds
 * stored in flash and sets/clears fault flags accordingly.
 * This is the main PROTECTION LOOP — called every 4 ticks from
 * bms_main_loop alongside bms_cell_balancing_and_soc.
 *
 * Confirmed disassembly structure:
 *
 * Step 1 — Load measurement data into local 72-byte buffer:
 *   copy_measurement_block_160bytes(buf + 0x21)  → cell voltages/temps
 *   copy_raw_block_32bytes(buf + 0x05)           → status/safety regs
 *   Then copies 10 pairs from the block into local_3f..local_0b
 *   (cell voltage pairs for cells 1-5 × 2 bytes each)
 *
 * Step 2 — SOC sign check:
 *   uVar1 = (local_3f & 0x8000) != 0  → is SOC accumulator negative?
 *   if (uVar1 == 0):  [SOC is positive — run overvoltage check chain]
 *
 *   The main body (positive SOC path) is a chain of up to 10
 *   nested signed comparisons (confirmed: FUN_00f1c7 → FUN_00f817 →
 *   FUN_00f7dd → FUN_00f2bd → FUN_00f39a pattern, repeated 10 times
 *   with offsets 0xDC82, 0xDCAF, 0xDCDC, 0xDD09, 0xDD36, 0xDD63,
 *   0xDD90, 0xDDBD, 0xDDEA, 0xDE17):
 *
 *   Iteration i (0-9):
 *     float_load_reg(8, prev_result)     → load previous comparison
 *     float_store(local_slot_i)          → store to local slot
 *     float_load(local_slot_i + 0x0D)   → load cell voltage float
 *     float_sub_compare(local_slot_i+0x11) → compare cell vs threshold
 *     float_compare(local_slot_i + 1)   → signed compare
 *     if (overvoltage detected):         → set OV flag, break chain
 *
 *   OV detection threshold addresses step from flash:
 *   0xDC82, 0xDCAF, 0xDCDC, 0xDD09 ... (stepping by 0x2D = 45 per cell)
 *
 * Step 3 — After the chain (via puVar4/puVar5 tracking the last
 *   cell checked), compute pack-level protection:
 *   float_load(pack_voltage) vs flash[0x80BC]
 *   → if pack undervoltage detected: set UV flag
 *   float_load(pack_current) vs flash[0x80C0]
 *   → if pack overcurrent detected:  set OC flag
 *
 * Step 4 — Store final protection flags:
 *   FUN_00f817(0x246)  → store protection word to RAM[0x246]
 *   DAT_00505F = 0     → clear TX-busy flag
 *
 * RAM variables:
 *   Fault flags written: 0x01EE (OC_CHG), 0x01F8 (OC_DSG), etc.
 *   Protection output: 0x0246 (protection status word for CAN TX)
 */
#include <stdint.h>
#include <stdbool.h>

/* Protection output register */
#define PROTECTION_STATUS  (*(volatile uint16_t *)0x0246u)
#define TX_BUSY_FLAG       (*(volatile uint8_t  *)0x505Fu)
#define SOC_ACCUMULATOR    (*(volatile int16_t  *)0x0249u)

/* Fault output flags */
#define FLAG_OV_FAULT   (*(volatile uint16_t *)0x01EEu)  /* overvoltage  */
#define FLAG_UV_FAULT   (*(volatile uint16_t *)0x01F8u)  /* undervoltage */
#define FLAG_OC_FAULT   (*(volatile uint16_t *)0x01F6u)  /* overcurrent  */

/* Flash threshold addresses (confirmed from disassembly) */
#define THRESH_OV_BASE  ((const volatile uint8_t *)0xDC82u)  /* OV per cell  */
#define THRESH_OV_STEP  0x2Du                                 /* 45 per cell  */
#define THRESH_UV_PACK  ((const volatile uint8_t *)0x80BCu)  /* pack UV      */
#define THRESH_OC_PACK  ((const volatile uint8_t *)0x80C0u)  /* pack OC      */

#define NUM_CELLS       10u

/* Buffer copy helpers */
extern void copy_measurement_block_160bytes(uint8_t *dst);  /* FUN_009077 */
extern void copy_raw_block_32bytes(uint8_t *dst);           /* FUN_009705 */

/* Float arithmetic helpers */
extern void    float_load_reg   (uint8_t reg, uint16_t ctx);          /* FUN_00F1C7 */
extern void    float_store      (uint8_t *dst);                        /* FUN_00F817 */
extern void    float_load_buf   (const uint8_t *src);                  /* FUN_00F7DD */
extern void    float_sub        (const uint8_t *subtrahend);           /* FUN_00F2BD */
extern int8_t  float_compare    (const uint8_t *threshold);            /* FUN_00F39A */

void bms_protection_threshold_check(void)
{
    uint8_t  buf[72u];
    uint16_t cell_pair[10u];
    uint16_t soc_word;
    bool     any_ov = false;
    uint8_t  i;

    /* Step 1: load measurements */
    copy_measurement_block_160bytes(buf + 0x21u);
    copy_raw_block_32bytes(buf + 0x05u);

    /* Copy cell voltage pairs into locals
     * (confirmed: 10 ldw X,(0x??SP); ldw (0x??SP),X pairs) */
    for (i = 0u; i < 5u; i++)
    {
        cell_pair[i*2u]       = *(uint16_t *)(buf + 0x33u - i*4u);
        cell_pair[i*2u + 1u]  = *(uint16_t *)(buf + 0x31u - i*4u);
    }
    soc_word = cell_pair[9];   /* local_3f */

    /* Step 2: only run OV check if SOC accumulator is non-negative */
    if ((soc_word & 0x8000u) == 0u)
    {
        /*
         * OV check chain — up to 10 cells.
         * Each iteration: load cell voltage as float, compare vs
         * per-cell OV threshold from flash (stepping by 45 bytes).
         * Stop at first cell that exceeds OV threshold.
         *
         * Confirmed pattern:
         *   FUN_00F1C7(8, result) → load float from register 8
         *   FUN_00F817(slot)      → store to local slot
         *   FUN_00F7DD(slot+0xD)  → load cell voltage
         *   FUN_00F2BD(slot+0x11) → subtract threshold
         *   FUN_00F39A(slot+1)    → signed compare result
         *   if (V bit set): set OV flag, done
         */
        for (i = 0u; i < NUM_CELLS; i++)
        {
            const volatile uint8_t *thresh =
                THRESH_OV_BASE + (uint16_t)(i * THRESH_OV_STEP);

            float_load_reg(8u, i);
            float_store(buf + i * 2u);
            float_load_buf(buf + i * 2u + 0x0Du);
            float_sub(thresh);
            if (float_compare(buf + i * 2u + 1u) > 0)
            {
                FLAG_OV_FAULT = 1u;
                any_ov = true;
                break;
            }
        }
        if (!any_ov)
            FLAG_OV_FAULT = 0u;
    }

    /* Step 3: pack-level UV and OC checks */

    /* Pack undervoltage: compare pack voltage vs flash[0x80BC] */
    float_load_buf(buf + 0x0Bu);
    if (float_compare(THRESH_UV_PACK) < 0)
        FLAG_UV_FAULT = 1u;
    else
        FLAG_UV_FAULT = 0u;

    /* Pack overcurrent: compare pack current vs flash[0x80C0] */
    float_load_buf(buf + 0x0Du);
    if (float_compare(THRESH_OC_PACK) > 0)
        FLAG_OC_FAULT = 1u;
    else
        FLAG_OC_FAULT = 0u;

    /* Step 4: store protection status and clear TX-busy */
    float_store((uint8_t *)&PROTECTION_STATUS);
    TX_BUSY_FLAG = 0u;
}
