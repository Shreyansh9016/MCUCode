/*
 * uart_print_string_wrapper.c
 *
 * FUN_00e3b1 (0x00E3B1) — printf-to-UART wrapper (confirmed from our
 *                          initial analysis as the most-called function: 82×)
 * FUN_00e3c2 (0x00E3C2) — character-transform-and-walk string loop
 * FUN_00e3e2 (0x00E3E2) — printf format-string parser (the core formatter)
 *
 * ---------------------------------------------------------------
 * FUN_00e3b1 — uart_print_formatted_string
 * ---------------------------------------------------------------
 * Confirmed disassembly (very clean — 10 instructions):
 *   pushw X            ; save format string pointer
 *   ldw X, SP; addw X, #6   ; X = SP+6 (varargs start address)
 *   pushw X            ; push varargs ptr
 *   ldw X, (0x03,SP)   ; X = format string ptr
 *   pushw X            ; push format string
 *   clrw X             ; X = 0 (output length = 0 initially)
 *   callf 0x00E4BD     ; call uart_format_engine(fmt, varargs, 0)
 *   add SP, #0x06
 *   retf
 *
 * This is the function we identified very early — called 82+ times
 * as the single choke point for all debug/UART output. Every
 * uart_send_bms_status and uart_send_fault_status call routes
 * through here.
 *
 * ---------------------------------------------------------------
 * FUN_00e3c2 — uart_walk_and_transform_string
 * ---------------------------------------------------------------
 * Confirmed disassembly — a tight counted string loop:
 *   Loop:
 *     A = *ptr              ; read current char
 *     FUN_00f1ae(A)         ; transform character (case conversion, etc.)
 *     *ptr = A              ; write back
 *     ptr++                 ; advance pointer
 *     count--               ; decrement length counter
 *     if (*ptr == 0) break  ; stop at null terminator
 *     if count != 0: loop   ; continue if more chars
 *
 * ---------------------------------------------------------------
 * FUN_00e3e2 — uart_printf_format_parser
 * ---------------------------------------------------------------
 * This is a minimal printf-style format parser. Confirmed from
 * disassembly — the function:
 *   1. Reads the format string char by char
 *   2. Detects '-' sign flag (0x2D) → sets negative flag
 *   3. Detects '0' (0x30) → writes '0' padding or ' ' padding
 *      to the output buffer based on field-width flag
 *   4. Detects '*' (0x2A) → reads width from args
 *   5. Main loop: walks format string and writes chars to output
 *      buffer (via pointer at puVar8+0x0E)
 *   6. Return value: chars written − starting position
 *      (= formatted output length)
 *
 * All three functions together form the complete UART printf chain:
 *   uart_print_formatted_string
 *     → uart_format_engine (FUN_00e4bd)
 *       → uart_printf_format_parser
 *         → uart_walk_and_transform_string
 *           → uart_send_byte (FUN_00bb72)
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

extern void    uart_format_engine(const char *fmt,
                                  const uint8_t *args,
                                  uint16_t len);       /* FUN_00E4BD */
extern uint8_t char_transform(uint8_t ch);            /* FUN_00F1AE */

/*
 * uart_print_formatted_string  (FUN_00e3b1)
 *
 * printf-to-UART entry point. Takes a format string pointer in X
 * and variadic arguments on the stack above it.
 * Confirmed called as: ldw X, #0x8260; callf 0x00E3B1
 */
void uart_print_formatted_string(const char *fmt, ...)
{
    /* Stack layout after entry:
     *   SP+3 = fmt (format string pointer)
     *   SP+6 = first vararg
     * Calls FUN_00E4BD with (fmt, &varargs, 0) */
    const uint8_t *args = (const uint8_t *)(&fmt + 1);
    uart_format_engine(fmt, args, 0u);
}

/*
 * uart_walk_and_transform_string  (FUN_00e3c2)
 *
 * Walks a string of up to `count` characters, applies
 * char_transform() to each byte, and writes back in place.
 * Stops early at null terminator.
 */
