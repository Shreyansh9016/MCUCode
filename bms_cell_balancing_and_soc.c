/*
 * bms_cell_balancing_and_soc.c
 *
 * Original address : 0x00C2DA  /  FUN_00c2da
 * Function name    : bms_cell_balancing_and_soc
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 * This is the BMS CELL BALANCING SELECTION + SOC COULOMB COUNTING
 * function — one of the most important in the firmware.
 * Called from bms_main_loop every 4 ticks (alongside CAN TX).
 *
 * ── Part 1: Cell balancing candidate search ──
 *
 * Entry guard (from disassembly at 0x00C313-0x00C328):
 *   cpw X, #0x1770   → if local_46[0x10] >= 6000 (0x1770): skip Part 1
 *   tnz 0x4054        → if DAT_004054 != 0 (already balancing): skip Part 1
 *
 * 0x1770 = 6000 decimal. local_46[0x10] is the 16th slot in
 * the cell voltage buffer. This guard means: only search for a
 * balance candidate if all cell voltages are below 6V AND no
 * balancing cycle is already active.
 *
 * The search loop (unrolled by the compiler into 100 iterations):
 *   For each cell slot i = 0 to 99:
 *     float_load_buf(buf + 0x45)      → load cell_voltage[i] as float
 *     result = float_compare(0x825C - i*4)  → compare vs threshold[i]
 *     if voltage < threshold_lo:
 *         cell_index = i              → this cell needs balance (too low)
 *         goto found_candidate
 *     float_load_buf(buf + 0x45)      → re-load same cell
 *     result = float_compare(next string)  → compare vs threshold_hi[i]
 *     if voltage < threshold_hi:
 *         cell_index = i+1            → this cell is in the balance zone
 *         goto found_candidate
 *
 * String table addresses used for thresholds (confirmed disassembly):
 *   0x825C, 0x8258, 0x8254, 0x8250, 0x824C, 0x8248, 0x8244, 0x8240,
 *   0x823C, 0x8238, 0x8234, 0x8230, 0x822C, 0x8228, 0x8224, 0x8220,
 *   0x821C, 0x8218, 0x8214, 0x8210, 0x820C, 0x8208, 0x8204, 0x8200,
 *   0x81FC, 0x81F8, 0x81F4, 0x81F0, 0x81EC, 0x81E8, 0x81E4, 0x81E0,
 *   0x81DC, 0x81D8, 0x81D4, 0x81D0, 0x81CC, 0x81C8, 0x81C4, 0x81C0,
 *   0x81BC, 0x81B8, 0x81B4, 0x81B0, 0x81AC, 0x81A8, 0x81A4, 0x81A0,
 *   0x819C, 0x8198, 0x8194, 0x8190, 0x818C, 0x8188, 0x8184, 0x8180,
 *   0x817C, 0x8178, 0x8174, 0x8170, 0x816C, 0x8168, 0x8164, 0x8160,
 *   0x815C, 0x8158, 0x8154, 0x8150, 0x814C, 0x8148, 0x8144, 0x8140,
 *   0x813C, 0x8138, 0x8134, 0x8130, 0x812C, 0x8128, 0x8124, 0x8120,
 *   0x811C, 0x8118, 0x8114, 0x8110, 0x810C, 0x8108, 0x8104, 0x8100,
 *   0x80FC, 0x80F8, 0x80F4, 0x80F0, 0x80EC, 0x80E8, 0x80E4, 0x80E0,
 *   0x80DC
 *
 * These step backward by 4 bytes through the string table, which
 * contains the voltage/temperature threshold constants we identified
 * in our initial string analysis. The first ~16 entries (0x825C-0x8220)
 * correspond to cell voltage thresholds; higher indices cover
 * temperature and pack-level thresholds.
 *
 * Three special entries use FUN_00f1c7 (register-load) instead of
 * FUN_00f7dd (buffer-load) at indices 0x27, 0x40, 0x5E:
 *   index 0x27: FUN_00f1c7(0x38, ...) → temperature register 0x38
 *   index 0x40: FUN_00f1c7(0x3A, ...) → temperature register 0x3A
 *   index 0x5E: FUN_00f1c7(0x40, ...) → temperature register 0x40
 *
 * found_candidate:
 *   DAT_004054 = 1                   → set "balancing active" flag
 *   FUN_00f622(cell_index)           → compute balance target value
 *   store result → RAM[0x0249]       → balance target current/time
 *   DAT_004054 already set above     → (confirmed via tnz at 0xD547)
 *
 * ── Part 2: Balance current integration (if balancing active) ──
 *
 * Confirmed from disassembly at 0x00D547-0x00D5A5:
 *
 *   cell_idx = RAM[0x0041 + SP]  (the found cell index, 0-100)
 *   Check MSB (bit 15) of cell_idx to determine sign:
 *
 *   if (cell_idx & 0x8000) == 0:  (positive / discharge direction)
 *     balance_current = cell_idx / 100     [FUN_00f893 = integer divide]
 *     float_add_to(RAM[0x0249], balance_current)  [FUN_00f454]
 *
 *   else:  (negative / charge direction)
 *     negate and compute: neg_idx = ~cell_idx
 *     balance_current = neg_idx / 100      [FUN_00f82f]
 *     float_sub_from(RAM[0x0249], balance_current)  [FUN_00f497]
 *
 * RAM[0x0249] = SOC-related accumulator (coulomb count or balance Ah)
 *
 * ── Part 3: SOC clamping and final calculation ──
 *
 * Confirmed from disassembly at 0x00D5A6-0x00D670:
 *
 *   if (RAM[0x0249] >= 0):     (accumulator is positive)
 *     float_load(buf[0x37])   → load current pack measurement
 *     compare with RAM[0x0249]
 *     if measurement > accumulator:
 *       compute SOC adjustment using FUN_00f3d0 + FUN_00f585
 *       RAM[0x0264] = new_soc   → store SOC result
 *       goto final
 *
 *   if (cell_index > 100):     (no valid cell found, or search exhausted)
 *     clamp SOC to 100:
 *       RAM[0x0264] = 100
 *       float constant = 0xFA40 = 64064 (SOC 100% float representation)
 *   if (cell_index == 0):
 *     clear SOC accumulators:
 *       RAM[0x024B] = 0
 *       RAM[0x0249] = 0
 *       RAM[0x0264] = 0
 *
 * final:
 *   FUN_00f7dd(RAM[0x0249])    → load SOC accumulator as float
 *   FUN_00f3d0(0x80D4)         → multiply by threshold from flash
 *   result = FUN_00f585()      → convert to integer SOC%
 *   RAM[0x0040+SP] = result    → local result
 *   FUN_00f5e8(result)         → float-encode result
 *   FUN_00f4a5(buf[0x45+SP])   → store to measurement buffer slot
 *   RAM[0x0204] = FUN_00f585() → final SOC stored to output struct
 *   DAT_00505F = 0             → clear TX-busy flag
 *   DAT_00406D = DAT_000265    → update SOC calibration byte
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Measurement buffer — same layout as throughout the firmware ── */
#define MEAS_BUF_CELL_VOLTAGE_OFFSET  0x45u   /* float start in local buf */

