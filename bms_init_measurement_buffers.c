/*
 * bms_init_measurement_buffers.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x00916C
 * Original name    : FUN_00916c
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  This is the BMS INITIALISATION routine — called once at startup
 *  (or on a reset/re-init event) to:
 *
 *   1. Zero-clear three RAM data blocks (the three measurement
 *      regions we've been reading from throughout these functions)
 *   2. Individually zero-clear all per-cell output bytes
 *      (the channel A–G slot addresses 0x01CB–0x01E0)
 *   3. Call the AFE hardware initialisation sequence
 *
 *  ── Confirmed RAM regions being cleared (from disassembly) ──
 *
 *  Block 1: 0x018F–0x01AE  (0x20 = 32 entries, indexed from +0x18E)
 *    clr (0x018E,X) with X from 0x20 down to 1
 *    → clears RAM[0x018F] through RAM[0x01AE]  (32 bytes)
 *    This is the "raw sensor input" region — e.g. ADC readings
 *    or I2C register shadow copies.
 *
 *  Block 2: 0x01AF–0x01C2  (0x14 = 20 entries, indexed from +0x1AE)
 *    clr (0x01AE,X) with X from 0x14 down to 1
 *    → clears RAM[0x01AF] through RAM[0x01C2]  (20 bytes)
 *    This is the "intermediate calculation" region.
 *
 *  Block 3: 0x01C3–0x01CA  (0x08 = 8 entries, indexed from +0x1C2)
 *    clr (0x01C2,X) with X from 0x08 down to 1
 *    → clears RAM[0x01C3] through RAM[0x01CA]  (8 bytes)
 *    This is the "AFE conversion result" region — the 8-byte
 *    conversion result buffer used by the channel readers.
 *
 *  ── Per-slot individual clears (confirmed addresses) ──
 *
 *  Then individually zero-clears all channel output slots we've
 *  seen throughout these functions:
 *    0x01CB–0x01CC  (ldw 0x01CB, X where X=0 → clears 2 bytes)
 *    0x01CD         cell channel A byte 0
 *    0x01CE         cell channel B byte 0
 *    0x01CF         cell channel C byte 0
 *    0x01D0         cell channel A byte 1
 *    0x01D1         cell channel B byte 1
 *    0x01D2         cell channel C byte 1
 *    0x01D3         cell channel D byte 0
 *    0x01D4         cell channel D byte 1
 *    0x01D5         cell channel E byte 0
 *    0x01D6         cell channel E byte 1
 *    0x01D7         cell channel F byte 0
 *    0x01D8         cell channel F byte 1
 *    0x01D9         cell channel G byte 0
 *    0x01DA         cell channel G byte 1
 *    0x01DB–0x01DC  (ldw 0x01DB → clears 2 bytes, pack word)
 *    0x01DD         single-byte status/flag slot
 *    0x01DE–0x01DF  (ldw 0x01DE → clears 2 bytes)
 *    0x01E0–0x01E1  (ldw 0x01E0 → clears 2 bytes)
 *
 *  ── Post-clear calls ──
 *
 *  FUN_009C47() — AFE hardware init (I2C/SPI peripheral setup)
 *
 *  FUN_009C6C(0x1A, 0x80, 0, 0x10, 0x10, 6) — AFE configuration
 *    (confirmed from disassembly: push #0x10, push #0x00,
 *     push #0x01, push #0x00, ldw X,#0x0010, ldw X,#0x1A80,
 *     ldw X,#0x0006; callf 0x009C6C)
 *    Likely: configure AFE with base address 0x1A80, 16 cells,
 *    6 temperature sensors, or similar BQ76952 init parameters.
 *
 *  FUN_009DB6(1) — start first conversion / enable AFE
 */

#include <stdint.h>

/* ── RAM block addresses (confirmed from disassembly) ── */
#define RAM_BLOCK1_BASE     ((volatile uint8_t *)0x018F)
#define RAM_BLOCK1_SIZE     32u    /* 0x20 bytes */

#define RAM_BLOCK2_BASE     ((volatile uint8_t *)0x01AF)
#define RAM_BLOCK2_SIZE     20u    /* 0x14 bytes */

#define RAM_BLOCK3_BASE     ((volatile uint8_t *)0x01C3)
#define RAM_BLOCK3_SIZE     8u     /* 0x08 bytes */

