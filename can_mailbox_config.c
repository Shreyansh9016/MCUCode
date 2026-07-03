/*
 * can_mailbox_config.c
 *
 * FUN_00a033 (0x00A033) — configure one CAN receive mailbox
 * FUN_00a2ad (0x00A2AD) — enable/disable CAN master controller
 * FUN_00a2c9 (0x00A2C9) — set/clear CAN filter bits
 * FUN_00a319 (0x00A319) — OR bits into CAN baud-rate register
 * FUN_00a35a (0x00A35A) — set CAN mode register bits
 *
 * STM8AF CAN peripheral (base 0x5250 region):
 *   0x5250 = CAN_MCR  — Master Control Register
 *   0x5252 = CAN_MSR  — Master Status Register  (also mode/config)
 *   0x5253 = CAN_TSR  — Transmit Status (or BRR baud rate in init)
 *   0x5254 = CAN_TPR  — Transmit Priority (or filter in init)
 *   0x5260 = CAN_RFR  — Receive FIFO / mailbox ID high
 *   0x5261 = CAN_IER  — Interrupt Enable (mailbox ID low)
 *   0x5262 = CAN_ESR  — Error Status (mailbox sub-addr byte 0)
 *   0x5263 = CAN_BTR  — Bit Timing (mailbox sub-addr byte 1)
 *   0x5264 = CAN_FMR  — Filter Mode (mailbox data length)
 */
#include <stdint.h>
#include <stdbool.h>

#define CAN_MCR    (*(volatile uint8_t *)0x5250u)
#define CAN_MSR    (*(volatile uint8_t *)0x5252u)
#define CAN_BRR    (*(volatile uint8_t *)0x5253u)
#define CAN_FILTER (*(volatile uint8_t *)0x5254u)
#define CAN_MBX_ID_HI  (*(volatile uint8_t *)0x5260u)
#define CAN_MBX_ID_LO  (*(volatile uint8_t *)0x5261u)
#define CAN_MBX_DATA0  (*(volatile uint8_t *)0x5262u)
#define CAN_MBX_DATA1  (*(volatile uint8_t *)0x5263u)
#define CAN_MBX_DLC    (*(volatile uint8_t *)0x5264u)

/*
 * can_configure_rx_mailbox  (FUN_00a033)
 *
 * Configures one CAN receive mailbox with ID, mode, and data bytes.
 *
 *   mailbox_id    : 16-bit base CAN ID
 *                   → high byte → CAN_MBX_ID_HI (0x5260)
 *                   → low  byte → CAN_MBX_ID_LO (0x5261)
 *   mode_bits     : written into CAN_MCR[6:4]  (3 mode bits)
 *   data_byte_0   : payload byte 0 → CAN_MBX_DATA0 (0x5262)
 *   data_byte_1   : payload byte 1 → CAN_MBX_DATA1 (0x5263)
 *   dlc           : data length code → CAN_MBX_DLC  (0x5264)
 */
uint16_t can_configure_rx_mailbox(uint16_t mailbox_id,
                                   uint8_t  mode_bits,
                                   uint8_t  data_byte_0,
                                   uint8_t  data_byte_1,
                                   uint8_t  dlc)
{
    CAN_MBX_DATA0  = data_byte_0;
    CAN_MBX_DATA1  = data_byte_1;
    CAN_MBX_ID_HI  = (uint8_t)(mailbox_id >> 8u);
    CAN_MBX_ID_LO  = (uint8_t)(mailbox_id & 0xFFu);
    CAN_MCR        = (CAN_MCR & 0x8Fu) | mode_bits;  /* write bits [6:4] */
    CAN_MBX_DLC    = dlc;
    return mailbox_id;
}

/*
 * can_set_master_enable  (FUN_00a2ad)
 *
 * Enables or disables the CAN controller (CAN_MCR bit 0 = INRQ).
 *   1 → set   bit 0 (request initialisation / enable controller)
 *   0 → clear bit 0 (exit init mode / disable)
 */
void can_set_master_enable(uint8_t enable)
{
    if (enable != 0u)
        CAN_MCR |=  0x01u;
    else
        CAN_MCR &= ~0x01u;
}

/*
 * can_set_filter_bit  (FUN_00a2c9)
 *
 * Sets or clears a bit in the CAN filter register (0x5254).
 *
 * param_1 high byte = the bit mask to apply
 * param_1 low  byte = 0 → clear the mask bit, else → set it
 */
uint16_t can_set_filter_bit(uint16_t param_1)
{
    uint8_t mask    = (uint8_t)(param_1 >> 8u);
    uint8_t set_clr = (uint8_t)(param_1 & 0xFFu);

    if (set_clr == 0u)
        CAN_FILTER &= ~mask;
    else
        CAN_FILTER |=  mask;

    return param_1;
}

/*
 * can_or_baud_register  (FUN_00a319)
 *
 * ORs bits into the CAN baud-rate/transmit-status register (0x5253).
 *
 * param_1 : both bytes are OR'd together with param_2 and existing reg
 * param_2 : additional byte to OR in
 *
 * Used during CAN initialisation to set baud rate prescaler bits.
 */
uint16_t can_or_baud_register(uint16_t param_1, uint8_t param_2)
{
    uint8_t lo = (uint8_t)(param_1 & 0xFFu);
    uint8_t hi = (uint8_t)(param_1 >> 8u);
    CAN_BRR |= lo | hi | param_2;
    return param_1;
}

/*
 * can_set_mode_bits  (FUN_00a35a)
 *
 * Writes param_1 into CAN_MSR bits [6:4], preserving bits [7] and [3:0].
 * Used to set CAN operating mode (normal, loopback, silent, etc.).
 */
uint8_t can_set_mode_bits(uint8_t param_1)
{
    CAN_MSR = (CAN_MSR & 0x8Fu) | param_1;
    return param_1;
}
