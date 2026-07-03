/*
 * uart_send_bms_status.c
 *
 * Original address : 0x00BB7E  /  FUN_00bb7e
 * Function name    : uart_send_bms_status
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * Sends a comprehensive BMS status report over UART using the
 * firmware's debug string table. Called every 4 ticks from
 * bms_main_loop as the debug telemetry channel.
 *
 * The function:
 *   1. Copies measurement data from AFE output struct into a
 *      local 76-byte stack buffer (using our previously decoded
 *      fetch_pair functions and copy helpers)
 *   2. Calls FUN_00E3B1 (the string-format-and-print function
 *      from our initial analysis) ~26+ times with string addresses
 *      from the flash string table (0x8260–0x8900 region)
 *   3. Each call to FUN_00E3B1 formats one BMS measurement value
 *      using the corresponding printf-style string from the table
 *      (e.g. "cellbalancingstatus %d", "BatteryVoltage %02d.%02d")
 *   4. For the pack current (local_3d / uStackY_3c):
 *      - if positive (bit 15 = 0): prints as decimal percent
 *        (value / 100 and value % 100)
 *      - if negative (bit 15 = 1): prints as negative value
 *        using FUN_00f829 (abs/negate helper) then / 100
 *   5. Loads timing/counter values from RAM 0x0264, 0x0246, 0x0244
 *      and prints them as the last output fields
 *
 * FUN_00E3B1 = the string-format-and-transmit helper (prints one
 * formatted value using its string from the flash string table,
 * then calls uart_send_byte for each character).
 *
 * String table addresses used (from flash 0x8260 region):
 *   All the debug strings we extracted early on:
 *   "thermalrunawayfault %d", "chargefetstatus %d",
 *   "dischargefetstatus %d", "cellbalancingstatus %d",
 *   "permanentfailurestatus %d", "cell1voltage"..."cell16voltage",
 *   "BatteryVoltage %02d.%02d", "StateOfCharge %d.%d",
 *   etc. — the complete BMS parameter set.
 */

#include <stdint.h>
#include <stddef.h>

/* ── Stack buffer size — confirmed from sub SP,#0x2C (44 bytes allocated) ── */
#define UART_STATUS_BUF_SIZE  76u   /* local stack frame covers ~76 bytes    */

/* ── Flash string table pointer type ── */
typedef const char * flash_str_t;

/* ── String-format-and-print helper — our early-analysis confirmed function ── */
extern void uart_print_formatted(flash_str_t str_addr, ...);  /* FUN_00E3B1 */

/* ── Measurement data copy helpers ── */
extern void fetch_pair_channel_a(uint8_t *p1, uint8_t *p2);   /* FUN_00971B */
extern void copy_raw_block_128bytes(uint8_t *dst);             /* FUN_0096EF */
extern void copy_raw_block_32bytes(uint8_t *dst);              /* FUN_009705 */

/* ── Float arithmetic helpers ── */
extern uint16_t FUN_00f829(uint16_t divisor, uint16_t dividend); /* abs/div */
extern void     FUN_00f82f(uint16_t divisor, uint16_t val);       /* divide  */

/* ── Runtime counter values to print ── */
#define RUNTIME_CTR_0  (*(volatile uint16_t *)0x0264u)
#define RUNTIME_CTR_1  (*(volatile uint16_t *)0x0246u)
#define RUNTIME_CTR_2  (*(volatile uint16_t *)0x0244u)

/* ── Calibration data from flash (same table as CAN TX frames) ── */
#define CAL_BASE       ((const volatile uint8_t *)0x406Du)

