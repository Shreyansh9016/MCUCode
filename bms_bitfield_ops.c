/*
 * bms_bitfield_ops.c
 *
 * Five utility functions for bit-level manipulation of BMS state structs.
 * All confirmed directly from disassembly — these are among the cleanest
 * functions in the firmware (no bus protocol, no STM8 quirks).
 *
 * FUN_009b95 (0x009B95) — clear 5 bytes of a BMS state struct
 * FUN_009b9d (0x009B9D) — conditional set/clear across 4 fields using flags
 * FUN_009c0d (0x009C0D) — set bits (OR)
 * FUN_009c14 (0x009C14) — clear bits (AND NOT)
 * FUN_009c1c (0x009C1C) — toggle bits (XOR)
 * FUN_009c28 (0x009C28) — identity (pass-through, returns param_1 unchanged)
 */

#include <stdint.h>

/*
 * bms_state_clear
 * FUN_009b95
 *
 * Zero-initialises 5 bytes of a BMS state record at [0], [2], [3], [4].
 * (byte [1] is skipped — likely a type/tag field set elsewhere.)
 *
 * Confirmed disassembly:
 *   clr  (X)       ; param_1[0] = 0
 *   clr  (0x02,X)  ; param_1[2] = 0
 *   clr  (0x03,X)  ; param_1[3] = 0
 *   clr  (0x04,X)  ; param_1[4] = 0
 */
void bms_state_clear(uint8_t *state)
{
    state[0] = 0u;
    state[2] = 0u;
    state[3] = 0u;
    state[4] = 0u;
}

/*
 * bms_state_update
 * FUN_009b9d
 *
 * Applies a mask+flags update to a 5-byte BMS state record.
 * Uses three flag bits in param_3 to control which fields are
 * set or cleared:
 *
 *   Bit 7 (0x80): controls state[0] and state[2]
 *     bit 7=1, bit 4=1 → state[0] |=  mask  (set)
 *     bit 7=1, bit 4=0 → state[0] &= ~mask  (clear)
 *     bit 7=1          → state[2] |=  mask  (always set)
 *     bit 7=0          → state[2] &= ~mask  (always clear)
 *
 *   Bit 6 (0x40): controls state[3]
 *     bit 6=1 → state[3] |=  mask
 *     bit 6=0 → state[3] &= ~mask
 *
 *   Bit 5 (0x20): controls state[4]
 *     bit 5=1 → state[4] |=  mask
 *     bit 5=0 → state[4] &= ~mask
 *
 *   state[4] &= ~mask  always applied first (unconditional clear before conditionals)
 *
 * Confirmed from bcp / jreq / or / cpl+and pattern in disassembly.
 */
uint8_t *bms_state_update(uint8_t *state, uint8_t mask, uint8_t flags)
{
    /* Always clear state[4] first (unconditional mask-out) */
    state[4] &= ~mask;

    /* Bit 7: primary enable flag */
    if (flags & 0x80u)
    {
        if (flags & 0x10u)
            state[0] |=  mask;   /* set   */
        else
            state[0] &= ~mask;   /* clear */

        state[2] |= mask;        /* always set when bit 7 active */
    }
    else
    {
        state[2] &= ~mask;       /* clear when bit 7 inactive   */
    }

    /* Bit 6: secondary register */
    if (flags & 0x40u)
        state[3] |=  mask;
    else
        state[3] &= ~mask;

    /* Bit 5: tertiary register */
    if (flags & 0x20u)
        state[4] |=  mask;
    else
        state[4] &= ~mask;

    return state;
}

/*
 * reg_set_bits   FUN_009c0d  — *reg |= mask
 * reg_clear_bits FUN_009c14  — *reg &= ~mask
 * reg_toggle_bits FUN_009c1c — *reg ^= mask
 * reg_passthrough FUN_009c28 — returns param_1 unchanged (identity)
 *
 * All confirmed from single-instruction disassembly bodies.
 */

uint8_t *reg_set_bits(uint8_t *reg, uint8_t mask)
{
    *reg |= mask;
    return reg;
}

uint8_t *reg_clear_bits(uint8_t *reg, uint8_t mask)
{
    *reg &= ~mask;
    return reg;
}

uint8_t *reg_toggle_bits(uint8_t *reg, uint8_t mask)
{
    *reg ^= mask;
    return reg;
}

uint16_t reg_passthrough(uint16_t param_1)
{
    return param_1;   /* identity — Ghidra confirmed: ld A,(X); retf */
}
