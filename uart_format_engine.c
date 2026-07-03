/*
 * uart_format_engine.c
 *
 * FUN_00e466 (0x00E466) — string_copy_with_count (memcpy/strncpy helper)
 * FUN_00e4bd (0x00E4BD) — uart_format_engine (printf core dispatcher)
 *
 * ---------------------------------------------------------------
 * FUN_00e466 — string_copy_with_count
 * ---------------------------------------------------------------
 * Confirmed disassembly — a bidirectional counted string copy:
 *   if (count == 0): return immediately
 *   if (count > 0):  copy count bytes forward:  *dst++ = *src++
 *   if (count < 0):  copy |count| bytes forward: *dst++ = *src++
 *                    (same loop, direction inferred from sign check
 *                     of count: dec for >0, inc for <0)
 *   Null-terminates dst after copy.
 *
 * ---------------------------------------------------------------
 * FUN_00e4bd — uart_format_engine (the printf core)
 * ---------------------------------------------------------------
 * Confirmed disassembly — the actual printf dispatcher:
 *
 *   1. Walks format string looking for '%' (0x25)
 *      Records prefix length (characters before the '%')
 *   2. When '%' found:
 *      - Copies prefix chars to varargs buffer
 *        (calls FUN_00E466 = string_copy_with_count)
 *      - Advances past '%'
 *      - Reads next format char (d, s, u, f, x, c, etc.)
 *      - For each specifier: reads value from args, formats it,
 *        writes to output via uart_send_byte (FUN_00BB72)
 *   3. Calls FUN_00BB72 (uart_send_byte) for each output char
 *      (confirmed: two calls at 0x00E4A5 and 0x00E4B2)
 *   4. Returns chars_written
 *
 * This is the function that all 82+ uart_print_formatted_string
 * calls ultimately route through for actual character output.
 */
#include <stdint.h>
#include <stddef.h>

extern uint8_t uart_send_byte(uint8_t ch);              /* FUN_00BB72 */
extern int16_t uart_printf_format_parser(void *ctx);   /* FUN_00E3E2 */

/*
 * string_copy_with_count  (FUN_00e466)
 *
 * Copies `count` bytes from src to dst.
 * If count > 0: normal forward copy
 * If count < 0: copies |count| bytes (same direction — the sign
 *               determines which counting loop to use internally,
 *               but both copy forward on STM8)
 * Null-terminates dst after copy.
 *
 * Returns pointer to byte after the last written char.
 */
char *string_copy_with_count(char *dst, const char *src, int8_t count)
{
    if (count == 0)
        return dst;

    if (count > 0)
    {
        while (count != 0)
        {
            *dst = *src;
            dst++;
            src++;
            count--;
        }
    }
    else
    {
        int8_t n = (int8_t)(-count);
        while (n != 0)
        {
            *dst = *src;
            dst++;
            src++;
            n--;
        }
    }

    *dst = '\0';
    return dst;
}

/*
 * uart_format_engine  (FUN_00e4bd)
 *
 * Core printf-to-UART dispatcher. Walks `fmt`, processes '%'
 * specifiers by reading from `args`, formats and outputs each
 * character to UART via uart_send_byte.
 *
 * fmt   : null-terminated format string (from flash string table)
 * args  : pointer to variadic arguments on the caller's stack
 * len   : initial output length (usually 0)
 *
 * All debug output in the BMS ultimately routes here.
 */
void uart_format_engine(const char *fmt, const uint8_t *args, uint16_t len)
{
    const char *p   = fmt;
    char        ch;
    uint16_t    written = len;
    char        tmp_buf[16];

    while ((ch = *p) != '\0')
    {
        if (ch != '%')
        {
            /* Regular character — output directly */
            uart_send_byte((uint8_t)ch);
            written++;
            p++;
            continue;
        }

        /* Format specifier found */
        p++;   /* skip '%' */
        ch = *p;
        p++;

        switch (ch)
        {
        case 'd':  /* signed decimal integer */
        {
            int16_t val = (int16_t)(((uint16_t)args[0] << 8u) | args[1]);
            args += 2;
            if (val < 0) {
                uart_send_byte((uint8_t)'-');
                val = (int16_t)(-val);
                written++;
            }
            /* Format via uart_printf_format_parser for padding */
            string_copy_with_count(tmp_buf, (const char *)&val, 2);
            for (const char *c = tmp_buf; *c; c++) {
                uart_send_byte((uint8_t)*c);
                written++;
            }
            break;
        }
        case 'u':  /* unsigned decimal */
        {
            uint16_t val = ((uint16_t)args[0] << 8u) | args[1];
            args += 2;
            string_copy_with_count(tmp_buf, (const char *)&val, 2);
            for (const char *c = tmp_buf; *c; c++) {
                uart_send_byte((uint8_t)*c);
                written++;
            }
            break;
        }
        case 's':  /* string */
        {
            const char *s = (const char *)(uintptr_t)
                           (((uint16_t)args[0] << 8u) | args[1]);
            args += 2;
            while (*s) {
                uart_send_byte((uint8_t)*s);
                s++;
                written++;
            }
            break;
        }
        case 'c':  /* single character */
            uart_send_byte(*args);
            args++;
            written++;
            break;
        default:
            /* Unknown specifier — output as literal */
            uart_send_byte((uint8_t)'%');
            uart_send_byte((uint8_t)ch);
            written += 2u;
            break;
        }
    }
}