/* ── Control flags ── */
#define FLAG_BALANCING_ACTIVE  (*(volatile uint8_t  *)0x4054u)   /* 0=idle,1=active */
#define TX_BUSY_FLAG           (*(volatile uint8_t  *)0x505Fu)
#define ACTIVITY_MARKER        (*(volatile uint8_t  *)0x5064u)

/* ── SOC accumulators and results ── */
#define SOC_ACCUMULATOR        (*(volatile int16_t  *)0x0249u)   /* coulomb count   */
#define SOC_ACCUMULATOR_HI     (*(volatile uint16_t *)0x024Bu)   /* hi word         */
#define SOC_RESULT             (*(volatile uint16_t *)0x0264u)   /* final SOC %×100 */
#define SOC_FINAL_OUTPUT       (*(volatile uint16_t *)0x0204u)   /* output struct   */
#define SOC_CAL_BYTE           (*(volatile uint8_t  *)0x00406Du)
#define SOC_CAL_SOURCE         (*(volatile uint8_t  *)0x000265u)

/* ── Balance threshold (6V max cell voltage for balancing) ── */
#define BALANCE_MAX_CELL_MV    6000u   /* 0x1770 = 6000 mV */

/* ── Number of threshold comparison entries ── */
#define NUM_BALANCE_THRESHOLDS 100u