void uart_send_bms_status(void)
{
    uint8_t  buf[UART_STATUS_BUF_SIZE];
    int16_t  pack_current;
    uint8_t  curr_hi, curr_lo;
    uint16_t abs_curr;

    /* ── Step 1: Fill local buffer from AFE output struct ── */
    fetch_pair_channel_a(buf + 0x1Fu, buf + 0x21u);   /* pair ch A */
    copy_raw_block_128bytes(buf + 0x2Cu);              /* raw block */
    copy_raw_block_32bytes(buf + 0x24u);               /* conv result */

    /* ── Step 2: Print calibration/config values ── */
    uart_print_formatted((flash_str_t)0x8903u);        /* header line        */

    uart_print_formatted((flash_str_t)0x88EEu,
        (uint8_t)CAL_BASE[5],                          /* cal byte 5 */
        (uint8_t)CAL_BASE[6],                          /* cal byte 6 */
        (uint8_t)CAL_BASE[7]);                         /* cal byte 7 */

    uart_print_formatted((flash_str_t)0x88BCu,
        (uint8_t)CAL_BASE[4],                          /* cal byte 4 */
        (uint8_t)CAL_BASE[2],                          /* cal byte 2 */
        (uint8_t)CAL_BASE[1]);                         /* cal byte 1 */

    uart_print_formatted((flash_str_t)0x88A3u,
        (uint8_t)CAL_BASE[11],                         /* cal byte 11 */
        (uint8_t)CAL_BASE[12],                         /* cal byte 12 */
        (uint8_t)CAL_BASE[13]);                        /* cal byte 13 */

    uart_print_formatted((flash_str_t)0x886Du,
        (uint8_t)CAL_BASE[10],                         /* cal byte 10 */
        (uint8_t)CAL_BASE[9],                          /* cal byte 9  */
        (uint8_t)CAL_BASE[8]);                         /* cal byte 8  */

    /* ── Step 3: Print 16 cell voltages and pack measurements ── */
    uart_print_formatted((flash_str_t)0x884Cu);
    uart_print_formatted((flash_str_t)0x883Au);
    uart_print_formatted((flash_str_t)0x8828u);
    uart_print_formatted((flash_str_t)0x8816u);
    uart_print_formatted((flash_str_t)0x8804u);
    uart_print_formatted((flash_str_t)0x87F2u);
    uart_print_formatted((flash_str_t)0x87E0u);
    uart_print_formatted((flash_str_t)0x87CEu);
    uart_print_formatted((flash_str_t)0x87BCu);
    uart_print_formatted((flash_str_t)0x87AAu);
    uart_print_formatted((flash_str_t)0x8797u);
    uart_print_formatted((flash_str_t)0x8784u);
    uart_print_formatted((flash_str_t)0x8771u);
    uart_print_formatted((flash_str_t)0x875Eu);
    uart_print_formatted((flash_str_t)0x874Bu);
    uart_print_formatted((flash_str_t)0x8738u);
    uart_print_formatted((flash_str_t)0x8725u);
    uart_print_formatted((flash_str_t)0x8703u);
    uart_print_formatted((flash_str_t)0x86E1u);

    /* ── Step 4: SOC print (integer % 100, / 100) ── */
    uart_print_formatted((flash_str_t)0x86C6u);

    /* ── Step 5: Pack current — signed value handling ── */
    pack_current = (int16_t)((buf[0x3Cu] << 8u) | buf[0x3Du]);

    if ((pack_current & (int16_t)0x8000) == 0)
    {
        /* Positive current: print as D.dd (hundredths) */
        curr_hi = (uint8_t)((uint16_t)pack_current / 100u);
        curr_lo = (uint8_t)((uint16_t)pack_current % 100u);
        uart_print_formatted((flash_str_t)0x86ACu, curr_hi, curr_lo);

        /* Print zero padding and clear fields */
        uart_print_formatted((flash_str_t)0x868Fu, 0u, 0u, 0u, 0u);
    }
    else
    {
        /* Negative current: negate and print as -D.dd */
        int16_t neg = (int16_t)(-1 - pack_current);

        uart_print_formatted((flash_str_t)0x86ACu, 7u);

        abs_curr = FUN_00f829(100u, (uint16_t)neg);
        uart_print_formatted((flash_str_t)0x868Fu,
            (uint8_t)(abs_curr >> 8u),
            (uint8_t)(abs_curr & 0xFFu));
    }

    /* ── Step 6: Runtime counters ── */
    uart_print_formatted((flash_str_t)0x867Cu, (uint16_t)RUNTIME_CTR_0);

    /* Set activity marker */
    *(volatile uint8_t *)0x5064u = 0x56u;

    uart_print_formatted((flash_str_t)0x8669u,
        (uint16_t)RUNTIME_CTR_1,
        (uint16_t)RUNTIME_CTR_2);

    /* Clear TX-busy flag */
    *(volatile uint8_t *)0x505Fu = 0u;
}
