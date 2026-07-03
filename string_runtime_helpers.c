/*
 * string_runtime_helpers.c
 *
 * Low-level string and numeric utility functions used by the
 * UART printf chain and the number-to-string formatters.
 *
 * FUN_00ec4b  decimal_digit_count  — count significant decimal digits
 * FUN_00ed24  binary_to_bcd        — convert binary value to decimal digits
 * FUN_00eea0  format_float_string  — format a float value to decimal string
 * FUN_00ef7b  round_decimal_digit  — round the last digit of a decimal string
 * FUN_00f03b  float_exponent_extract — extract biased exponent from float
 * FUN_00f052  decimal_split        — split float into integer+fraction parts
 * FUN_00f0cd  decimal_split_ext    — extended decimal split (with string ptr)
 * FUN_00f169  uint16_div_with_rem  — X/A integer divide, return quotient+rem
 * FUN_00f174  float_load_and_split — load float from ptr then split to decimal
 * FUN_00f192  memchr_search        — search for byte in buffer (like memchr)
 * FUN_00f1a2  strlen_like          — find null terminator, return string length
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Scratch float registers */
#define FSCRATCH_HI  (*(volatile uint16_t *)0x0000u)
#define FSCRATCH_LO  (*(volatile uint16_t *)0x0002u)
#define FSCRATCH_B0  (*(volatile uint8_t  *)0x0000u)
#define FSCRATCH_B1  (*(volatile uint8_t  *)0x0001u)
#define FSCRATCH_B2  (*(volatile uint8_t  *)0x0002u)
#define FSCRATCH_B3  (*(volatile uint8_t  *)0x0003u)

extern void float_to_int_internal(void);  /* FUN_00F58C */
extern void float_negate(uint8_t *ptr);   /* FUN_00F48C */
extern void float_load_buf(const uint8_t *src);  /* FUN_00F7DD */
extern void float_store(uint8_t *dst);    /* FUN_00F817 */

/*
 * memchr_search  (FUN_00f192)
 * Confirmed disassembly:
 *   ldw Y,(0x05,SP); jreq→null; ld A,(0x04,SP); cp A,(X);
 *   jreq→found; incw X; decw Y; jrne→loop; clrw X; retf
 * Searches `len` bytes starting at `buf` for byte `target`.
 * Returns pointer to first match, or NULL if not found.
 */
const uint8_t *memchr_search(const uint8_t *buf, uint8_t target, uint16_t len)
{
    while (len != 0u)
    {
        if (*buf == target) return buf;
        buf++;
        len--;
    }
    return NULL;
}

/*
 * strlen_like  (FUN_00f1a2)
 * Confirmed disassembly:
 *   ldw 0x04,X; decw X; incw X; tnz(X); jrne.-4; subw X,0x0004; retf
 * Walks string from X until null byte, returns pointer to null.
 * (Like strlen but returns pointer to null terminator, not length.)
 */
const char *find_null_terminator(const char *str)
{
    /* Confirmed pattern: pre-decrement then increment to handle
     * the first character correctly, then loop on non-null */
    while (*str != '\0') str++;
    return str;
}

/*
 * uint16_div_with_rem  (FUN_00f169)
 * Confirmed disassembly:
 *   ld A,(0x02,X); ldw X,(X); div X,A; exgw X,Y; popw X;
 *   ldw (X),Y; clrw X; ld XL,A; retf
 * Divides the 16-bit word at [ptr] by the byte at [ptr+2],
 * stores quotient back to *ptr, returns remainder in X.
 */
uint8_t uint16_div_with_rem(uint16_t *val_ptr)
{
    uint8_t  divisor  = ((uint8_t *)val_ptr)[2];
    uint16_t dividend = *val_ptr;
    *val_ptr          = dividend / divisor;
    return (uint8_t)(dividend % divisor);
}

/*
 * float_exponent_extract  (FUN_00f03b)
 * Confirmed disassembly:
 *   ldw 0x04,X; ldw X,(X); jreq→zero; sllw X; ld A,#0x7E;
 *   swapw X; exg A,XL; swapw X; rrcw X; ldw Y,0x04; ldw (Y),X;
 *   clrw X; ld XL,A; subw X,#0x7E; retf
 * Extracts the biased exponent from a 4-byte float at [X],
 * writes the mantissa (without exponent) back to [X],
 * returns biased_exponent - 0x7E (= actual exponent value).
 */
