/*
 * bms_main_loop.c
 *
 * Original address : 0x00B936  /  FUN_00b936
 * Function name    : bms_main_loop
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * THIS IS THE BMS FIRMWARE ENTRY POINT / MAIN FUNCTION.
 *
 * This is the function that FUN_008B3B (startup code) calls —
 * confirmed from the call site at 0x008B35 where the entire
 * startup chain culminates here. It never returns.
 *
 * ── Startup sequence (one-time initialisation) ──
 *
 *   1. GPIO setup:
 *      - set_gpio_output_bit0(1)        : enable output
 *      - set_gpio_config_bits(0)        : clear config bits
 *      - set_gpio_pin_mask(1, ...)      : set pin 1 enable
 *      - set_gpio_pin_mask(0x701, ...)  : set pins 7+1+0
 *
 *   2. CAN receive mailbox setup:
 *      - can_configure_rx_mailbox(0xA0, 0, 0x4E, 0x20, 0)
 *        → CAN RX filter: accept frames with ID 0xA0...
 *      - can_set_filter_bit(0x101)       : set filter bit
 *
 *   3. GPIO peripheral registers (Port A, B, C, D, E):
 *      Multiple calls to bms_state_clear, bms_state_update,
 *      reg_clear_bits on GPIO registers 0x5000, 0x500A, 0x500F, 0x5014
 *      → configures GPIO pins for I2C, CAN, UART, indicator LEDs
 *
 *   4. ADC output configuration:
 *      - configure_gpio_adc_output()     : select ADC mux channel
 *      - FUN_00f817(0x250) / FUN_00f7dd(0x250): write config word
 *      - FUN_00f65d(0x80AC): read threshold from flash
 *        → branches on result to set appropriate GPIO output level
 *        (up to 5 nested comparisons against flash thresholds
 *         0x80AC, 0x80B0, 0x80B4, 0x80B8, 0x80AC — reads 5 different
 *         config values and enables different GPIO output bits
 *         {0x5000 bit3, 0x500F bit3, 0x500F bit0, 0x5014 bit3, 0x500A bit3}
 *         depending on which threshold the measurement clears)
 *
 *   5. Hardware init:
 *      - __EnableInterrupt()            : enable global interrupts
 *      - can_bus_init()                 : init CAN TX mailbox
 *      - init_measurement_buffers()     : zero-clear AFE RAM + init AFE
 *      - FUN_008b3b() / FUN_00bb35()    : UART + timer peripheral init
 *      - FUN_00d673()                   : (protection threshold init)
 *      - can_set_master_enable(1)       : enable CAN controller
 *      - FUN_009055(0x254)              : trigger AFE start conversion
 *
 * ── Main loop (runs forever) ──
 *
 *   Waits for 4 timer ticks (DAT_00024D==1, DAT_00024F increments)
 *   then on every 4th tick executes:
 *
 *   a. configure_afe_registers()        — read all AFE measurements
 *   b. FUN_008ba6()                     — SOC / coulomb counting
 *   c. can_rx_frame_handler()           — process any received CAN frame
 *   d. can_transmit_bms_frames()        — broadcast all BMS data over CAN
 *   e. FUN_00d898() / FUN_00d76e()      — protection logic
 *   f. bms_uart_send_status()           — UART debug output
 *   g. FUN_00bd7b() / FUN_00be17()      — display / telemetry
 *   h. FUN_00c2da() / FUN_00d689()      — balancing / thermal logic
 *   i. reg_toggle_bits(0x5000, 8)       — toggle heartbeat LED
 *   j. loop forever
 */

#include <stdint.h>
#include <stdbool.h>

/* ── Peripheral init helpers ── */
extern void set_gpio_output_bit0    (uint8_t en);           /* FUN_009861  */
extern void set_gpio_config_bits    (uint8_t cfg);          /* FUN_0099CD  */
extern void set_gpio_pin_mask       (uint16_t cfg, uint8_t en); /* FUN_0098A7  */
extern void can_configure_rx_mailbox(uint16_t id, uint8_t mode,
                                     uint8_t d0, uint8_t d1,
                                     uint8_t dlc);          /* FUN_00A033  */
