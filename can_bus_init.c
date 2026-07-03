/*
 * can_bus_init.c
 *
 * Original address : 0x00A880  /  FUN_00a880
 *
 * Initialises the CAN TX mailbox hardware registers for operation.
 *
 * Confirmed disassembly:
 *   Phase 1 — wait for CAN TX mailbox to become ready:
 *     bset 0x5420, #0       → set TXRQ (transmit request) in mailbox 0
 *     do {
 *       if (DAT_005421 | 0xFD != 0xFF) break  → exit if bit 1 is 0
 *     } while (DAT_005421 & 1 == 0)            → wait while TXOK not set
 *
 *   Phase 2 — abort pending TX and enter init mode:
 *     DAT_005420 = (DAT_005420 & 0xFC) | 4    → set ABRQ (abort request)
 *
 *   Phase 3 — wait for mailbox to be empty:
 *     do {} while (DAT_005421 | 0xFE == 0xFF)  → wait while TME not set
 *
 *   Phase 4 — initialise all TX mailbox registers to 0:
 *     DAT_000248 = 1    → set a "CAN ready" flag in RAM
 *     DAT_005430..0x542F = 0   → clear ID bytes and data bytes
 *
 *   Phase 5 — set initial mailbox state:
 *     DAT_005427 = 6    → mailbox 0 priority = 6
 *     DAT_005432 = 7    → first data byte preload
 *
 * STM8AF CAN TX mailbox registers (base 0x5420):
 *   0x5420 = CAN_T0CR — TX mailbox 0 control (TXRQ, ABRQ, RQCP)
 *   0x5421 = CAN_T0SR — TX mailbox 0 status  (TXOK, TME, ALST, TERR)
 *   0x5427 = CAN_T0MDLR — mailbox data length + priority
 *   0x5428 = CAN_T0MCR  — mailbox control (send trigger)
 *   0x5429 = CAN_T0DLC  — data length code (DLC)
 *   0x542A..0x542D = CAN_T0IDR0..3 — message ID bytes
 *   0x542E..0x5435 = CAN_T0D0R..7  — 8 data bytes
 */
#include <stdint.h>
#include <stdbool.h>

#define CAN_TX_CR      (*(volatile uint8_t *)0x5420u)   /* TX control   */
#define CAN_TX_SR      (*(volatile uint8_t *)0x5421u)   /* TX status    */
#define CAN_TX_PRIO    (*(volatile uint8_t *)0x5427u)   /* priority/DL  */
#define CAN_TX_MCR     (*(volatile uint8_t *)0x5428u)   /* send trigger */
#define CAN_TX_DLC     (*(volatile uint8_t *)0x5429u)   /* data length  */
#define CAN_TX_ID0     (*(volatile uint8_t *)0x542Au)   /* ID byte 0    */
#define CAN_TX_ID1     (*(volatile uint8_t *)0x542Bu)   /* ID byte 1    */
#define CAN_TX_ID2     (*(volatile uint8_t *)0x542Cu)   /* ID byte 2    */
#define CAN_TX_ID3     (*(volatile uint8_t *)0x542Du)   /* ID byte 3    */
#define CAN_TX_D0      (*(volatile uint8_t *)0x542Eu)   /* data byte 0  */
#define CAN_TX_D1      (*(volatile uint8_t *)0x542Fu)   /* data byte 1  */

#define CAN_READY_FLAG (*(volatile uint8_t *)0x0248u)   /* RAM flag     */

void can_bus_init(void)
{
    /* Phase 1: wait for TX mailbox ready (TXOK bit poll) */
    do {
        if ((CAN_TX_SR | 0xFDu) != 0xFFu) break;  /* TME set → exit */
    } while ((CAN_TX_SR & 0x01u) == 0u);           /* wait for TXOK  */

    /* Phase 2: abort any pending TX, enter init */
    CAN_TX_CR = (CAN_TX_CR & 0xFCu) | 0x04u;      /* ABRQ = 1       */

    /* Phase 3: wait for mailbox empty (TME = transmit mailbox empty) */
    while ((CAN_TX_SR | 0xFEu) == 0xFFu) {}

    /* Phase 4: set CAN-ready flag and clear all mailbox registers */
    CAN_READY_FLAG = 1u;
    CAN_TX_D1      = 0u;
    CAN_TX_MCR     = 0u;
    CAN_TX_DLC     = 0u;
    CAN_TX_ID0     = 0u;
    CAN_TX_ID1     = 0u;
    CAN_TX_ID2     = 0u;
    CAN_TX_ID3     = 0u;
    CAN_TX_D0      = 0u;
    CAN_TX_D1      = 0u;

    /* Phase 5: set initial priority and data length */
    CAN_TX_PRIO    = 6u;
    /* 0x5432 = 7 (first data register preloaded to 7) */
    *(volatile uint8_t *)0x5432u = 7u;
}