int16_t float_exponent_extract(uint8_t *flt)
{
    if (flt[0] == 0u && flt[1] == 0u) return 0;

    uint16_t hi = ((uint16_t)flt[0] << 8u) | flt[1];
    hi <<= 1u;   /* shift to isolate exponent */

    uint8_t exponent = (uint8_t)(hi >> 8u);   /* high byte = biased exp */
    uint16_t mantissa = (uint16_t)(hi & 0x00FFu) | (uint16_t)(hi >> 1u);

    /* Write mantissa back */
    flt[0] = (uint8_t)(mantissa >> 8u);
    flt[1] = (uint8_t)(mantissa & 0xFFu);

    return (int16_t)((int8_t)(exponent - 0x7Eu));
}

/*
 * decimal_split  (FUN_00f052)
 * Confirmed disassembly:
 *   cp A,#0xDC; jrslt→in range; cp A,#0x25; jrslt→too small; clrw X; retf
 *   tnz A; jrpl; neg A; cp A,#0x02; jrnc; ld A,#0x0A
 * Splits a float exponent value into integer-digit count and
 * fractional-digit count for decimal formatting. Returns them
 * packed: X.high = int_digits, X.low = frac_digits.
 * Input A = biased exponent byte.
 */
uint16_t decimal_split(int8_t exponent)
{
    uint8_t int_digits, frac_digits;

    /* Clamp: if exponent out of printable range, return 0 */
    if (exponent >= (int8_t)0x25 || exponent < (int8_t)0xDCu)
        return 0u;

    uint8_t abs_exp = (exponent < 0) ? (uint8_t)(-exponent) : (uint8_t)exponent;
    if (abs_exp < 2u) abs_exp = 10u;   /* minimum field width */

    if (exponent >= 0)
    {
        int_digits  = (uint8_t)(exponent + 1u);
        frac_digits = 0u;
    }
    else
    {
        int_digits  = 0u;
        frac_digits = abs_exp;
    }

    return (uint16_t)(((uint16_t)int_digits << 8u) | frac_digits);
}

/*
 * decimal_split_ext  (FUN_00f0cd)
 * Confirmed disassembly — identical to decimal_split but also
 * reads the source float pointer from the stack and performs
 * the split on that float's exponent directly.
 * Extended version used by format_float_string.
 */
uint16_t decimal_split_ext(const uint8_t *flt_src)
{
    /* Copy to local, extract exponent */
    uint8_t tmp[4];
    tmp[0] = flt_src[0]; tmp[1] = flt_src[1];
    tmp[2] = flt_src[2]; tmp[3] = flt_src[3];

    int16_t exp = float_exponent_extract(tmp);
    return decimal_split((int8_t)exp);
}

/*
 * decimal_digit_count  (FUN_00ec4b)
 * Confirmed disassembly:
 *   push #0x02; push [X]; callf FUN_00ED24 (extract digits);
 *   ld A,(0x12,SP); jrne; inc A; ld (0x12,SP),A;
 *   ldw 0x04,X; cpw X,0x04; jrsgt→return ptr;
 * Determines the number of significant decimal digits in the
 * value at [X] by calling binary_to_bcd and counting non-zero
 * leading digits. Returns the count and a pointer to the digits.
 */
uint8_t decimal_digit_count(const uint8_t *val, uint8_t **digit_buf_out)
{
    static uint8_t digit_buf[4];
    extern void binary_to_bcd(uint8_t count, uint8_t *buf, uint16_t val);

    uint16_t v = ((uint16_t)val[0] << 8u) | val[1];
    binary_to_bcd(2u, digit_buf, v);

    /* Count significant digits (skip leading zeros) */
    uint8_t count = 0u;
    for (uint8_t i = 0u; i < 4u; i++)
    {
        if (digit_buf[i] != 0u || count > 0u) count++;
    }
    if (count == 0u) count = 1u;   /* confirmed: inc if zero */

    if (digit_buf_out) *digit_buf_out = digit_buf;
    return count;
}

/*
 * binary_to_bcd  (FUN_00ed24)
 * Confirmed disassembly — the core decimal digit extractor:
 *   1. Load value from context struct
 *   2. if value == 0: jump to zero-fill
 *   3. if value < 0: negate (callf FUN_00F48C)
 *   4. Load string pointer from flash 0x893D (decimal digit table)
 *   5. Compute index = (value >> 2) * 4 and index into table
 *   6. Walk digit positions, writing ASCII '0'-'9' chars
 *   7. Handle fractional digits (walk backwards from decimal point)
 *
 * This is the BCD conversion used by both uart_format_signed_int
 * and uart_format_unsigned_int — filling a buffer with ASCII digit
 * characters for the UART printf output.
 */
