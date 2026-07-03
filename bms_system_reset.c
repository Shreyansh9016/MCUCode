/*
 * bms_system_reset.c
 *
 * Original address : 0x00D673  /  FUN_00d673
 * Function name    : bms_system_reset
 *
 * Confirmed disassembly:
 *   ld   A, 0x505F      ; read TX_BUSY_FLAG (return value)
 *   bcp  A, #0x08       ; test bit 3 (watchdog bit)
 *   mov  0x5064, #0xAE  ; activity marker pulse low
 *   mov  0x5064, #0x56  ; activity marker pulse high
 *   clr  0x4054         ; FLAG_BALANCING_ACTIVE = 0
 *   clr  0x505F         ; TX_BUSY_FLAG = 0
 *   retf
 *
 * Called once from bms_main_loop startup (0x00BACC) to reset
 * the balancing and TX-busy flags before the main loop starts.
 * The activity marker pulse (0xAE → 0x56) is seen in every
 * function that touches hardware — it's a watchdog/activity signal
 * on GPIO pin tied to an external supervisor IC.
 */
#include <stdint.h>

#define TX_BUSY_FLAG          (*(volatile uint8_t *)0x505Fu)
#define FLAG_BALANCING_ACTIVE (*(volatile uint8_t *)0x4054u)
#define ACTIVITY_MARKER       (*(volatile uint8_t *)0x5064u)

uint8_t bms_system_reset(void)
{
    uint8_t prev_tx_busy = TX_BUSY_FLAG;

    /* Pulse activity marker (watchdog kick) */
    ACTIVITY_MARKER = 0xAEu;
    ACTIVITY_MARKER = 0x56u;

    /* Reset balancing and TX-busy flags */
    FLAG_BALANCING_ACTIVE = 0u;
    TX_BUSY_FLAG          = 0u;

    return prev_tx_busy;
}