/* ── Flash threshold string table (cell/temp voltage bounds) ── */
#define THRESH_TABLE_START     ((const volatile uint8_t *)0x825Cu)
#define THRESH_TABLE_STEP      4u   /* bytes per entry (confirmed: 0x825C,0x8258,...) */

/* ── Special temperature register indices in the scan ── */
#define TEMP_REG_IDX_0         0x27u   /* uses register 0x38 */
#define TEMP_REG_IDX_1         0x40u   /* uses register 0x3A */
#define TEMP_REG_IDX_2         0x5Eu   /* uses register 0x40 */

/* ── Float arithmetic helpers ── */
extern void     float_load_buf      (const uint8_t *buf_ptr);      /* FUN_00F7DD */
extern int8_t   float_compare       (const volatile uint8_t *thr); /* FUN_00F39A */
extern void     float_load_reg      (uint8_t reg, uint16_t ctx);   /* FUN_00F1C7 */
extern void     float_store         (uint8_t *dst);                /* FUN_00F817 */
extern void     float_ext           (void);                        /* FUN_00F7EF */
extern void     float_mul_threshold (const volatile uint8_t *thr); /* FUN_00F4A5 */
extern uint16_t float_to_int        (void);                        /* FUN_00F585 */
extern void     float_from_int      (uint16_t val);                /* FUN_00F5E8 */
extern void     float_load_const    (const void *ptr);             /* FUN_00F6C7 */
extern void     float_accum_add     (uint8_t *acc_ptr);            /* FUN_00F454 */
extern void     float_accum_sub     (uint8_t *acc_ptr);            /* FUN_00F497 */
extern void     float_mul_inv       (const volatile uint8_t *thr); /* FUN_00F3D0 */
extern void     float_encode        (uint16_t val);                /* FUN_00F8BE */
extern uint16_t int_divide          (uint16_t num, uint8_t den);   /* FUN_00F82F */
extern void     float_set_int       (uint16_t val);                /* FUN_00F893 */
extern uint16_t balance_compute_target(uint16_t cell_idx);         /* FUN_00F622 */
extern void     float_load_const_77f(uint8_t mode);               /* FUN_00F77F */

/* ── Buffer copy helpers (same as rest of firmware) ── */
extern void copy_raw_block_128bytes(uint8_t *dst);     /* FUN_0096EF */
extern void copy_status_block_64bytes(uint8_t *dst);   /* FUN_009082 */
extern void copy_raw_block_32bytes(uint8_t *dst);      /* FUN_009705 */

/*
 * bms_cell_balancing_and_soc
 *
 * Scans all cell voltages against balance thresholds, selects the
 * first cell that needs balancing, then integrates coulomb count
 * and computes the final State of Charge percentage.
 */
