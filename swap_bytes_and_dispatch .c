#include <stdint.h>

extern void byte_swapped_handler(const uint8_t *swapped_value);  /* was FUN_00f1e9 */

/*
 * swap_bytes_and_dispatch
 * ---------------------------------------------------------------
 * Reads a 4-byte value from src (passed in the X register - not
 * visible as a normal parameter) and reverses its byte order
 * before handing it to byte_swapped_handler. Likely an endianness
 * conversion between the internal accumulator format and an
 * external representation (table storage, comms protocol, etc.).
 * Exact purpose depends on byte_swapped_handler's behavior.
 */
void swap_bytes_and_dispatch(const uint8_t *src /* in_X */)
{
    uint8_t reversed[4];

    reversed[3] = src[3];
    reversed[2] = src[2];
    reversed[1] = src[1];
    reversed[0] = src[0];

    byte_swapped_handler(reversed);
}