extern void can_set_filter_bit      (uint16_t param_1);     /* FUN_00A2C9  */
extern void bms_state_clear         (uint8_t *state);       /* FUN_009B95  */
extern void bms_state_update        (uint8_t *state,
                                     uint8_t mask,
                                     uint8_t flags);        /* FUN_009B9D  */
extern void reg_clear_bits          (uint8_t *reg, uint8_t mask);/* FUN_009C14  */
extern void reg_set_bits            (uint8_t *reg, uint8_t mask);/* FUN_009C0D  */
extern void reg_toggle_bits         (uint8_t *reg, uint8_t mask);/* FUN_009C1C  */
extern void configure_gpio_adc_output(void);                /* FUN_009A73  */
extern void can_bus_init            (void);                 /* FUN_00A880  */
extern void init_measurement_buffers(void);                 /* FUN_00916C  */
extern void can_set_master_enable   (uint8_t en);           /* FUN_00A2AD  */

/* ── Main loop tasks ── */
extern void configure_afe_registers (void);                 /* FUN_0091FB  */
extern void can_rx_frame_handler    (void);                 /* FUN_00B6E5  */
extern void can_transmit_bms_frames (void);                 /* FUN_00A8F8  */
extern void trigger_afe_conversion  (void *ptr);            /* FUN_009055  */

/* ── Forward-declared firmware functions (not yet decoded) ── */
extern void FUN_008b3b(void);   /* startup / clock init           */
extern void FUN_008ba6(void);   /* SOC / coulomb counting         */
extern void FUN_00bb35(void);   /* UART peripheral init           */
extern void FUN_00d673(void);   /* protection threshold init      */
extern void FUN_00d898(void);   /* protection logic               */
extern void FUN_00d76e(void);   /* thermal / balancing check      */
extern void FUN_00bb7e(void);   /* UART status print              */
extern void FUN_00bd7b(void);   /* display / CAN telemetry        */
extern void FUN_00be17(void);   /* fault display output           */
extern void FUN_00c2da(void);   /* cell balancing logic           */
extern void FUN_00d689(void);   /* additional protection          */

/* ── Peripheral config (ADC mux threshold) from flash ── */
extern uint8_t FUN_00f65d(const volatile uint8_t *cfg);     /* read+compare */
extern void    FUN_00f817(uint16_t ctx);                     /* write word    */
extern void    FUN_00f7dd(uint16_t ctx);                     /* load float    */

/* ── STM8 enable-interrupt intrinsic ── */
extern void __EnableInterrupt(void);

/* ── GPIO peripheral registers ── */
#define GPIO_A  ((volatile uint8_t *)0x5000u)
#define GPIO_B  ((volatile uint8_t *)0x500Au)
#define GPIO_C  ((volatile uint8_t *)0x500Fu)
#define GPIO_D  ((volatile uint8_t *)0x5014u)

/* ── Tick synchronisation ── */
#define TICK_FLAG    (*(volatile uint8_t *)0x024Du)
#define TICK_COUNTER (*(volatile uint8_t *)0x024Fu)

/* ── CAN RX mailbox address for start-conversion trigger ── */
#define CAN_RX_MBOX_ADDR  ((void *)0x0254u)
#define CAN_RX_MBOX_DATA  (*(volatile uint16_t *)0x0254u)

