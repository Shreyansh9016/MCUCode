/*
 * bms_read_status_registers.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x0095F6
 * Original name    : FUN_0095f6
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Reads 4 BQ76952 status/protection registers using
 *  afe_read_register (FUN_009288) into a caller-supplied buffer.
 *
 *  Confirmed disassembly call sequence:
 *    ld A,#0x34; callf 0x009288 → reg 0x34 → buf[0:1]
 *    ld A,#0x36; callf 0x009288 → reg 0x36 → buf[2:3]
 *    ld A,#0x38; callf 0x009288 → reg 0x38 → buf[4:5]
 *    ld A,#0x3A; callf 0x009288 → reg 0x3A → buf[6:7]
 *
 *  BQ76952 direct command map (SLUUBY4 Table 4-1):
 *    0x34 = BatteryStatus      (16-bit pack status flags)
 *    0x36 = AlarmStatus        (16-bit alarm flags)
 *    0x38 = AlarmRawStatus     (16-bit raw alarm flags)
 *    0x3A = PowerStatus        (16-bit power path status)
 *
 *  Called from configure_afe_registers with buf = (uint8_t*)0x01C3.
 *  This is the 8-byte AFE conversion result buffer confirmed earlier.
 */

#include <stdint.h>

#define BQ76952_BATTERY_STATUS_CMD  0x34u
#define BQ76952_ALARM_STATUS_CMD    0x36u
#define BQ76952_ALARM_RAW_CMD       0x38u
#define BQ76952_POWER_STATUS_CMD    0x3Au

#define STATUS_REG_COUNT    4u
#define BYTES_PER_STATUS    2u
#define STATUS_BUF_SIZE     (STATUS_REG_COUNT * BYTES_PER_STATUS)  /* 8 */

extern void afe_read_register(uint8_t cmd_byte, uint8_t *out_ptr);

/*
 * read_status_registers
 *
 * Reads BatteryStatus, AlarmStatus, AlarmRawStatus, PowerStatus
 * from the BQ76952 into buf[0:7] (2 bytes each).
 *
 * buf must point to a buffer of at least 8 bytes.
 *
 * Called from configure_afe_registers with buf = (uint8_t*)0x01C3.
 */
void read_status_registers(uint8_t *buf)
{
    static const uint8_t cmds[STATUS_REG_COUNT] = {
        BQ76952_BATTERY_STATUS_CMD,
        BQ76952_ALARM_STATUS_CMD,
        BQ76952_ALARM_RAW_CMD,
        BQ76952_POWER_STATUS_CMD
    };
    uint8_t i;

    for (i = 0u; i < STATUS_REG_COUNT; i++)
    {
        afe_read_register(cmds[i], buf + (i * BYTES_PER_STATUS));
    }
}
