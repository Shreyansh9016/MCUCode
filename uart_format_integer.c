/*
 * uart_format_integer.c
 *
 * FUN_00ea13 (0x00EA13) — format signed integer into string buffer
 * FUN_00eb58 (0x00EB58) — format unsigned integer into string buffer
 *
 * Both are integer-to-string conversion functions called by
 * uart_format_engine during printf formatting.
 *
 * ---------------------------------------------------------------
 * FUN_00ea13 — uart_format_signed_int
 * ---------------------------------------------------------------
 * Confirmed disassembly:
 *   1. Load int16 value from ctx->value (ptr offset 0x00/0x02)
 *   2. if (value & 0x8000): [negative]
 *      - FUN_00f48c(SP+0x0B) → negate the value (two's complement)
 *      - write '-' (0x2D) to out_ptr; out_ptr++
 *   3. Compute total digits = min(exp_digits + frac_digits, 8)
 *      (confirmed: add A,B; cp A,#8; jrc; ld A,#7)
 *   4. Push (digits+1) as loop count
 *   5. Call FUN_00ED24 (decimal digit extractor) to fill digit buffer
 *   6. Copy digits from buffer to output, skipping leading zeros
 *      if field width > 0
 *
 * ---------------------------------------------------------------
 * FUN_00eb58 — uart_format_unsigned_int
 * ---------------------------------------------------------------
 * Confirmed disassembly (identical structure, no sign handling):
 *   1. Load uint16 value from ctx
 *   2. No negation step
 *   3. Same digits = min(exp+frac, 8) calculation
 *   4. Push (digits+1)
 *   5. Call FUN_00ED24 → fill digit buffer
 *   6. Copy to output
 *
 * FUN_00ED24 is the BCD/binary-to-decimal digit extractor —
 * takes a 16-bit value and fills a buffer with individual digit
 * characters (ASCII '0'–'9'), confirmed by the jra+buffer-walk
 * pattern after the call.
 *
 * Context struct layout (inferred from puVar8 offsets):
 *   [0x00] = value_lo (int/uint low byte)
 *   [0x02] = value_hi
 *   [0x08] = output buffer start
 *   [0x0A] = exp_digits (integer digit count)
 *   [0x0C] = upper limit ptr (past end of output buffer)
 *   [0x0E] = out_ptr (current write position)
 *   [0x1D] = frac_digits
 */
#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
extern void    negate_16bit   (uint8_t *val_ptr);         /* FUN_00F48C */
extern void    extract_digits (uint8_t count,
                               uint8_t *digit_buf,
                               uint16_t value);           /* FUN_00ED24 */

/* Format context (same struct used by uart_printf_format_parser) */
typedef struct {
    int16_t  value;          /* [0x00] the integer to format        */
    uint16_t value_hi;       /* [0x02] high word (32-bit support)   */
    char    *out_buf_start;  /* [0x08] output buffer base           */
    uint8_t  exp_digits;     /* [0x0A] integer digit places         */
    char    *out_buf_end;    /* [0x0C] past end of output buffer    */
    char    *out_ptr;        /* [0x0E] current write position       */
    int16_t  field_width;    /* [0x10] printf field width           */
    char    *src_ptr;        /* [0x12] (unused in this path)        */
    int8_t   char_count;     /* [0x14] working counter              */
    char    *start_ptr;      /* [0x15] output start mark            */
    int8_t   frac_digits;    /* [0x1D] fractional decimal places    */
} fmt_ctx_t;

/*
 * uart_format_signed_int  (FUN_00ea13)
 *
 * Formats a signed 16-bit integer from ctx->value into the
 * output buffer at ctx->out_ptr. Writes a leading '-' for negatives.
 * Returns pointer to one past the last written character.
 */
char *uart_format_signed_int(fmt_ctx_t *ctx)
{
    char    digit_buf[9];
    uint8_t digits;
    bool    negative = false;

    /* Check sign (bit 15 of value) */
    if ((uint16_t)ctx->value & 0x8000u)
    {
        negative = true;
        negate_16bit((uint8_t *)&ctx->value);   /* FUN_00F48C: two's complement */
        *ctx->out_ptr = '-';                     /* write '-' to output          */
        ctx->out_ptr++;
    }

    /* Compute total digit count = min(exp_digits + frac_digits, 8) */
    digits = (uint8_t)(ctx->exp_digits + (uint8_t)ctx->frac_digits);
    if (digits > 7u) digits = 7u;

    /* Extract digits into temporary buffer via BCD extractor */
    extract_digits((uint8_t)(digits + 1u), digit_buf, (uint16_t)ctx->value);

    /* Copy digits to output, respecting field width */
    ctx->char_count = (int8_t)digits;
    ctx->start_ptr  = ctx->out_ptr;

    while (ctx->char_count != 0)
    {
        char ch = digit_buf[digits - (uint8_t)ctx->char_count];

        /* Skip leading zeros if field width specified */
        if (ctx->field_width > 0 && ch == '0' &&
            ctx->char_count > ctx->frac_digits)
        {
            ch = (ctx->out_ptr > ctx->out_buf_start) ? '0' : ' ';
        }

        *ctx->out_ptr = ch;
        ctx->out_ptr++;
        ctx->char_count--;
    }

    return ctx->out_ptr;
}

/*
 * uart_format_unsigned_int  (FUN_00eb58)
 *
 * Identical to uart_format_signed_int but without sign handling.
 * Formats a uint16_t value (ctx->value treated as unsigned).
 */
char *uart_format_unsigned_int(fmt_ctx_t *ctx)
{
    char    digit_buf[9];
    uint8_t digits;

    /* No sign check — treat value as unsigned */

    /* Compute digit count */
    digits = (uint8_t)(ctx->exp_digits + (uint8_t)ctx->frac_digits);
    if (digits > 7u) digits = 7u;

    /* Extract digits */
    extract_digits((uint8_t)(digits + 1u), digit_buf, (uint16_t)ctx->value);

    /* Copy to output */
    ctx->char_count = (int8_t)digits;
    ctx->start_ptr  = ctx->out_ptr;

    while (ctx->char_count != 0)
    {
        char ch = digit_buf[digits - (uint8_t)ctx->char_count];

        if (ctx->field_width > 0 && ch == '0' &&
            ctx->char_count > ctx->frac_digits)
        {
            ch = (ctx->out_ptr > ctx->out_buf_start) ? '0' : ' ';
        }

        *ctx->out_ptr = ch;
        ctx->out_ptr++;
        ctx->char_count--;
    }

    return ctx->out_ptr;
}
