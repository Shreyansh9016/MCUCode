/*
 * can_rx_frame_handler.c
 *
 * Original address : 0x00B6E5  /  FUN_00b6e5
 * Function name    : can_rx_frame_handler
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * This is the BMS CAN RECEIVE handler — the counterpart to
 * can_transmit_bms_frames (FUN_00a8f8). It runs in the main loop
 * after each CAN transmission cycle and processes any CAN frames
 * that arrived in the RX mailbox.
 *
 * Confirmed disassembly:
 *   1. Set DAT_005064 = 0x56  (activity marker / watchdog kick)
 *   2. Check CAN_RX_SR (0x5424) bit 0 — if zero, skip to cleanup
 *   3. Set CAN_TX_PRIO = 7    (highest priority for any reply)
 *   4. Parse each received frame by its 4-byte extended ID:
 *      ID 0x5806_B0_C0: special event flag (DAT_0001FC)
 *      ID 0x5806_B2_C2: set charge-active flag (DAT_0001FA = 1)
 *      ID 0x5806_B4_C4: set charge-active flag (DAT_0001FA = 1)
 *      ID 0x5806_BA_C1: write calibration block 2 (0x4075-0x407A)
 *      ID 0x5806_BB_C1: write calibration block 1 (0x406E-0x4074)
 *      ID 0x5806_BB_C2: write 16-bit measurement word to RAM 0x01EA/0x01E8
 *   5. Clear TX-busy flag (DAT_00505F = 0)
 *   6. Set CAN_RX_SR bit 5 (acknowledge/clear RX mailbox)
 *
 * CAN RX mailbox registers (STM8AF, base 0x5420 RX side):
 *   0x5424 = CAN_RF0R — RX FIFO 0 status (bit 0 = message pending)
 *   0x5427 = CAN_TX priority (reused here for response priority)
 *   0x542A = CAN_RX_ID byte 0   \
 *   0x542B = CAN_RX_ID byte 1   |  received frame extended ID
 *   0x542C = CAN_RX_ID byte 2   |
 *   0x542D = CAN_RX_ID byte 3  /
 *   0x542E..0x5435 = RX data bytes 0..7
 */

#include <stdint.h>

/* ── CAN RX mailbox registers ── */
#define CAN_RX_STATUS  (*(volatile uint8_t *)0x5424u)   /* RF0R */
#define CAN_TX_PRIO    (*(volatile uint8_t *)0x5427u)
#define CAN_RX_ID0     (*(volatile uint8_t *)0x542Au)
#define CAN_RX_ID1     (*(volatile uint8_t *)0x542Bu)
#define CAN_RX_ID2     (*(volatile uint8_t *)0x542Cu)
#define CAN_RX_ID3     (*(volatile uint8_t *)0x542Du)
#define CAN_RX_D0      (*(volatile uint8_t *)0x542Eu)
#define CAN_RX_D1      (*(volatile uint8_t *)0x542Fu)
#define CAN_RX_D2      (*(volatile uint8_t *)0x5430u)
#define CAN_RX_D3      (*(volatile uint8_t *)0x5431u)
#define CAN_RX_D4      (*(volatile uint8_t *)0x5432u)
#define CAN_RX_D5      (*(volatile uint8_t *)0x5433u)

/* ── Activity / watchdog marker ── */
#define ACTIVITY_MARKER  (*(volatile uint8_t *)0x5064u)
#define TX_BUSY_FLAG     (*(volatile uint8_t *)0x505Fu)

/* ── CAN base ID bytes (same as TX side) ── */
#define CAN_BASE_B0    0x58u
#define CAN_BASE_B1    0x06u

/* ── State flags written by this RX handler ── */
#define FLAG_SPECIAL_EVENT  (*(volatile uint16_t *)0x01FCu) /* magic 0xAA event */
#define FLAG_CHARGE_ACTIVE  (*(volatile uint16_t *)0x01FAu) /* charge frame guard */

/* ── Flash calibration write targets ── */
#define CAL2_BYTE0  (*(volatile uint8_t *)0x4075u)
#define CAL2_BYTE1  (*(volatile uint8_t *)0x4076u)
#define CAL2_BYTE2  (*(volatile uint8_t *)0x4077u)
#define CAL2_BYTE3  (*(volatile uint8_t *)0x4078u)
#define CAL2_BYTE4  (*(volatile uint8_t *)0x4079u)
#define CAL2_BYTE5  (*(volatile uint8_t *)0x407Au)

