/*
 * compare_accumulator_32bit.c
 *
 * Original address : 0x00F65D  /  FUN_00f65d
 * Function name    : compare_accumulator_32bit
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 * Confirmed disassembly:
 *   ld  A, 0x00        ; A = scratch[0]   (accumulator byte 0)
 *   cp  A, (X)         ; compare with param_1[0]
 *   jrne → 0x00F67C   ; if byte 0 differs → jump to "not equal" path
 *
 *   ld  A, 0x01        ; A = scratch[1]
 *   cp  A, (0x01,X)    ; compare with param_1[1]
 *   jrne → 0x00F674   ; if byte 1 differs → jump to "less/greater" path
 *
 *   ld  A, 0x02        ; A = scratch[2]
 *   cp  A, (0x02,X)    ; compare with param_1[2]
 *   jrne → 0x00F674   ; if byte 2 differs
 *
 *   ld  A, 0x03        ; A = scratch[3]
 *   cp  A, (0x03,X)    ; compare with param_1[3]
 *   jreq → 0x00F67C   ; if all 4 bytes equal → "equal" path
 *
 *   rvf                ; clear V flag before using carry
 *   jrnc → 0x00F67A   ; if scratch[3] >= param_1[3] → bVar2 = 1 (greater)
 *   ld  A, #0xFF       ; else: scratch < param_1 → return 0xFF
 *   retf
 *
 * 0x00F67A path (scratch > param_1, or byte 0 mismatch with scratch > param_1):
 *   bVar2 = 1 (scratch > rhs, return 1)
 *   retf
 *
 * 0x00F67C path (equal, or byte 0 mismatch with scratch == rhs byte 0):
 *   return DAT_000003  (= scratch[3], the last accumulator byte)
 *   retf
 *
 * ---------------------------------------------------------------
 * Return value semantics
 * ---------------------------------------------------------------
 *   0xFF (255)    scratch accumulator  <  *param_1   (less than)
 *   0x00          scratch accumulator  == *param_1   (equal — returns scratch[3])
 *   0x01          scratch accumulator  >  *param_1   (greater than)
 *   scratch[0]    byte 0 differs       (returned from the jrne 0x00F67C path)
 *
 * This maps to a standard three-way compare:
 *   negative → 0xFF (less)
 *   zero     → 0x00 (equal)
 *   positive → 0x01 (greater)
 *
 * Used throughout the firmware wherever a 32-bit float value in
 * the scratch accumulator needs to be tested against a threshold
 * stored in RAM or flash. The returned byte is tested for zero
 * (equal), 0xFF (below threshold), or 0x01 (above threshold).
 *
 * ---------------------------------------------------------------
 * Scratch register layout
 * ---------------------------------------------------------------
 *   RAM[0x00] = scratch byte 0 (MSB of 4-byte float)
 *   RAM[0x01] = scratch byte 1
 *   RAM[0x02] = scratch byte 2
 *   RAM[0x03] = scratch byte 3 (LSB of 4-byte float)
 */

#include <stdint.h>

/* Scratch accumulator bytes (RAM 0x00–0x03) */
#define SCRATCH_B0  (*(volatile uint8_t *)0x0000u)
#define SCRATCH_B1  (*(volatile uint8_t *)0x0001u)
#define SCRATCH_B2  (*(volatile uint8_t *)0x0002u)
#define SCRATCH_B3  (*(volatile uint8_t *)0x0003u)

/*
 * compare_accumulator_32bit
 *
 * Compares the 4-byte scratch accumulator (RAM 0x00–0x03) against
 * the 4-byte value at *param_1, byte by byte from MSB to LSB.
 *
 * Returns:
 *   0xFF  if accumulator  <  *param_1
 *   0x00  if accumulator  == *param_1  (returns scratch[3])
 *   0x01  if accumulator  >  *param_1
 *   scratch[0]  if byte 0 differs but accumulator[0] == param_1[0]
 *               (byte 0 mismatch — returns the accumulator high byte)
 *
 * param_1 : pointer to 4-byte comparison value (threshold in RAM/flash)
 */
uint8_t compare_accumulator_32bit(const uint8_t *param_1)
{
    uint8_t bVar2 = SCRATCH_B0;   /* default return = scratch[0] */

    /* Compare byte 0 (MSB) */
    if (SCRATCH_B0 == param_1[0])
    {
        /* Byte 0 matches — compare bytes 1, 2, 3 */
        if (SCRATCH_B1 == param_1[1] &&
            SCRATCH_B2 == param_1[2] &&
            SCRATCH_B3 == param_1[3])
        {
            /* All 4 bytes equal — return scratch[3] (= 0 for equal floats) */
            return SCRATCH_B3;
        }

        /* At least one of bytes 1-3 differs — determine less/greater */
        if (SCRATCH_B1 < param_1[1]) return 0xFFu;   /* accumulator < rhs */
        if (SCRATCH_B1 > param_1[1]) return 0x01u;   /* accumulator > rhs */
        if (SCRATCH_B2 < param_1[2]) return 0xFFu;
        if (SCRATCH_B2 > param_1[2]) return 0x01u;
        if (SCRATCH_B3 < param_1[3]) return 0xFFu;   /* confirmed: rvf+jrnc path */
        return 0x01u;                                  /* scratch[3] >= param_1[3] */
    }

    /* Byte 0 differs — return scratch[0] (bVar2 set at top) */
    return bVar2;
}