#include <stdint.h>

typedef void (*state_handler_fn)(void);

/*
 * state_dispatch_jump_table
 * ---------------------------------------------------------------
 * Classic compiler-generated switch/case jump table for STM8.
 * table_base is a 24-bit far pointer to a table of 16-bit signed
 * offsets, one per case (2 bytes each). Looks up the offset for
 * case_index, sign-extends it to 24 bits, adds it to table_base,
 * and calls the resulting address.
 *
 * STRONG CANDIDATE for the BMS state-machine dispatcher - each
 * table entry likely corresponds to one of the 7 known states.
 * Finding the call site(s) that invoke this with a specific
 * table_base will reveal all state-handler function addresses.
 */
void state_dispatch_jump_table(uint8_t case_index, uintptr_t table_base)
{
    const uint8_t *entry = (const uint8_t *)(table_base + (case_index * 2));
    int16_t offset = (int16_t)((entry[0] << 8) | entry[1]);   /* signed 16-bit table entry */

    uintptr_t target = table_base + (int32_t)offset;   /* sign-extended add */

    ((state_handler_fn)target)();
}