#define CAL1_BYTE0  (*(volatile uint8_t *)0x406Eu)
#define CAL1_BYTE1  (*(volatile uint8_t *)0x406Fu)
#define CAL1_BYTE2  (*(volatile uint8_t *)0x4071u)
#define CAL1_BYTE3  (*(volatile uint8_t *)0x4072u)
#define CAL1_BYTE4  (*(volatile uint8_t *)0x4073u)
#define CAL1_BYTE5  (*(volatile uint8_t *)0x4074u)

/* ── RAM measurement word written by frame 0xBBC2 ── */
#define MEAS_WORD_EA  (*(volatile uint16_t *)0x01EAu)
#define MEAS_WORD_E8  (*(volatile uint16_t *)0x01E8u)

void can_rx_frame_handler(void)
{
    /* Kick activity marker / watchdog */
    ACTIVITY_MARKER = 0x56u;

    /* Check if a CAN frame is pending in RX FIFO 0 */
    if ((CAN_RX_STATUS & 0x01u) == 0u)
        goto cleanup;

    /* Set highest TX priority for any outgoing reply */
    CAN_TX_PRIO = 7u;

    /* Verify common ID prefix: 0x58, 0x06 */
    if (CAN_RX_ID0 != CAN_BASE_B0 || CAN_RX_ID1 != CAN_BASE_B1)
        goto cleanup;

    /* ── Frame ID 0x5806_B0C0: special event control ── */
    if (CAN_RX_ID2 == 0xB0u && CAN_RX_ID3 == 0xC0u)
    {
        /* If data byte 0 == 0xAA, trigger the special event frame.
         * Otherwise clear the event flag.                            */
        FLAG_SPECIAL_EVENT = (CAN_RX_D0 == 0xAAu) ? 0xAAu : 0u;
    }

    /* ── Frame ID 0x5806_B2C2: enable charge-active frame ── */
    if (CAN_RX_ID2 == 0xB2u && CAN_RX_ID3 == 0xC2u)
        FLAG_CHARGE_ACTIVE = 1u;

    /* ── Frame ID 0x5806_B4C4: also enables charge-active frame ── */
    if (CAN_RX_ID2 == 0xB4u && CAN_RX_ID3 == 0xC4u)
        FLAG_CHARGE_ACTIVE = 1u;

    /* ── Frame ID 0x5806_BAC1: write calibration block 2 ── */
    if (CAN_RX_ID2 == 0xBAu && CAN_RX_ID3 == 0xC1u)
    {
        CAL2_BYTE0 = CAN_RX_D0;
        CAL2_BYTE1 = CAN_RX_D1;
        CAL2_BYTE2 = CAN_RX_D2;
        CAL2_BYTE3 = CAN_RX_D3;
        CAL2_BYTE4 = CAN_RX_D4;
        CAL2_BYTE5 = CAN_RX_D5;
    }

    /* ── Frame ID 0x5806_BBC1: write calibration block 1 ── */
    if (CAN_RX_ID2 == 0xBBu && CAN_RX_ID3 == 0xC1u)
    {
        CAL1_BYTE0 = CAN_RX_D0;
        CAL1_BYTE1 = CAN_RX_D1;
        CAL1_BYTE2 = CAN_RX_D2;
        CAL1_BYTE3 = CAN_RX_D3;
        CAL1_BYTE4 = CAN_RX_D4;
        CAL1_BYTE5 = CAN_RX_D5;
    }

    /* ── Frame ID 0x5806_BBC2: write 16-bit measurement words ── */
    if (CAN_RX_ID2 == 0xBBu && CAN_RX_ID3 == 0xC2u)
    {
        MEAS_WORD_EA = (uint16_t)CAN_RX_D0;
        MEAS_WORD_E8 = (uint16_t)CAN_RX_D1;
    }

cleanup:
    TX_BUSY_FLAG   = 0u;
    CAN_RX_STATUS |= 0x20u;   /* RFOM0: release RX FIFO 0 mailbox */
}