void uart_walk_and_transform_string(char *str, uint16_t count)
{
    while (count != 0u && *str != '\0')
    {
        *str = (char)char_transform((uint8_t)*str);
        str++;
        count--;
    }
}

/*
 * uart_printf_format_parser  (FUN_00e3e2)
 *
 * Minimal printf format-string parser. Writes formatted output
 * characters into the output buffer pointed to by ctx->out_ptr.
 *
 * Supported format features (confirmed from disassembly):
 *   '-'  leading minus sign (sets negative_flag)
 *   '0'  leading zero or space padding (writes '0' or ' ')
 *   '*'  field width from args
 *   Main loop: copies characters to output until field done
 *   Decimal output after '.': writes trailing fractional digits
 *
 * ctx is an opaque format context struct (layout inferred from
 * puVar8 offsets in Ghidra: [0x0E]=out_ptr, [0x10]=width,
 * [0x12]=src_ptr, [0x14]=count, [0x15]=start_ptr, [0x1D]=frac_digits)
 */
typedef struct {
    char    *dummy[5];     /* confirmed byte offsets 0x00-0x09 */
    int16_t  field_width;  /* offset 0x10 */
    char    *src_ptr;      /* offset 0x12 — source string      */
    int8_t   char_count;   /* offset 0x14 — chars to format    */
    char    *start_ptr;    /* offset 0x15 — output start mark  */
    char    *out_ptr;      /* offset 0x0E — current output pos */
    char    *upper_ptr;    /* offset 0x0C — source upper limit */
    int8_t   frac_digits;  /* offset 0x1D — fractional places  */
} printf_ctx_t;

int16_t uart_printf_format_parser(printf_ctx_t *ctx)
{
    bool    negative_flag = false;
    char   *src = ctx->src_ptr;

    /* Detect leading '-' (0x2D) */
    if (*src == '-')
    {
        negative_flag = true;
        src++;
    }
    ctx->src_ptr = src;

    /* Handle leading zero/space padding and '*' width specifier */
    if (*src == '0')
    {
        /* Zero padding: write '0' to output if field-width flag set */
        if (ctx->out_ptr != NULL)
            *ctx->out_ptr = '0';
        src++;
        ctx->src_ptr = src;
    }
    else if (*src != '*')
    {
        /* Space padding */
        if (ctx->out_ptr != NULL)
            *ctx->out_ptr = ' ';
    }

    if (*src == '*')
    {
        /* Width from argument — skip specifier */
        src++;
        ctx->src_ptr = src;
    }

    /* Main output loop: copy chars to output buffer */
    ctx->char_count = ctx->frac_digits;
    if (ctx->frac_digits != '\0')
    {
        do {
            char ch;
            if (ctx->src_ptr < ctx->upper_ptr)
            {
                ch = *ctx->src_ptr;
                ctx->src_ptr++;
            }
            else
            {
                ch = '0';
            }
            *ctx->out_ptr = ch;
            ctx->out_ptr++;
            ctx->char_count--;
        } while (ctx->char_count != '\0');
    }

    /* Write decimal point if fractional digits specified */
    ctx->char_count = ctx->frac_digits;
    if (ctx->frac_digits != '\0')
    {
        *ctx->out_ptr = '.';
        ctx->out_ptr++;
    }

    /* Output negative-precision trailing zeros */
    while ((ctx->field_width < 0) && (ctx->char_count != '\0'))
    {
        *ctx->out_ptr = '0';
        ctx->out_ptr++;
        ctx->field_width++;
        ctx->char_count--;
    }

    /* Output remaining source chars */
    while (ctx->char_count != '\0')
    {
        char ch;
        if (ctx->src_ptr < ctx->upper_ptr)
        {
            ch = *ctx->src_ptr;
            ctx->src_ptr++;
        }
        else
        {
            ch = '0';
        }
        *ctx->out_ptr = ch;
        ctx->out_ptr++;
        ctx->char_count--;
    }

    /* Return bytes written = out_ptr - start_ptr */
    return (int16_t)((char *)ctx->out_ptr - (char *)ctx->start_ptr);
}
