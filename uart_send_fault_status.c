/*
 * uart_send_fault_status.c
 *
 * Original address : 0x00BE17  /  FUN_00be17
 * Function name    : uart_send_fault_status
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * Sends the complete BMS fault and protection status over UART,
 * printing every individual fault flag as a named field.
 * Called from bms_main_loop's periodic cycle.
 *
 * This is the UART counterpart to the CAN frames 0xD7/0xD8/0xD9
 * that we decoded in can_transmit_bms_frames — same data, human-
 * readable format over UART instead of packed bytes over CAN.
 *
 * Step 1: Copy safety status registers into local buffer.
 *   Uses the same fetch_pair / copy helpers as the CAN TX:
 *   FUN_00971B (fetch_pair_channel_a) → local vars bVar6, bVar5
 *   FUN_009734 (fetch_pair_channel_b) → bVar4
 *   FUN_00974D (fetch_pair_channel_c) → local_12
 *   FUN_0097CA (copy_pack_word_01db)
 *   FUN_0097EB (copy_pack_word_01e0)
 *   FUN_0097D5 (copy_status_byte_01dd)
 *
 * Step 2: Print fault flags — each flag gets its own named string.
 *   The bit-extraction matches exactly what we found in CAN frame 0xD7:
 *
 *   local_8  bit 3   → "overvoltagefault %d"         (string 0x84D3)
 *   local_9  bit 2   → "undervoltagefault %d"        (string 0x84BC)
 *   local_a  bit 4   → "overcurrentfault %d"         (string 0x849B)
 *   local_b  bit 5   → "overcurrentdischarge1 %d"   (string 0x8475)
 *   local_c  bit 6   → "overcurrentdischarge2 %d"   (string 0x844F)
 *   unconditional    → "shortcircuit %d"             (string 0x8429)
 *   bVar6   bit 7   → "overtemp charge %d"          (string 0x83FA or 0x8412)
 *   bVar6   bit 7   → "overtemp discharge %d"       (string 0x83DD)
 *   bVar5   bit 6   → "undertemp charge %d"         (string 0x83BB)
 *   bVar4   bit 5   → "undertemp discharge %d"      (string 0x8392)
 *   bVar3   bit 4   → "permanentfailure %d"         (string 0x836C)
 *   local_12 bit 2  → "thermalrunaway %d"           (string 0x8349)
 *   unconditional   → "safetystatusA %d"            (string 0x831F)
 *   overtemp again  → "safetystatusB %d"            (string 0x82F8)
 *
 *   Then charge/discharge status:
 *   unconditional   → "chargefetstatus %d"          (string 0x82E2)
 *   unconditional   → "dischargefetstatus %d"       (string 0x82C9)
 *   _DAT_0001E4     → "cellbalancingstatus %d"      (string 0x82B1)
 *   _DAT_0001E6     → "BatteryVoltage %02d.%02d"   (string 0x829C)
 *   all-fault-clear → "permanentfailurestatus %d"  (string 0x8283)
 *   unconditional   → [final status line]           (string 0x8260)
 *
 * NOTE: Ghidra shows "WARNING: Removing unreachable block" — the
 * compiler-generated code has dead blocks from optimisation. The
 * else branches are identical to the if branches (same function
 * called regardless), meaning the branch conditions only affect
 * which *value* is passed to uart_print_formatted, not which
 * string is printed. This is consistent with "print flag name +
 * value" where value = 0 or 1 based on the flag.
 */

#include <stdint.h>
#include <stdbool.h>

typedef const char * flash_str_t;

extern void    uart_print_formatted(flash_str_t str, ...);     /* FUN_00E3B1 */

extern void    fetch_pair_channel_a(uint8_t *p1, uint8_t *p2); /* FUN_00971B */
extern void    fetch_pair_channel_b(uint8_t *p1, uint8_t *p2); /* FUN_009734 */
extern void    fetch_pair_channel_c(uint8_t *p1, uint8_t *p2); /* FUN_00974D */
extern void    copy_pack_word_01db (uint8_t *dst);             /* FUN_0097CA */
extern void    copy_pack_word_01e0 (uint8_t *dst);             /* FUN_0097EB */
extern void    copy_status_byte_01dd(uint8_t *dst);            /* FUN_0097D5 */

/* ── BMS state flags (fault conditions) ── */
#define FLAG_CHG_OC_FAULT   (*(volatile uint16_t *)0x01E4u)
#define FLAG_DSG_OC_FAULT   (*(volatile uint16_t *)0x01E6u)
#define FLAG_OC_CHARGE_2    (*(volatile uint16_t *)0x01EEu)
#define FLAG_OC_CHARGE_3    (*(volatile uint16_t *)0x01F6u)
#define FLAG_OC_DSG_2       (*(volatile uint16_t *)0x01FEu)
#define FLAG_OC_DSG_3       (*(volatile uint16_t *)0x01F8u)