/* ── Per-channel output slot addresses ── */
#define SLOT_01CB   (*(volatile uint16_t *)0x01CB)   /* 2-byte word  */
#define SLOT_01CD   (*(volatile uint8_t  *)0x01CD)   /* ch A byte 0  */
#define SLOT_01CE   (*(volatile uint8_t  *)0x01CE)   /* ch B byte 0  */
#define SLOT_01CF   (*(volatile uint8_t  *)0x01CF)   /* ch C byte 0  */
#define SLOT_01D0   (*(volatile uint8_t  *)0x01D0)   /* ch A byte 1  */
#define SLOT_01D1   (*(volatile uint8_t  *)0x01D1)   /* ch B byte 1  */
#define SLOT_01D2   (*(volatile uint8_t  *)0x01D2)   /* ch C byte 1  */
#define SLOT_01D3   (*(volatile uint8_t  *)0x01D3)   /* ch D byte 0  */
#define SLOT_01D4   (*(volatile uint8_t  *)0x01D4)   /* ch D byte 1  */
#define SLOT_01D5   (*(volatile uint8_t  *)0x01D5)   /* ch E byte 0  */
#define SLOT_01D6   (*(volatile uint8_t  *)0x01D6)   /* ch E byte 1  */
#define SLOT_01D7   (*(volatile uint8_t  *)0x01D7)   /* ch F byte 0  */
#define SLOT_01D8   (*(volatile uint8_t  *)0x01D8)   /* ch F byte 1  */
#define SLOT_01D9   (*(volatile uint8_t  *)0x01D9)   /* ch G byte 0  */
#define SLOT_01DA   (*(volatile uint8_t  *)0x01DA)   /* ch G byte 1  */
#define SLOT_01DB   (*(volatile uint16_t *)0x01DB)   /* pack word     */
#define SLOT_01DD   (*(volatile uint8_t  *)0x01DD)   /* status/flag   */
#define SLOT_01DE   (*(volatile uint16_t *)0x01DE)   /* pack word 2   */
#define SLOT_01E0   (*(volatile uint16_t *)0x01E0)   /* pack word 3   */

/* ── External AFE/peripheral init functions ── */
extern void afe_hardware_init(void);                 /* FUN_009C47   */
extern void afe_configure(uint16_t base_addr,        /* FUN_009C6C   */
                          uint8_t  param_a,
                          uint8_t  param_b,
                          uint8_t  num_cells,
                          uint8_t  num_temps,
                          uint8_t  num_channels);
extern void afe_start_conversion(uint8_t enable);    /* FUN_009DB6   */

/* ── Zero-clear helper ── */
static void clear_ram_block(volatile uint8_t *base, uint16_t size)
{
    uint16_t i;
    for (i = size; i != 0u; i--)
    {
        base[i - 1u] = 0u;   /* matches the decrement-then-clear pattern */
    }
}

/*
 * init_measurement_buffers
 *
 * Clears all AFE measurement RAM buffers and per-channel output
 * slots, then initialises the AFE hardware and starts the first
 * conversion cycle.
 *
 * Call this once at power-on, or after a fault-triggered reset
 * before re-starting the measurement cycle.
 */
void init_measurement_buffers(void)
{
    /* ── Step 1: Zero-clear the three RAM data blocks ── */

    /* Block 1: raw sensor / ADC shadow region (32 bytes) */
    clear_ram_block(RAM_BLOCK1_BASE, RAM_BLOCK1_SIZE);

    /* Block 2: intermediate calculation region (20 bytes) */
    clear_ram_block(RAM_BLOCK2_BASE, RAM_BLOCK2_SIZE);

    /* Block 3: AFE conversion result buffer (8 bytes) */
    clear_ram_block(RAM_BLOCK3_BASE, RAM_BLOCK3_SIZE);

    /* ── Step 2: Zero-clear all per-channel output slots ── */
    /*    Matches the sequence of individual clr / clrw instructions   */

    SLOT_01CB = 0u;    /* 2-byte word at 0x01CB/0x01CC                  */
    SLOT_01CD = 0u;    /* channel A byte 0                              */
    SLOT_01D0 = 0u;    /* channel A byte 1                              */
    SLOT_01CE = 0u;    /* channel B byte 0                              */
    SLOT_01D1 = 0u;    /* channel B byte 1                              */
    SLOT_01CF = 0u;    /* channel C byte 0                              */
    SLOT_01D2 = 0u;    /* channel C byte 1                              */
    SLOT_01D3 = 0u;    /* channel D byte 0                              */
    SLOT_01D4 = 0u;    /* channel D byte 1                              */
    SLOT_01D5 = 0u;    /* channel E byte 0                              */
    SLOT_01D6 = 0u;    /* channel E byte 1                              */
    SLOT_01D7 = 0u;    /* channel F byte 0                              */
    SLOT_01D8 = 0u;    /* channel F byte 1                              */
    SLOT_01D9 = 0u;    /* channel G byte 0                              */
    SLOT_01DA = 0u;    /* channel G byte 1                              */
    SLOT_01DB = 0u;    /* pack measurement word (2 bytes)               */
    SLOT_01DD = 0u;    /* status / flag byte                            */
    SLOT_01DE = 0u;    /* pack measurement word 2 (2 bytes)             */
    SLOT_01E0 = 0u;    /* pack measurement word 3 (2 bytes)             */

    /* ── Step 3: AFE hardware init ── */
    /*    Matches: callf 0x009C47                                        */
    afe_hardware_init();

    /* ── Step 4: AFE configuration ── */
    /*    Confirmed push sequence: push 0x10,0x00,0x01,0x00             */
    /*    ldw X,0x0010  ldw X,0x1A80  ldw X,0x0006; callf 0x009C6C     */
    /*    Likely: base=0x1A80, 16 cells, 6 temp sensors                 */
    afe_configure(0x1A80,   /* AFE I2C/SPI register base address        */
                  0x00,     /* param a (sub-address or mode)             */
                  0x01,     /* param b (enable/disable flag)             */
                  0x10,     /* number of cells (16)                      */
                  0x10,     /* number of temperature sensors (16)        */
                  0x06);    /* number of channels / sample rate config   */

    /* ── Step 5: Start first conversion ── */
    /*    Matches: ld A, #0x01; callf 0x009DB6                          */
    afe_start_conversion(1u);
}
