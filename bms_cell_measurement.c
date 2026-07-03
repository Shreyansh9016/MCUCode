/*
 * bms_cell_measurement.c
 *
 * Reconstructed from Ghidra decompilation of STM8 BMS firmware.
 * Original address : 0x008D9E
 * Original name    : FUN_008d9e
 *
 * What this function does:
 * -------------------------
 * Reads two 16-bit raw ADC/register values from memory-mapped BMS
 * registers (0x8088 and 0x808A — likely cell voltage or current sense
 * registers on the BQ76952 AFE), then runs 10 sequential measurement
 * cycles. Each cycle:
 *   1. Loads a source value (FUN_load_value)
 *   2. Reads a register at 0x8084 (FUN_read_register — likely an AFE
 *      status or cell-voltage register base)
 *   3. Processes / formats the result (FUN_process)
 *   4. Stores the result into a 4-byte slot in an output array
 *      (FUN_store_result), offset advancing by 4 bytes each cycle
 *      (slots 0x00 through 0x24, i.e. 10 × uint32_t entries)
 *
 * The 10 iterations and 4-byte stride strongly suggest this is
 * populating a packed array of 10 cell measurements — e.g.
 * cell voltages, temperatures, or impedance values — for later
 * use by the BMS state machine.
 *
 * Function signatures below are inferred from call-site analysis;
 * rename them once you confirm the actual peripheral map.
 */

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Memory-mapped BMS registers (base 0x8000 region)                    */
/* Adjust addresses to match your actual hardware register map.        */
/* ------------------------------------------------------------------ */
#define REG_CELL_BASE       (*(volatile uint16_t *)0x8084)  /* AFE cell-voltage / status base */
#define REG_MEASUREMENT_A   (*(volatile uint16_t *)0x808A)  /* Raw measurement channel A      */
#define REG_MEASUREMENT_B   (*(volatile uint16_t *)0x8088)  /* Raw measurement channel B      */

/* ------------------------------------------------------------------ */
/* Forward declarations for helper functions                            */
/* Rename these once you identify them in the disassembly.             */
/* ------------------------------------------------------------------ */

/*
 * FUN_load_value — called once at startup, likely initialises the
 * measurement engine or performs a one-shot calibration read.
 * Called with a stack pointer offset; treated as void here.
 */
extern void FUN_load_value(void *stack_ctx);

/*
 * FUN_read_register — reads a 16-bit value from the given register
 * address. Called every cycle with &REG_CELL_BASE.
 * Returns the raw register word.
 */
extern uint16_t FUN_read_register(volatile uint16_t *reg_addr);

/*
 * FUN_process — processes / formats the raw 16-bit register value.
 * Takes a pointer to the raw value; likely applies scaling or
 * unit conversion (e.g. ADC counts → millivolts).
 */
extern void FUN_process(uint8_t *raw_value_ptr);

/*
 * FUN_store_result — stores a 16-bit result into the measurement
 * output array at the given byte offset.
 * Signature inferred from 10 calls with offsets 0x00..0x24.
 */
extern void FUN_store_result(uint16_t value, uint8_t byte_offset);

/* ------------------------------------------------------------------ */
/* Output array — 10 cell measurement slots × 4 bytes each            */
/* ------------------------------------------------------------------ */
#define CELL_COUNT          10
#define CELL_SLOT_SIZE      4       /* bytes per slot (matches 0x24 / 9 stride) */

/* ------------------------------------------------------------------ */
/* Main function — reads and stores 10 cell measurements               */
/* ------------------------------------------------------------------ */