void bms_main_loop(void)
{
    /* ── Stage 1: GPIO configuration ── */
    set_gpio_output_bit0(1u);
    set_gpio_config_bits(0u);
    set_gpio_pin_mask(0x0001u, 0u);
    set_gpio_pin_mask(0x0701u, 0u);

    /* ── Stage 2: CAN RX mailbox filter ── */
    can_configure_rx_mailbox(0x00A0u, 0u, 0x4Eu, 0x20u, 0u);
    can_set_filter_bit(0x0101u);

    /* ── Stage 3: GPIO peripheral register initialisation ── */
    bms_state_clear(GPIO_A + 0x14u);   /* clear Port A+0x14 */
    bms_state_update(GPIO_A,  0x08u, 0xE0u);
    bms_state_update(GPIO_A,  0x40u, 0xE0u);
    bms_state_update(GPIO_B,  0x02u, 0xE0u);
    bms_state_update(GPIO_B,  0x04u, 0xE0u);
    bms_state_update(GPIO_B,  0x08u, 0xE0u);
    bms_state_update(GPIO_B,  0x10u, 0xE0u);
    bms_state_update(GPIO_C,  0x01u, 0xE0u);
    bms_state_update(GPIO_C,  0x04u, 0x40u);
    bms_state_update(GPIO_C,  0x08u, 0xE0u);
    bms_state_update(GPIO_C,  0x10u, 0xE0u);
    bms_state_update(GPIO_D,  0x08u, 0xE0u);
    bms_state_update(GPIO_D,  0x40u, 0xE0u);
    reg_clear_bits  (GPIO_C,  0x10u);
    reg_clear_bits  (GPIO_B,  0x10u);
    reg_clear_bits  (GPIO_B,  0x04u);
    reg_clear_bits  (GPIO_B,  0x08u);

    /* ── Stage 4: ADC output level selection ── */
    configure_gpio_adc_output();
    FUN_00f817(0x0250u);
    FUN_00f7dd(0x0250u);

    /* Select GPIO output bit based on flash threshold comparison */
    if (FUN_00f65d((const volatile uint8_t *)0x80ACu))
        reg_set_bits(GPIO_A, 0x08u);           /* threshold 1 */
    else if (FUN_00f65d((const volatile uint8_t *)0x80B0u))
        reg_set_bits(GPIO_C, 0x08u);           /* threshold 2 */
    else if (FUN_00f65d((const volatile uint8_t *)0x80B4u))
        reg_set_bits(GPIO_C, 0x01u);           /* threshold 3 */
    else if (FUN_00f65d((const volatile uint8_t *)0x80B8u))
        reg_set_bits(GPIO_D, 0x08u);           /* threshold 4 */
    else if (FUN_00f65d((const volatile uint8_t *)0x80ACu))
        reg_set_bits(GPIO_B, 0x08u);           /* threshold 5 */

    /* ── Stage 5: Enable interrupts and init hardware ── */
    __EnableInterrupt();
    can_bus_init();
    init_measurement_buffers();
    FUN_008b3b();
    FUN_00bb35();
    FUN_00d673();
    can_set_master_enable(1u);
    CAN_RX_MBOX_DATA = 0xF082u;
    trigger_afe_conversion(CAN_RX_MBOX_ADDR);

    /* ── Main loop — runs forever ── */
    while (1)
    {
        /* Wait for tick flag (set by timer ISR) */
        while (TICK_FLAG != 1u) {}
        TICK_FLAG = 0u;
        TICK_COUNTER++;

        /* Execute full cycle every 4 ticks */
        if (TICK_COUNTER < 4u) continue;

        /* ── Full measurement + CAN cycle ── */
        configure_afe_registers();          /* read all AFE registers    */
        FUN_008ba6();                       /* SOC / coulomb counting    */
        can_rx_frame_handler();             /* process any received CAN  */
        can_transmit_bms_frames();          /* broadcast all BMS data    */
        FUN_00d898();                       /* protection logic          */
        FUN_00d76e();                       /* thermal/balancing check   */
        FUN_00bb7e();                       /* UART status print         */
        FUN_00bd7b();                       /* display/telemetry         */
        FUN_00be17();                       /* fault display output      */
        FUN_00c2da();                       /* cell balancing logic      */
        FUN_00d689();                       /* additional protection     */

        /* Toggle heartbeat LED on GPIO_A bit 3 */
        reg_toggle_bits(GPIO_A, 0x08u);

        TICK_COUNTER = 0u;
    }
}
