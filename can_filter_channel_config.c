/*
 * can_filter_channel_config.c
 *
 * Eight functions configuring CAN filter channel registers.
 *
 * FUN_00a668 (0x00A668) — set CAN filter channel 1 (register 0x5258)
 * FUN_00a675 (0x00A675) — set CAN filter channel 2 (register 0x5259)
 * FUN_00a682 (0x00A682) — set CAN filter channel 3 (register 0x525A)
 * FUN_00a68f (0x00A68F) — set CAN filter channel 4 (register 0x525B)
 * FUN_00a7bb (0x00A7BB) — set CAN filter mask register (0x5255)
 * FUN_00a7c0 (0x00A7C0) — configure CAN TX channel 1 (0x525C/0x5258)
 * FUN_00a7f0 (0x00A7F0) — configure CAN TX channel 2 (0x525C/0x5259)
 * FUN_00a820 (0x00A820) — configure CAN TX channel 3 (0x525D/0x525A)
 * FUN_00a850 (0x00A850) — configure CAN TX channel 4 (0x525D/0x525B)
 *
 * Register map (STM8AF/STM8AL CAN, 0x5250+ region):
 *   0x5255 = CAN filter acceptance mask (complement = reject mask)
 *   0x5258 = CAN channel 1 control/config
 *   0x5259 = CAN channel 2 control/config
 *   0x525A = CAN channel 3 control/config
 *   0x525B = CAN channel 4 control/config
 *   0x525C = CAN TX mailbox control register (channels 1+2)
 *   0x525D = CAN TX mailbox control register (channels 3+4)
 *
 * Each channel register: bits [3:2] preserved, bits [1:0] set from param.
 * TX mailbox register: bits [7:2] are enable/mode, [1:0] are channel flags.
 */
#include <stdint.h>

#define CAN_FILTER_MASK   (*(volatile uint8_t *)0x5255u)
#define CAN_CH1_CFG       (*(volatile uint8_t *)0x5258u)
#define CAN_CH2_CFG       (*(volatile uint8_t *)0x5259u)
#define CAN_CH3_CFG       (*(volatile uint8_t *)0x525Au)
#define CAN_CH4_CFG       (*(volatile uint8_t *)0x525Bu)
#define CAN_TX_MBX_AB     (*(volatile uint8_t *)0x525Cu)  /* channels 1+2 */
#define CAN_TX_MBX_CD     (*(volatile uint8_t *)0x525Du)  /* channels 3+4 */

/* Confirmed disassembly for all four channel writers:
 *   ld A,(X) OR ld A,param; and A,#0xF3; or A,param_1; ld reg,A
 *   → preserves bits [3:2], writes param_1 into bits [1:0] and [7:4] */

uint8_t can_set_filter_channel1(uint8_t param_1)
{
    CAN_CH1_CFG = (CAN_CH1_CFG & 0xF3u) | param_1;
    return param_1;
}

uint8_t can_set_filter_channel2(uint8_t param_1)
{
    CAN_CH2_CFG = (CAN_CH2_CFG & 0xF3u) | param_1;
    return param_1;
}

uint8_t can_set_filter_channel3(uint8_t param_1)
{
    CAN_CH3_CFG = (CAN_CH3_CFG & 0xF3u) | param_1;
    return param_1;
}

uint8_t can_set_filter_channel4(uint8_t param_1)
{
    CAN_CH4_CFG = (CAN_CH4_CFG & 0xF3u) | param_1;
    return param_1;
}

/*
 * can_set_filter_mask  (FUN_00a7bb)
 *
 * Writes the bitwise complement of param_1 into the CAN
 * filter acceptance mask register (0x5255).
 * A '1' in this register means the corresponding bit
 * is "don't care" for acceptance filtering.
 */
void can_set_filter_mask(uint8_t param_1)
{
    CAN_FILTER_MASK = ~param_1;
}

/*
 * can_configure_tx_channel1  (FUN_00a7c0)
 *
 * Configures TX channel 1 in the TX mailbox A/B register (0x525C):
 *   - Writes param_2 * 0x10 into CH1_CFG bits [7:4], [3:2] preserved
 *   - If param_1 high byte != 0: sets bit 1 of TX_MBX_AB (enable)
 *   - If param_1 high byte == 0: clears bits [1:0] of TX_MBX_AB
 *   - Always sets bit 0 of TX_MBX_AB (channel 1 active)
 */
void can_configure_tx_channel1(uint16_t param_1, uint8_t param_2)
{
    uint8_t prev  = CAN_TX_MBX_AB;
    CAN_TX_MBX_AB &= ~0x01u;                            /* clear ch1 active */
    CAN_CH1_CFG    = (CAN_CH1_CFG & 0x0Cu) | (uint8_t)(param_2 * 0x10u);

    if ((uint8_t)(param_1 >> 8u) == 0u)
        CAN_TX_MBX_AB = prev & 0xFCu;                   /* clear bits [1:0] */
    else
        CAN_TX_MBX_AB |= 0x02u;                         /* set  enable bit  */

    CAN_TX_MBX_AB |= 0x01u;                             /* set  ch1 active  */
}

/*
 * can_configure_tx_channel2  (FUN_00a7f0)
 *
 * Same as channel 1 but uses bit 4 (ch2 active) and bit 5 (ch2 enable).
 */
void can_configure_tx_channel2(uint16_t param_1, uint8_t param_2)
{
    uint8_t prev  = CAN_TX_MBX_AB;
    CAN_TX_MBX_AB &= ~0x10u;
    CAN_CH2_CFG    = (CAN_CH2_CFG & 0x0Cu) | (uint8_t)(param_2 * 0x10u);

    if ((uint8_t)(param_1 >> 8u) == 0u)
        CAN_TX_MBX_AB = prev & 0xCFu;
    else
        CAN_TX_MBX_AB |= 0x20u;

    CAN_TX_MBX_AB |= 0x10u;
}

/*
 * can_configure_tx_channel3  (FUN_00a820)
 *
 * Same pattern but uses TX_MBX_CD (0x525D) for channels 3+4.
 * Bit 0 = ch3 active, bit 1 = ch3 enable.
 */
void can_configure_tx_channel3(uint16_t param_1, uint8_t param_2)
{
    uint8_t prev  = CAN_TX_MBX_CD;
    CAN_TX_MBX_CD &= ~0x01u;
    CAN_CH3_CFG    = (CAN_CH3_CFG & 0x0Cu) | (uint8_t)(param_2 * 0x10u);

    if ((uint8_t)(param_1 >> 8u) == 0u)
        CAN_TX_MBX_CD = prev & 0xFCu;
    else
        CAN_TX_MBX_CD |= 0x02u;

    CAN_TX_MBX_CD |= 0x01u;
}

/*
 * can_configure_tx_channel4  (FUN_00a850)
 *
 * Same pattern, bits 4 and 5 of TX_MBX_CD.
 */
void can_configure_tx_channel4(uint16_t param_1, uint8_t param_2)
{
    uint8_t prev  = CAN_TX_MBX_CD;
    CAN_TX_MBX_CD &= ~0x10u;
    CAN_CH4_CFG    = (CAN_CH4_CFG & 0x0Cu) | (uint8_t)(param_2 * 0x10u);

    if ((uint8_t)(param_1 >> 8u) == 0u)
        CAN_TX_MBX_CD = prev & 0xCFu;
    else
        CAN_TX_MBX_CD |= 0x20u;

    CAN_TX_MBX_CD |= 0x10u;
}