void bms_cell_balancing_and_soc(void)
{
    /* ── Local measurement buffer layout ── */
    uint8_t  meas_buf[0x48u];          /* stack buffer for measurements  */
    uint16_t cell_voltages[26u];       /* local_46: cell voltages        */
    uint16_t soc_init;                 /* local_f[0]: 0xFA40 float const */
    uint16_t soc_one;                  /* local_11: 1 (SOC = 1 initially)*/
    int16_t  cell_index = 0;           /* which cell triggered balance   */
    uint16_t balance_target = 0;       /* computed balance target        */
    bool     candidate_found = false;
    uint8_t  i;

    /* Initialise float constants */
    soc_init = 0xFA40u;   /* 100.0 in BMS float encoding */
    soc_one  = 1u;

    /* ── Step 1: Load measurements into local buffers ── */
    copy_raw_block_128bytes (meas_buf + 7u);    /* FUN_0096EF: 0x18F→0x20E */
    copy_status_block_64bytes(meas_buf + 0x27u); /* FUN_009082: 0x1AF→0x1EE */
    copy_raw_block_32bytes  (meas_buf + 0x3Bu); /* FUN_009705: 0x1C3→0x1E2 */

    /* Copy derived values from measurement struct into locals */
    cell_voltages[0x12] = *(uint16_t *)(meas_buf + 0x29u);  /* local_4 */
    cell_voltages[0x11] = *(uint16_t *)(meas_buf + 0x27u);  /* local_6 */

    /* Mark activity */
    ACTIVITY_MARKER = 0x56u;

    /* ── Step 2: Balance candidate search ──
     *
     * Only run if:
     *   a) Max cell voltage (index 0x10) is below 6V (6000 mV)
     *   b) No balancing cycle is already running
     */
    if ((cell_voltages[0x10] < BALANCE_MAX_CELL_MV) &&
        (FLAG_BALANCING_ACTIVE == 0u))
    {
        /*
         * Scan each cell pair (lower threshold / upper threshold)
         * against its calibrated balance voltage range.
         *
         * The measurement buffer at offset 0x45 contains cell voltages
         * as floats. The string table entries at (0x825C - i*4) contain
         * the calibrated threshold values for each cell and each half
         * of the balance window.
         *
         * The compiler fully unrolled this loop (100 iterations =
         * 200 compare calls total, one per threshold boundary).
         * Here we reconstruct the logical loop.
         */
        for (i = 0u; i < NUM_BALANCE_THRESHOLDS; i++)
        {
            const volatile uint8_t *thr_lo;
            const volatile uint8_t *thr_hi;
            uint8_t thr_idx_lo = (uint8_t)(i * 2u);
            uint8_t thr_idx_hi = (uint8_t)(thr_idx_lo + 1u);

            thr_lo = THRESH_TABLE_START - (thr_idx_lo * THRESH_TABLE_STEP);
            thr_hi = THRESH_TABLE_START - (thr_idx_hi * THRESH_TABLE_STEP);

            /* Lower boundary: load cell float from buffer, compare */
            if (i == TEMP_REG_IDX_0)
                float_load_reg(0x38u, 0u);   /* temperature register */
            else if (i == TEMP_REG_IDX_1)
                float_load_reg(0x3Au, 0u);
            else if (i == TEMP_REG_IDX_2)
                float_load_reg(0x40u, 0u);
            else
                float_load_buf(meas_buf + MEAS_BUF_CELL_VOLTAGE_OFFSET);

            if (float_compare(thr_lo) < 0)
            {
                /* Cell voltage below lower threshold → balance candidate */
                cell_index      = (int16_t)i;
                candidate_found = true;
                break;
            }

            /* Upper boundary: same cell, next threshold */
            if (i == TEMP_REG_IDX_0)
                float_load_reg(0x38u, 0u);
            else if (i == TEMP_REG_IDX_1)
                float_load_reg(0x3Au, 0u);
            else if (i == TEMP_REG_IDX_2)
                float_load_reg(0x40u, 0u);
            else
                float_load_buf(meas_buf + MEAS_BUF_CELL_VOLTAGE_OFFSET);

            if (float_compare(thr_hi) < 0)
            {
                /* Cell voltage in balance window → select this cell */
                cell_index      = (int16_t)(i + 1u);
                candidate_found = true;
                break;
            }
        }

        if (!candidate_found)
        {
            /* No cell needs balancing — default to cell 100 */
            cell_index = (int16_t)NUM_BALANCE_THRESHOLDS;
        }

        /* ── Activate balancing ── */
        FLAG_BALANCING_ACTIVE = 1u;

        /*
         * Compute balance target from cell_index.
         * FUN_00F622 takes the cell index and returns a target
         * current/time value based on the cell's position in the
         * SOC map (confirmed: ldw X,(0x43,SP); callf 0x00F622).
         * Result stored to RAM[0x0249] (SOC accumulator).
         */
        balance_target = balance_compute_target((uint16_t)cell_index);
        float_store((uint8_t *)&SOC_ACCUMULATOR);
    }

    /* ── Step 3: Balance current integration ──
     *
     * Only runs if FLAG_BALANCING_ACTIVE (DAT_004054 != 0).
     * Integrates the balance current into the SOC accumulator.
     */
    if (FLAG_BALANCING_ACTIVE != 0u)
    {
        if ((cell_index & (int16_t)0x8000) == 0)
        {
            /*
             * Positive cell_index: discharge direction.
             * Divide by 100 to get per-tick current increment,
             * then add to SOC accumulator.
             * Confirmed: div X,A (#100); callf FUN_00F893; FUN_00F454
             */
            uint16_t incr = (uint16_t)cell_index / 100u;
            float_set_int(incr);
            float_accum_add((uint8_t *)&SOC_ACCUMULATOR);
        }
        else
        {
            /*
             * Negative cell_index: charge direction.
             * Negate, scale, subtract from SOC accumulator.
             * Confirmed: ldw X,#0xFFFF; subw X,(0x41,SP);
             *            callf FUN_00F82F(100,...); FUN_00F5E8; FUN_00F497
             */
            uint16_t neg_idx = (uint16_t)(~cell_index);
            float_from_int(300u);       /* 0x012C = 300 — scaling constant */
            float_store(meas_buf + 1u);
            float_load_buf((uint8_t *)&SOC_ACCUMULATOR);
            if (float_compare(meas_buf + 1u) > 0)
            {
                /* Accumulator exceeds 300 — negate path */
                uint16_t scaled = int_divide(100u, neg_idx);
                float_from_int(scaled);
                float_accum_sub((uint8_t *)&SOC_ACCUMULATOR);
            }
        }
    }

    /* ── Step 4: SOC final calculation ──
     *
     * Compare accumulator against current measurement to
     * derive the final SOC percentage (0-100).
     */
    if (SOC_ACCUMULATOR >= 0)
    {
        /* Load current float measurement from local buffer */
        float_load_buf(meas_buf + 0x37u);
        float_encode(0u);
        float_store(meas_buf + 1u);

        /* Load SOC accumulator and compare */
        float_load_buf((uint8_t *)&SOC_ACCUMULATOR);
        float_store(meas_buf + 1u);

        if (float_compare(meas_buf + 1u) > 0)
        {
            /* Current > accumulator: compute forward SOC */
            float_load_buf(meas_buf + 0x37u);
            float_encode(0u);
            float_store(meas_buf + 1u);
            float_load_buf((uint8_t *)&SOC_ACCUMULATOR);
            float_mul_inv((const volatile uint8_t *)0x80D8u);
            SOC_RESULT = float_to_int();
            goto final_calc;
        }
    }

    /* ── SOC clamping ── */
    if (cell_index > (int16_t)NUM_BALANCE_THRESHOLDS)
    {
        /* No cell found (exhausted search): clamp SOC to 100% */
        SOC_RESULT    = 100u;
        float_load_const_77f(1u);     /* load 100.0 float constant */
        float_store((uint8_t *)&SOC_ACCUMULATOR);
    }

    if (cell_index == 0)
    {
        /* Cell index zero: reset all SOC state */
        SOC_ACCUMULATOR_HI = 0u;
        SOC_ACCUMULATOR    = 0;
        SOC_RESULT         = 0u;
    }

final_calc:
    /*
     * Final SOC calculation:
     *   1. Load SOC accumulator as float
     *   2. Multiply by calibration factor from flash 0x80D4
     *   3. Convert to integer percentage
     *   4. Store to output measurement struct
     *
     * Confirmed from disassembly 0x00D63E-0x00D670:
     *   ldw X,#0x0249; callf FUN_00F7DD
     *   ldw X,#0x80D4; callf FUN_00F3D0
     *   callf FUN_00F585  → result in X
     *   ldw (0x40,SP),X
     *   FUN_00F5E8(result)
     *   FUN_00F4A5(buf[0x45+SP])
     *   FUN_00F585() → DAT_000204
     */
    float_load_buf((uint8_t *)&SOC_ACCUMULATOR);
    float_mul_inv((const volatile uint8_t *)0x80D4u);
    {
        uint16_t final_soc = float_to_int();
        float_from_int(final_soc);
        float_mul_threshold(meas_buf + MEAS_BUF_CELL_VOLTAGE_OFFSET);
        SOC_FINAL_OUTPUT = float_to_int();
    }

    /* Clear TX-busy flag and update SOC calibration byte */
    TX_BUSY_FLAG  = 0u;
    SOC_CAL_BYTE  = SOC_CAL_SOURCE;
}
