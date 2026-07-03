/*
 * bms_fault_counter_update.c
 *
 * Original address : 0x00B8D5  /  FUN_00b8d5
 * Function name    : bms_fault_counter_update
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * Called from the interrupt/timer tick (DAT_00024D is a tick flag).
 * Updates persistent fault occurrence counters in RAM and returns
 * the current value of the "load active" / "overcurrent" flag.
 *
 * Confirmed disassembly:
 *   1. Set tick flag DAT_00024D = 1  (signal "tick processed")
 *   2. Set CAN filter mask = 1       (FUN_00a7bb(1))
 *   3. If overcurrent-charge (_DAT_0001F6==1) OR
 *      overcurrent-discharge (_DAT_0001FE==1) flag set:
 *        - if charge overcurrent:    increment counter at 0x0258
 *        - if discharge overcurrent: increment counter at 0x025A
 *      else: clear both counters
 *   4. If load-active flag (_DAT_000200==1):
 *        increment load-duration counter at 0x025C
 *      else: clear load counter
 *   5. Return current value of _DAT_000200 (load/overcurrent state)
 *
 * RAM variables:
 *   0x00024D = tick-processed flag (set by ISR, cleared here)
 *   0x0001F6 = overcurrent-in-charge  flag
 *   0x0001FE = overcurrent-in-discharge flag
 *   0x000200 = load-active / general-overcurrent flag
 *   0x000258 = charge   overcurrent event counter
 *   0x00025A = discharge overcurrent event counter
 *   0x00025C = load-active duration counter
 */

#include <stdint.h>

/* ── Tick/synchronisation flag ── */
#define TICK_FLAG           (*(volatile uint8_t  *)0x024Du)

/* ── BMS fault state flags (set by protection logic) ── */
#define FLAG_OC_CHARGE      (*(volatile uint16_t *)0x01F6u)
#define FLAG_OC_DISCHARGE   (*(volatile uint16_t *)0x01FEu)
#define FLAG_LOAD_ACTIVE    (*(volatile uint16_t *)0x0200u)

/* ── Persistent event counters ── */
#define CTR_OC_CHARGE       (*(volatile uint16_t *)0x0258u)
#define CTR_OC_DISCHARGE    (*(volatile uint16_t *)0x025Au)
#define CTR_LOAD_DURATION   (*(volatile uint16_t *)0x025Cu)

extern void can_set_filter_mask(uint8_t mask);   /* FUN_00A7BB */

/*
 * bms_fault_counter_update
 *
 * Updates overcurrent and load-duration counters for one tick.
 * Returns the current load-active / overcurrent state (0 or 1).
 */
int16_t bms_fault_counter_update(void)
{
    /* Signal that this tick has been processed */
    TICK_FLAG = 1u;

    /* Set CAN filter mask bit 0 */
    can_set_filter_mask(1u);

    /* Update overcurrent counters */
    if (FLAG_OC_CHARGE == 1u || FLAG_OC_DISCHARGE == 1u)
    {
        if (FLAG_OC_CHARGE    == 1u) CTR_OC_CHARGE++;
        if (FLAG_OC_DISCHARGE == 1u) CTR_OC_DISCHARGE++;
    }
    else
    {
        CTR_OC_CHARGE    = 0u;
        CTR_OC_DISCHARGE = 0u;
    }

    /* Update load-duration counter */
    if (FLAG_LOAD_ACTIVE == 1u)
    {
        CTR_LOAD_DURATION++;
    }
    else
    {
        CTR_LOAD_DURATION = 0u;
    }

    return (int16_t)FLAG_LOAD_ACTIVE;
}