uint16_t uart_send_fault_status(void)
{
    /* Local safety status register copies */
    uint8_t local_8, local_9, local_a, local_b, local_c;
    uint8_t bVar3, bVar4, bVar5, bVar6, local_12;
    uint8_t pack_word_db[8], pack_word_e0[8], status_dd[4];

    /* ── Step 1: Load safety status registers ── */
    fetch_pair_channel_a (&bVar6,    &bVar5);
    fetch_pair_channel_b (&bVar4,    &local_12);
    fetch_pair_channel_c (&local_b,  &bVar3);
    copy_pack_word_01db  (pack_word_db);
    copy_pack_word_01e0  (pack_word_e0);
    copy_status_byte_01dd(status_dd);

    /* Map to individual fault bytes */
    local_8  = pack_word_db[0];
    local_9  = pack_word_db[2];
    local_a  = pack_word_db[4];
    local_c  = pack_word_e0[0];

    /* ── Step 2: Print every fault flag by name with its value ── */

    /* Print header */
    uart_print_formatted((flash_str_t)0x84E9u);

    /* Overvoltage fault — local_8 bit 3 */
    uart_print_formatted((flash_str_t)0x84D3u, (local_8 >> 3u) & 1u);

    /* Undervoltage fault — local_9 bit 2 */
    uart_print_formatted((flash_str_t)0x84BCu, (local_9 >> 2u) & 1u);

    /* Overcurrent charge — local_a bit 4 */
    uart_print_formatted((flash_str_t)0x849Bu, (local_a >> 4u) & 1u);

    /* Overcurrent discharge 1 — local_b bit 5 */
    uart_print_formatted((flash_str_t)0x8475u, (local_b >> 5u) & 1u);

    /* Overcurrent discharge 2 — local_c bit 6 */
    uart_print_formatted((flash_str_t)0x844Fu, (local_c >> 6u) & 1u);

    /* Short circuit (unconditional) */
    uart_print_formatted((flash_str_t)0x8429u);

    /* Overtemperature charge — bVar6 bit 7 */
    uart_print_formatted((flash_str_t)((bVar6 & 0x80u) ? 0x8412u : 0x83FAu),
                         (bVar6 & 0x80u) != 0u);

    /* Overtemperature discharge — bVar6 bit 7 (same flag, different string) */
    uart_print_formatted((flash_str_t)0x83DDu, (bVar6 & 0x80u) != 0u);

    /* Undertemperature charge — bVar5 bit 6 */
    uart_print_formatted((flash_str_t)0x83BBu, (bVar5 >> 6u) & 1u);

    /* Undertemperature discharge — bVar4 bit 5 */
    uart_print_formatted((flash_str_t)0x8392u, (bVar4 >> 5u) & 1u);

    /* Permanent failure — bVar3 bit 4 */
    uart_print_formatted((flash_str_t)0x836Cu, (bVar3 >> 4u) & 1u);

    /* Thermal runaway — local_12 bit 2 */
    uart_print_formatted((flash_str_t)0x8349u, (local_12 >> 2u) & 1u);

    /* Safety status A (unconditional raw byte) */
    uart_print_formatted((flash_str_t)0x831Fu);

    /* Safety status B */
    uart_print_formatted((flash_str_t)0x82F8u, (bVar6 & 0x80u) != 0u);

    /* Charge/discharge FET status */
    uart_print_formatted((flash_str_t)0x82E2u);
    uart_print_formatted((flash_str_t)0x82C9u);

    /* Cell balancing status */
    uart_print_formatted((flash_str_t)0x82B1u, FLAG_CHG_OC_FAULT == 0u ? 0u : 1u);

    /* Battery voltage (charge/discharge enable) */
    uart_print_formatted((flash_str_t)0x829Cu, FLAG_DSG_OC_FAULT == 0u ? 0u : 1u);

    /* All-clear or any-fault indicator */
    {
        uint8_t any_fault = (FLAG_OC_CHARGE_2 || FLAG_OC_CHARGE_3 ||
                             FLAG_OC_DSG_2    || FLAG_OC_DSG_3) ? 1u : 0u;
        uart_print_formatted((flash_str_t)0x8283u, any_fault);
    }

    /* Final status line */
    uart_print_formatted((flash_str_t)0x8260u);

    return 0x5400u;   /* return value unused by caller */
}
