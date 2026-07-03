/*
 * float_accum_add.c
 *
 * Original address : 0x00F454  /  FUN_00f454
 * Function name    : float_accum_add
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 * Confirmed disassembly:
 *   pushw X              ; save param_1 pointer
 *   callf 0x00F2D1       ; float_assign_from_ptr(param_1)
 *                        ;   → copies 4-byte float at [param_1]
 *                        ;     into scratch registers 0x00-0x03
 *                        ;     (conditional: only if scratch is zero)
 *   popw  X              ; restore param_1 pointer
 *   jpf   0x00F817       ; float_accumulator_store(param_1)
 *                        ;   → copies scratch 0x00-0x03 back
 *                        ;     out to [param_1]
 *   retf (via jpf tail-call)
 *
 * The Ghidra CONCAT11(0x59, uStack_1) in the decompilation is
 * an artifact of the STM8 far-call return address encoding —
 * 0x59 is a byte from the instruction stream, not a data value.
 * The real operation is simply: restore X (param_1), then call
 * float_accumulator_store with it.
 *
 * Combined effect:
 *   1. float_assign_from_ptr(*param_1) → load [param_1] into scratch
 *      (the "assign if scratch is zero" semantics of FUN_00F2D1
 *       mean this acts as an accumulator initialise-if-empty step)
 *   2. float_accumulator_store(*param_1) → write scratch back to [param_1]
 *
 * In the BMS context this is used to initialise the float scratch
 * accumulator from a RAM slot and immediately mirror the result back,
 * making it the "load and accumulate" primitive used by the SOC
 * coulomb-counting integration in bms_cell_balancing_and_soc.
 *
 * ---------------------------------------------------------------
 * Dependencies
 * ---------------------------------------------------------------
 *   float_assign_from_ptr  (FUN_00F2D1) — conditional load into scratch
 *   float_accumulator_store (FUN_00F817) — store scratch to [ptr]
 */

#include <stdint.h>

extern void float_assign_from_ptr   (const uint8_t *src);  /* FUN_00F2D1 */
extern void float_accumulator_store (uint8_t       *dst);  /* FUN_00F817 */

/*
 * float_accum_add
 *
 * Loads the 4-byte float value at *param_1 into the scratch
 * accumulator (if accumulator is currently zero), then writes
 * the accumulator result back to *param_1.
 *
 * param_1 : pointer to a 4-byte float in RAM
 *           (both source and destination of the operation)
 */
void float_accum_add(uint8_t *param_1)
{
    /* Step 1: assign [param_1] into scratch accumulator
     *         (FUN_00F2D1 — conditional assign: loads only if scratch == 0) */
    float_assign_from_ptr(param_1);

    /* Step 2: write scratch accumulator result back to [param_1]
     *         (FUN_00F817 — unconditional store) */
    float_accumulator_store(param_1);
}