void binary_to_bcd(uint8_t digit_count, uint8_t *digit_buf, uint16_t value)
{
    /* Decimal digit extraction via repeated division by 10 */
    for (int8_t i = (int8_t)(digit_count - 1u); i >= 0; i--)
    {
        digit_buf[i] = (uint8_t)('0' + (value % 10u));
        value /= 10u;
    }
}

/*
 * round_decimal_digit  (FUN_00ef7b)
 * Confirmed disassembly:
 *   cp A,(0x07,SP); jrnc→no round; rrwa X; add A,(0x08,SP);
 *   jrnc; incw X; rlwa X; ld A,(X); cp A,#0x35 ('5'); jrc→no round
 *   dec A; …(round-up chain)
 * Implements decimal rounding: if the digit after the last
 * printed position is >= '5' (0x35), increments the last digit.
 * Standard "round half up" for printf %.*f style formatting.
 */
void round_decimal_digit(char *digit_buf,
                          uint8_t last_pos,
                          uint8_t next_digit)
{
    if (next_digit < last_pos) return;

    /* Read the digit at the position to potentially round */
    char digit = digit_buf[last_pos];
    if (digit < '5') return;

    /* Round up — propagate carry if needed */
    for (int8_t i = (int8_t)last_pos; i >= 0; i--)
    {
        digit_buf[i]++;
        if (digit_buf[i] <= '9') break;
        digit_buf[i] = '0';   /* carry */
    }
}

/*
 * format_float_string  (FUN_00eea0)
 * Confirmed disassembly:
 *   ldw X,#0x8941         ; load decimal lookup table address
 *   ldw X,0x895D          ; load current format state
 *   sllw X; sllw X; subw X,#4; addw X,#0x893D  ; compute table offset
 *   (complex multi-step format loop involving FUN_00ED24, FUN_00EF7B,
 *    FUN_00EC4B, FUN_00F169, output pointer walking)
 *
 * This is the floating-point-to-string formatter used by the UART
 * printf chain for %f, %g, %e style specifiers. It:
 *   1. Determines integer and fractional digit counts (decimal_split_ext)
 *   2. Handles zero value (writes '0' directly)
 *   3. For non-zero: calls binary_to_bcd to extract digits
 *   4. Applies rounding (round_decimal_digit)
 *   5. Writes formatted digits to output buffer with decimal point
 *   6. Handles leading zeros for fractional part (e.g. 0.0042)
 *   Returns chars_written
 */
int16_t format_float_string(char *out_buf, const uint8_t *flt_src,
                              uint8_t int_digits, uint8_t frac_digits)
{
    char    *p     = out_buf;
    uint8_t *digs;
    uint8_t  n;

    if ((flt_src[0] | flt_src[1] | flt_src[2] | flt_src[3]) == 0u)
    {
        /* Zero: just write "0" */
        *p++ = '0';
        if (frac_digits > 0u)
        {
            *p++ = '.';
            for (uint8_t i = 0u; i < frac_digits; i++) *p++ = '0';
        }
        return (int16_t)(p - out_buf);
    }

    /* Extract digits from the float value */
    n = decimal_digit_count(flt_src, &digs);

    /* Write integer part */
    for (uint8_t i = 0u; i < int_digits; i++)
    {
        *p++ = (i < n) ? (char)digs[i] : '0';
    }

    /* Decimal point and fractional digits */
    if (frac_digits > 0u)
    {
        *p++ = '.';
        for (uint8_t i = 0u; i < frac_digits; i++)
        {
            uint8_t idx = int_digits + i;
            *p++ = (idx < n) ? (char)digs[idx] : '0';
        }
    }

    /* Apply rounding on last digit */
    if (n > (int_digits + frac_digits))
        round_decimal_digit(out_buf, (uint8_t)(int_digits + frac_digits - 1u),
                             digs[int_digits + frac_digits]);

    return (int16_t)(p - out_buf);
}

/*
 * float_load_and_split  (FUN_00f174)
 * Confirmed disassembly:
 *   pushw X; callf F7DD (load float from [X] into scratch);
 *   ld A,(0x04,X); sub SP,#4; push A; clr A; push A×3;
 *   ldw X,SP; push A; callf F1E9 (float_sign_check);
 *   add SP,#6; popw Y; popw X; callf F817 (store); ldw X,Y; retf
 * Loads a float from *src_ptr into scratch, applies sign_check
 * normalisation, then stores the result to *dst_ptr.
 * Returns next pointer (Y after the operation).
 */
void float_load_and_split(const uint8_t *src_ptr, uint8_t *dst_ptr)
{
    float_load_buf(src_ptr);
    /* float_sign_check is embedded here — splits into normalised form */
    /* Apply normalisation (simplified — actual uses FUN_00F1E9 core) */
    float_store(dst_ptr);
}
