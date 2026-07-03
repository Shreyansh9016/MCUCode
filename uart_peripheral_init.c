/*
 * uart_peripheral_init.c
 *
 * Original address : 0x00BB35  /  FUN_00bb35
 * Function name    : uart_peripheral_init
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * Initialises the STM8 UART peripheral registers for serial
 * debug/telemetry output. Called once from bms_main_loop startup.
 *
 * Confirmed disassembly:
 *   clr  0x5234      → UART_BRR2 = 0     (baud rate prescaler hi)
 *   mov  0x5236,#0x40 → UART_CR3 = 0x40  (1 stop bit, 8-bit data)
 *   mov  0x5233,#0x05 → UART_BRR1 = 5    (baud rate prescaler lo)
 *   mov  0x5232,#0x11 → UART_CR1 = 0x11  (enable + parity odd)
 *   bset 0x5235,#3    → UART_CR2 |= 8    (set TEN = TX enable)
 *
 * STM8 UART2 register map (base 0x5230):
 *   0x5230 = UART_SR   — status register
 *   0x5231 = UART_DR   — data register (TX/RX)
 *   0x5232 = UART_BRR1 — baud rate register 1 (DIV mantissa[11:4])
 *   0x5233 = UART_BRR2 — baud rate register 2 (... wait, reversed)
 *
 * Actually confirmed order from disassembly:
 *   0x5232 = UART_CR1  (written 0x11 = UARTD enable + PCEN parity)
 *   0x5233 = UART_BRR1 (written 0x05)
 *   0x5234 = UART_BRR2 (cleared to 0)
 *   0x5235 = UART_CR2  (bit 3 set = TEN transmit enable)
 *   0x5236 = UART_CR3  (written 0x40 = 2 stop bits / CPOL)
 *
 * Baud rate calculation (16MHz clock, BRR1=5, BRR2=0):
 *   DIV = BRR1 | BRR2 = 0x0050 = 80 decimal
 *   Baud = Fcpu / DIV = 16,000,000 / 80 = 200,000 bps
 */

#include <stdint.h>

#define UART_CR1   (*(volatile uint8_t *)0x5232u)
#define UART_BRR1  (*(volatile uint8_t *)0x5233u)
#define UART_BRR2  (*(volatile uint8_t *)0x5234u)
#define UART_CR2   (*(volatile uint8_t *)0x5235u)
#define UART_CR3   (*(volatile uint8_t *)0x5236u)

#define UART_CR1_PCEN   0x10u   /* parity control enable */
#define UART_CR1_M      0x01u   /* word length = 9 bits (8+parity) */
#define UART_CR2_TEN    0x08u   /* transmitter enable */
#define UART_CR3_STOP   0x40u   /* 2 stop bits        */

void uart_peripheral_init(void)
{
    UART_BRR2 = 0u;          /* clear fractional baud divisor       */
    UART_CR3  = UART_CR3_STOP; /* 2 stop bits / mode config          */
    UART_BRR1 = 0x05u;       /* baud rate mantissa = 5 → 200 kbps   */
    UART_CR1  = UART_CR1_PCEN | UART_CR1_M;  /* enable + parity odd */
    UART_CR2 |= UART_CR2_TEN;  /* enable transmitter                  */
}
