/*
 * bms_read_safety_status_a.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009627
 * Original name    : FUN_009627
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Reads a single BQ76952 register using afe_read_register
 *  (FUN_009288) with command byte 0x00.
 *
 *  Confirmed disassembly:
 *    pushw X            ; save param_1 (output buffer pointer)
 *    clr   A            ; command byte = 0x00
 *    callf 0x009288     ; afe_read_register(0x00, param_1)
 *    popw  X
 *    retf
 *
 *  BQ76952 direct command 0x00 = SafetyStatusA (SLUUBY4 Table 4-1)
 *    16-bit register containing the primary protection fault flags:
 *      bit 0  = CUV  (cell undervoltage)
 *      bit 2  = COV  (cell overvoltage)
 *      bit 4  = OCC  (overcurrent in charge)
 *      bit 6  = OCD1 (overcurrent in discharge 1)
 *      bit 8  = OCD2 (overcurrent in discharge 2)
 *      bit 10 = SCD  (short circuit in discharge)
 *      bit 12 = OTC  (overtemperature in charge)
 *      bit 14 = OTD  (overtemperature in discharge)
 *
 *  Called from configure_afe_registers with buf = (uint8_t*)0x01CB.
 *  0x01CB is the "single word config" slot confirmed earlier.
 */

#include <stdint.h>

#define BQ76952_SAFETY_STATUS_A_CMD  0x00u

extern void afe_read_register(uint8_t cmd_byte, uint8_t *out_ptr);

/*
 * read_safety_status_a
 *
 * Reads SafetyStatusA (primary protection fault register) from
 * the BQ76952. Stores 2 bytes at *out_ptr.
 *
 * Called from configure_afe_registers with out_ptr = (uint8_t*)0x01CB.
 */
void read_safety_status_a(uint8_t *out_ptr)
{
    afe_read_register(BQ76952_SAFETY_STATUS_A_CMD, out_ptr);
}
