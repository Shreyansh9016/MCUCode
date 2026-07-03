/*
 * bms_read_all_temperatures.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009577
 * Original name    : FUN_009577
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Reads 10 temperature sensor values from the BQ76952 AFE
 *  using afe_read_register (FUN_009288), storing results
 *  sequentially into the caller-supplied buffer (2 bytes each = 20 bytes).
 *
 *  Confirmed disassembly call sequence:
 *    ld A,#0x68; callf 0x009288 → TS1  → buf[0:1]
 *    ld A,#0x6A; callf 0x009288 → TS2  → buf[2:3]
 *    ld A,#0x6C; callf 0x009288 → TS3  → buf[4:5]
 *    ld A,#0x6E; callf 0x009288 → TS4  → buf[6:7]
 *    ld A,#0x70; callf 0x009288 → TS5  → buf[8:9]
 *    ld A,#0x72; callf 0x009288 → TS6  → buf[10:11]
 *    ld A,#0x74; callf 0x009288 → TS7  → buf[12:13]
 *    ld A,#0x76; callf 0x009288 → TS8  → buf[14:15]
 *    ld A,#0x78; callf 0x009288 → TS9  → buf[16:17]
 *    ld A,#0x7A; callf 0x009288 → TS10 → buf[18:19]
 *
 *  Register commands 0x68–0x7A match exactly BQ76952 direct
 *  command map (SLUUBY4 Table 4-1):
 *    0x68 = TS1Temperature, 0x6A = TS2Temperature, ...
 *    0x7A = TS10Temperature
 *  Each returns a 16-bit value in 0.1K units (add offset for °C).
 *
 *  Buffer pointer advances +2 per call (incw X; incw X pattern).
 *
 *  Called from configure_afe_registers with buf = (uint8_t*)0x01AF.
 */

#include <stdint.h>

#define BQ76952_TS1_TEMP_CMD    0x68u   /* TS1Temperature direct cmd  */
#define BQ76952_TS_CMD_STEP     0x02u   /* +2 per sensor              */
#define TEMP_SENSOR_COUNT       10u
#define BYTES_PER_TEMP          2u      /* 16-bit temperature value   */
#define TEMP_BUF_SIZE           (TEMP_SENSOR_COUNT * BYTES_PER_TEMP)  /* 20 */

extern void afe_read_register(uint8_t cmd_byte, uint8_t *out_ptr);  /* FUN_009288 */

/*
 * read_all_temperatures
 *
 * Reads all 10 temperature sensor values from the BQ76952 into buf[].
 * Each value is 2 bytes (uint16_t, 0.1K per LSB).
 * Subtract 2731 from the raw value to convert to 0.1°C.
 *
 * buf must point to a buffer of at least 20 bytes (10 sensors × 2).
 *
 * Called from configure_afe_registers with buf = (uint8_t*)0x01AF.
 */
void read_all_temperatures(uint8_t *buf)
{
    uint8_t  sensor;
    uint8_t  cmd = BQ76952_TS1_TEMP_CMD;
    uint8_t *ptr = buf;

    for (sensor = 0u; sensor < TEMP_SENSOR_COUNT; sensor++)
    {
        afe_read_register(cmd, ptr);
        cmd += BQ76952_TS_CMD_STEP;
        ptr += BYTES_PER_TEMP;
    }
}
