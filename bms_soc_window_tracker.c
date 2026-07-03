/*
 * bms_soc_window_tracker.c
 *
 * Original address : 0x00D689  /  FUN_00d689
 * Function name    : bms_soc_window_tracker
 *
 * Tracks SOC crossing of charge/discharge window boundaries
 * and computes a display-ready CAN value for the SOC output.
 *
 * Confirmed disassembly:
 *
 * Step 1 — One-time CAN ID init (if DAT_004098 == 0):
 *   DAT_004052 = 0x07  (CAN ID high byte of SOC broadcast frame)
 *   DAT_004053 = 0xD0  (CAN ID low  byte)
 *   DAT_004098 = 1     (mark as initialised)
 *
 * Step 2 — Build a 16-bit combined CAN ID word and call
 *   FUN_00f616(DAT_004053, DAT_004052) → float encode of CAN ID
 *   FUN_00f817(SP+3) → store float result to local_5
 *
 * Step 3 — SOC window tracking (_DAT_000264 = SOC result from
 *   bms_cell_balancing_and_soc):
 *   if (_DAT_000242 == 1):         (currently in mid-SOC window)
 *     if (_DAT_000264 > 79):        _DAT_000236 = 1  (high flag)
 *     if (_DAT_000264 < 41):        _DAT_000238 = 1  (low  flag)
 *   if (40 < _DAT_000264 < 80):    _DAT_000242 = 1  (enter window)
 *
 * Step 4 — If BOTH high and low flags set (full charge/discharge
 *   cycle observed):
 *   - Compute FUN_00f6b1(1, ...)  → update cycle counter
 *   - Restore CAN ID bytes (DAT_004052/53 = local_5 / local_3)
 *   - Clear _DAT_000236, _DAT_000238, _DAT_000242 (reset flags)
 *
 * Step 5 — Compute display SOC value:
 *   float_load_ptr(local_b)
 *   FUN_00f8be(result)   → float multiply
 *   FUN_00f817(local_d)  → store
 *   FUN_00f7dd(local_f)  → reload
 *   FUN_00f3d0(0x80D0)   → multiply by calibration
 *   FUN_00f4a5(0x80D8)   → multiply by scale
 *   FUN_00f817(0x244)    → store SOC display value → RAM[0x244]
 *
 * RAM variables:
 *   0x004098 = init flag (set on first call)
 *   0x004052 = CAN SOC frame ID high byte (default 0x07)
 *   0x004053 = CAN SOC frame ID low  byte (default 0xD0)
 *   0x000242 = "in SOC window" flag
 *   0x000236 = SOC high-crossed flag
 *   0x000238 = SOC low-crossed flag
 *   0x000264 = current SOC% (from bms_cell_balancing_and_soc)
 *   0x000244 = display/CAN SOC output value
 */
#include <stdint.h>
#include <stdbool.h>

/* CAN SOC frame ID bytes */
#define CAN_SOC_ID_HI    (*(volatile uint8_t  *)0x4052u)
#define CAN_SOC_ID_LO    (*(volatile uint8_t  *)0x4053u)
#define FLAG_CAN_INIT    (*(volatile uint8_t  *)0x4098u)

/* SOC window tracking flags */
#define SOC_IN_WINDOW    (*(volatile uint16_t *)0x0242u)
#define SOC_HIGH_CROSSED (*(volatile uint16_t *)0x0236u)
#define SOC_LOW_CROSSED  (*(volatile uint16_t *)0x0238u)

/* SOC values */
#define SOC_CURRENT      (*(volatile uint16_t *)0x0264u)  /* from balancing fn */
#define SOC_DISPLAY_OUT  (*(volatile uint16_t *)0x0244u)  /* display/CAN value */

/* Activity marker */
#define ACTIVITY_MARKER  (*(volatile uint8_t  *)0x5064u)
#define TX_BUSY_FLAG     (*(volatile uint8_t  *)0x505Fu)

/* SOC window boundaries (confirmed: 0x29=41, 0x4F=79, 0x28=40, 0x50=80) */
#define SOC_WINDOW_LO    40u
#define SOC_WINDOW_HI    80u

/* Float arithmetic helpers */
extern void     float_load_int  (uint16_t a, uint16_t b);          /* FUN_00F616 */
extern void     float_store     (uint8_t *dst);                     /* FUN_00F817 */
extern void     float_load_buf  (const uint8_t *src);               /* FUN_00F7DD */
extern void     float_encode    (uint16_t val);                     /* FUN_00F8BE */
extern void     float_mul_cal   (const volatile uint8_t *cal);      /* FUN_00F3D0 */
extern void     float_mul_scale (const volatile uint16_t *scale);   /* FUN_00F4A5 */
extern void     cycle_counter_update(uint8_t step, uint8_t *ctx);  /* FUN_00F6B1 */

uint16_t bms_soc_window_tracker(void)
{
    uint8_t saved_id_lo;
    uint8_t saved_id_hi;

    /* Kick activity watchdog */
    ACTIVITY_MARKER = 0x56u;

    /* Step 1: one-time CAN SOC frame ID initialisation */
    if (FLAG_CAN_INIT == 0u)
    {
        CAN_SOC_ID_HI = 0x07u;
        CAN_SOC_ID_LO = 0xD0u;
        FLAG_CAN_INIT = 1u;
    }

    /* Step 2: encode current CAN ID as float for later restore */
    saved_id_lo = CAN_SOC_ID_LO;
    saved_id_hi = CAN_SOC_ID_HI;
    float_load_int(CAN_SOC_ID_LO, CAN_SOC_ID_HI);

    /* Step 3: SOC window crossing detection */
    if (SOC_IN_WINDOW == 1u)
    {
        if (SOC_CURRENT > (uint16_t)(SOC_WINDOW_HI - 1u))
            SOC_HIGH_CROSSED = 1u;  /* SOC went above 79% */
        if (SOC_CURRENT < (uint16_t)(SOC_WINDOW_LO + 1u))
            SOC_LOW_CROSSED  = 1u;  /* SOC went below 41% */
    }
    if ((SOC_CURRENT > (uint16_t)SOC_WINDOW_LO) &&
        (SOC_CURRENT < (uint16_t)SOC_WINDOW_HI))
    {
        SOC_IN_WINDOW = 1u;         /* entered the 41-79% window */
    }

    /* Step 4: if a full charge/discharge cycle has been observed */
    if ((SOC_HIGH_CROSSED != 0u) && (SOC_LOW_CROSSED != 0u))
    {
        cycle_counter_update(1u, (uint8_t *)0xD71Du);

        /* Restore original CAN SOC ID bytes */
        CAN_SOC_ID_LO  = saved_id_lo;
        CAN_SOC_ID_HI  = saved_id_hi;

        /* Clear all window-tracking flags */
        SOC_HIGH_CROSSED = 0u;
        SOC_LOW_CROSSED  = 0u;
        SOC_IN_WINDOW    = 0u;
    }

    /* Step 5: compute and store display/CAN SOC value */
    float_load_buf((const uint8_t *)&SOC_CURRENT);
    float_encode(0u);
    float_load_buf((const uint8_t *)&SOC_CURRENT);
    float_mul_cal ((const volatile uint8_t  *)0x80D0u);
    float_mul_scale((const volatile uint16_t *)0x80D8u);
    float_store((uint8_t *)&SOC_DISPLAY_OUT);

    TX_BUSY_FLAG = 0u;
    return 0u;
}
