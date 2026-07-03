/*
 * uart_send_byte.c
 *
 * Original address : 0x00BB72  /  FUN_00bb72
 * Function name    : uart_send_byte
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * Sends a single byte over the UART by writing to UART_DR (0x5231)
 * then busy-waits until the TX complete flag (bit 7 of UART_SR) is set.
 *
 * Confirmed disassembly:
 *   ld   0x5231, A    → UART_DR = param_1  (write byte to TX register)
 *   ld   A, 0x5230    → A = UART_SR
 *   bcp  A, #0x80     → test bit 7 (TC = transmission complete)
 *   jreq .-7          → loop while TC == 0 (not yet complete)
 *   clr  A            → A = 0
 *   retf              → return 0
 *
 * STM8 UART registers:
 *   0x5230 = UART_SR  — bit 7 = TC (transmission complete)
 *   0x5231 = UART_DR  — data register
 *
 * Used by FUN_00E3B1 (the string-output function we see called
 * throughout the debug print chain) to transmit each character.
 * Called 82+ times indirectly via the string-table print function.
 */

#include <stdint.h>

#define UART_SR  (*(volatile uint8_t *)0x5230u)
#define UART_DR  (*(volatile uint8_t *)0x5231u)

#define UART_SR_TC   0x80u   /* transmission complete bit */

/*
 * uart_send_byte
 *
 * Transmits one byte synchronously over UART.
 * Blocks until the byte has been fully sent (TC flag set).
 * Returns 0.
 */
uint8_t uart_send_byte(uint8_t byte)
{
    UART_DR = byte;                         /* load byte into TX register */
    while ((UART_SR & UART_SR_TC) == 0u)   /* wait for transmission done */
    {}
    return 0u;
}
