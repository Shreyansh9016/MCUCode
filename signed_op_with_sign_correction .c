#include <stdint.h>

extern uint8_t sign_flags;   /* DAT_000004 */
extern void unknown_op_f1d0(int8_t a, const uint8_t *b);   /* FUN_00f1d0 - not yet analyzed */
extern void negate_accumulator(void);                       /* FUN_00f752 */

/*
 * signed_op_with_sign_correction
 * ---------------------------------------------------------------
 * Byte-reverses a 4-byte input, passes it to an unanalyzed
 * operation (unknown_op_f1d0 - likely a signed multiply or
 * divide), then negates the accumulator unless the resulting
 * sign_flags value indicates no correction is needed (0 or 3).
 * This wrapper is the sign-correction stage of a larger signed
 * arithmetic routine.
 */
void signed_op_with_sign_correction(const char *value)
{
    uint8_t reversed[4];
    reversed[3] = value[3];
    reversed[2] = value[2];
    reversed[1] = value[1];
    reversed[0] = value[0];

    unknown_op_f1d0(reversed[0], reversed);

    if (sign_flags != 0 && sign_flags != 3) {
        negate_accumulator();
    }
}