void read_cell_measurements(void)
{
    /*
     * Read the two 16-bit source registers once upfront.
     * In the original disassembly these map to DAT_00808A and DAT_008088.
     * Likely a pair of cell-voltage or differential-sense values that
     * seed the 10-cycle measurement loop below.
     */
    uint16_t channel_a = REG_MEASUREMENT_A;   /* DAT_00808A */
    uint16_t channel_b = REG_MEASUREMENT_B;   /* DAT_008088 */

    /* High and low bytes, used as separate arguments in the loop */
    uint8_t ch_a_hi = (uint8_t)(channel_a >> 8);
    uint8_t ch_a_lo = (uint8_t)(channel_a & 0xFF);
    uint8_t ch_b_hi = (uint8_t)(channel_b >> 8);
    uint8_t ch_b_lo = (uint8_t)(channel_b & 0xFF);

    /*
     * One-shot initialisation call before the measurement loop.
     * Original: FUN_0096fa — possibly a peripheral reset / start-of-
     * conversion trigger on the AFE or I2C bus.
     */
    FUN_load_value(NULL);   /* FUN_0096fa in original */

    /*
     * 10-cycle measurement loop.
     *
     * Each iteration:
     *   a) Supply a source word to the measurement engine
     *      (FUN_00f893 — likely "set channel / write operand")
     *   b) Read the AFE register at 0x8084
     *      (FUN_00f4a5 — always the same address, likely "read ADC")
     *   c) Process / scale the raw reading
     *      (FUN_00f2bd — likely "apply gain / unit conversion")
     *   d) Store the processed result into the output array
     *      (FUN_00f817 — offset advances 4 bytes per cycle)
     *
     * The source words rotate through combinations of ch_a / ch_b
     * high and low bytes (CONCAT11 in Ghidra = combine two uint8_t
     * into a uint16_t), cycling through the available channels.
     * This pattern is consistent with a BQ76952 that multiplexes
     * cell inputs and reads them one at a time.
     */

    /*
     * Source words, derived from channel_a and channel_b bytes.
     * In Ghidra: CONCAT11(hi_byte, lo_byte) = (hi << 8) | lo
     */
    uint16_t src[CELL_COUNT];
    src[0] = (uint16_t)((ch_b_lo << 8) | ch_a_hi);   /* cycle 0  */
    src[1] = (uint16_t)((ch_b_hi << 8) | ch_b_lo);   /* cycle 1  */
    src[2] = (uint16_t)((ch_a_lo << 8) | ch_b_hi);   /* cycle 2  */
    src[3] = (uint16_t)((ch_a_hi << 8) | ch_a_lo);   /* cycle 3  */
    src[4] = (uint16_t)((ch_b_lo << 8) | ch_a_hi);   /* cycle 4  — pattern repeats */
    src[5] = (uint16_t)((ch_b_hi << 8) | ch_b_lo);   /* cycle 5  */
    src[6] = (uint16_t)((ch_a_lo << 8) | ch_b_hi);   /* cycle 6  */
    src[7] = (uint16_t)((ch_a_hi << 8) | ch_a_lo);   /* cycle 7  */
    src[8] = (uint16_t)((ch_b_lo << 8) | ch_a_hi);   /* cycle 8  */
    src[9] = (uint16_t)((ch_b_hi << 8) | ch_b_lo);   /* cycle 9  */

    for (uint8_t i = 0; i < CELL_COUNT; i++)
    {
        uint8_t slot_offset = (uint8_t)(i * CELL_SLOT_SIZE);  /* 0x00, 0x04, 0x08 ... 0x24 */
        uint8_t raw_lo      = (uint8_t)(src[i] & 0xFF);

        /* Step a: load source value into measurement engine */
        FUN_read_register((volatile uint16_t *)(uintptr_t)src[i]);   /* FUN_00f893 */

        /* Step b: read the AFE cell-voltage register */
        FUN_read_register(&REG_CELL_BASE);                            /* FUN_00f4a5 */

        /* Step c: process / scale the raw reading */
        FUN_process(&raw_lo);                                         /* FUN_00f2bd */

        /* Step d: store result into output array at this slot */
        FUN_store_result(src[i], slot_offset);                        /* FUN_00f817 */
    }
}
