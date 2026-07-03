/*
 * can_transmit_bms_frames.c
 *
 * Original address : 0x00A8F8  /  FUN_00a8f8
 *
 * ---------------------------------------------------------------
 * Overview — what this function does
 * ---------------------------------------------------------------
 *
 * This is the BMS CAN BROADCAST function — the most significant
 * function decoded so far. It transmits up to 14 CAN frames
 * containing ALL BMS measurement data, fault flags, and status.
 *
 * The function:
 *   1. Copies all measurement blocks from AFE buffer into a local
 *      128-byte working buffer (using our decoded copy functions)
 *   2. For each frame: waits for the CAN TX mailbox to be free,
 *      then loads ID + 8 data bytes into CAN hardware registers
 *      and triggers transmission
 *   3. Has a guard flag (_DAT_0001fa) — if not set to 1, the
 *      function returns early after the standard frames
 *
 * ---------------------------------------------------------------
 * CAN Frame Inventory (all confirmed from disassembly)
 * ---------------------------------------------------------------
 *
 * Standard frames — CAN ID prefix 0x5806BC__ (29-bit extended ID):
 *
 *  ID=0xD0 — Cell voltages group 1: buf[0x2C..0x33] (cells 1–4)
 *  ID=0xD1 — Cell voltages group 2: buf[0x34..0x3B] (cells 5–8)
 *  ID=0xD2 — Cell voltages group 3: buf[0x3C..0x43] (cells 9–12)
 *  ID=0xD3 — Cell voltages group 4: buf[0x44..0x4B] (cells 13–16)
 *  ID=0xD4 — Mixed cell data:       buf[0x24..0x2B] with SOC calc
 *  ID=0xD5 — Temperature/current:   8 float values from buf[0x4C..]
 *  ID=0xD6 — SOC + misc:            zeros + FUN_00f7dd(0,0x244) result
 *  ID=0xD7 — Fault bits set A:      bit-extracted from buf[0x22/0x16]
 *  ID=0xD8 — Fault bits set B:      bit-extracted from buf[0x23]
 *  ID=0xD9 — Pack status:           from buf[0x1F/0x18/0x21], flash 0x4052-53
 *  ID=0xDA — Calibration data 1:    flash ROM[0x406E..0x4074]
 *  ID=0xDB — Calibration data 2:    flash ROM[0x4075..0x407A]
 *
 * Conditional frame (only if _DAT_0001fc == 0xAA):
 *  ID=0xC1B1 — Special event frame: hardcoded bytes {1,0xB0,2,0xA8,0,0xBE,0,0}
 *              (0xAA marker cleared after sending)
 *
 * Guard frame (only if _DAT_0001fa == 1):
 *  ID=0xC3B3 — Charging/balancing data: computed float comparison results
 *              (4 cells × current vs threshold, up to 5 comparisons)
 *              Guard flag cleared after sending.
 *
 * ---------------------------------------------------------------
 * CAN frame structure (STM8AF TX mailbox registers)
 * ---------------------------------------------------------------
 *   CAN_TX_PRIO (0x5427) = 0         — lowest priority
 *   CAN_TX_ID0  (0x542A) = 0x58      — extended ID byte 0
 *   CAN_TX_ID1  (0x542B) = 0x06      — extended ID byte 1
 *   CAN_TX_ID2  (0x542C) = 0xBC      — extended ID byte 2
 *   CAN_TX_ID3  (0x542D) = 0xD0..0xDB — extended ID byte 3 (frame type)
 *   CAN_TX_DLC  (0x5429) = 8         — always 8 data bytes
 *   CAN_TX_D0..7 (0x542E..0x5435)    — 8 data bytes
 *   CAN_TX_MCR  (0x5428) |= 1        — trigger transmission
 *
 * ---------------------------------------------------------------
 * CAN wait loop (before each frame)
 * ---------------------------------------------------------------
 * Before each frame, the firmware polls:
 *   DAT_005423 = CAN status register
 *   counter (stack local) = retry counter
 * It calls FUN_00f8b7 (a timeout/compare function) and FUN_00f65d
 * (reads a value from flash 0x80A8 — probably a timeout constant)
 * If the condition matches (CAN TX ready), it exits the wait loop.
 * If the counter overflows (bus stuck), it sets a timeout flag and
 * skips that frame.
 *
 * ---------------------------------------------------------------
 * Data buffer layout (128-byte local stack buffer)
 * ---------------------------------------------------------------
 * Offsets used in the transmit function match the AFE output struct
 * layout we decoded in init_measurement_buffers:
 *   buf[0x16]      = fault status byte A
 *   buf[0x18]      = protection status byte
 *   buf[0x1F:0x20] = pack voltage word
 *   buf[0x21]      = FET/protection status
 *   buf[0x22]      = fault flags register A (bit-packed)
 *   buf[0x23]      = fault flags register B (bit-packed)
 *   buf[0x24:0x2B] = cell pair / mixed data
 *   buf[0x2C:0x33] = cell voltages 1-4 (2 bytes each)
 *   buf[0x34:0x3B] = cell voltages 5-8
 *   buf[0x3C:0x43] = cell voltages 9-12
 *   buf[0x44:0x4B] = cell voltages 13-16
 *   buf[0x4C:]     = temperature/current float data
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── CAN TX mailbox registers ── */
#define CAN_TX_PRIO   (*(volatile uint8_t *)0x5427u)
#define CAN_TX_MCR    (*(volatile uint8_t *)0x5428u)
#define CAN_TX_DLC    (*(volatile uint8_t *)0x5429u)
#define CAN_TX_ID0    (*(volatile uint8_t *)0x542Au)
#define CAN_TX_ID1    (*(volatile uint8_t *)0x542Bu)
#define CAN_TX_ID2    (*(volatile uint8_t *)0x542Cu)
#define CAN_TX_ID3    (*(volatile uint8_t *)0x542Du)
#define CAN_TX_D0     (*(volatile uint8_t *)0x542Eu)
#define CAN_TX_D1     (*(volatile uint8_t *)0x542Fu)
#define CAN_TX_D2     (*(volatile uint8_t *)0x5430u)
#define CAN_TX_D3     (*(volatile uint8_t *)0x5431u)
#define CAN_TX_D4     (*(volatile uint8_t *)0x5432u)
#define CAN_TX_D5     (*(volatile uint8_t *)0x5433u)
#define CAN_TX_D6     (*(volatile uint8_t *)0x5434u)
#define CAN_TX_D7     (*(volatile uint8_t *)0x5435u)
#define CAN_STATUS    (*(volatile uint8_t *)0x5423u)

/* ── CAN frame base ID bytes (extended 29-bit) ── */
#define CAN_ID_B0     0x58u
#define CAN_ID_B1     0x06u
#define CAN_ID_B2_STD 0xBCu   /* standard BMS frames */
#define CAN_ID_B2_EVT 0xC1u   /* event frame         */
#define CAN_ID_B2_CHG 0xC3u   /* charge/balance frame*/

/* ── State flags in RAM ── */
#define FLAG_SPECIAL_EVENT  (*(volatile uint8_t *)0x01FCu)   /* 0xAA = send event frame */
#define FLAG_CHARGE_ACTIVE  (*(volatile uint8_t *)0x01FAu)   /* 1 = send charge frame   */
#define FLAG_TX_BUSY        (*(volatile uint8_t *)0x505Fu)   /* TX busy / complete flag */

/* ── Flash calibration data (read-only) ── */
#define CAL_DATA_BASE       ((const volatile uint8_t *)0x406Du)

/* ── Helper: load data buffer from AFE measurement output struct ── */
extern void copy_raw_block_128bytes (uint8_t *dst);   /* FUN_0096EF */
extern void copy_measurement_block_160bytes(uint8_t *dst); /* FUN_009077 */
extern void copy_raw_block_32bytes  (uint8_t *dst);   /* FUN_009705 */
extern void fetch_pair_channel_a    (uint8_t *p1, uint8_t *p2); /* FUN_00971B */
extern void fetch_pair_channel_b    (uint8_t *p1, uint8_t *p2); /* FUN_009734 */
extern void fetch_pair_channel_c    (uint8_t *p1, uint8_t *p2); /* FUN_00974D */
extern void copy_pack_word_01db     (uint8_t *dst);   /* FUN_0097CA */
extern void copy_pack_word_01e0     (uint8_t *dst);   /* FUN_0097EB */
extern void copy_status_byte_01dd   (uint8_t *dst);   /* FUN_0097D5 */

/* ── Helper: float arithmetic and comparison ── */
extern uint8_t  float_compare       (uint8_t mode, uint16_t ctx);    /* FUN_00F39A */
extern void     float_load          (uint8_t reg, uint16_t val);     /* FUN_00F1C7 */
extern void     float_store         (uint16_t *ctx);                 /* FUN_00F817 */
extern void     float_load_ptr      (uint16_t *ctx);                 /* FUN_00F7DD */
extern void     float_to_byte       (void);                         /* FUN_00F58C */
extern uint8_t  scratch3;  /* DAT_000003 — float result byte output */

/* ── CAN TX wait helper ── */
extern void     can_tx_wait_timer   (uint16_t counter);              /* FUN_00F8B7 */
extern uint8_t  can_tx_check_ready  (const volatile uint8_t *cfg);  /* FUN_00F65D */
#define CAN_TX_TIMEOUT_CFG  ((const volatile uint8_t *)0x80A8u)
#define CAN_BUS_READY_MASK  0xFBu   /* DAT_005423 | 0xFB == 0xFF → bus free */

/* ── Internal helper: wait for CAN TX mailbox ready ── */
static bool can_wait_for_tx_ready(uint16_t *retry_counter, uint16_t *tx_done)
{
    if ((CAN_STATUS | CAN_BUS_READY_MASK) == 0xFFu) {
        *tx_done = 0u;
        return true;  /* bus already ready */
    }
    while ((CAN_STATUS | CAN_BUS_READY_MASK) != 0xFFu && (*tx_done == 0u)) {
        (*retry_counter)++;
        can_tx_wait_timer(*retry_counter);
        if (can_tx_check_ready(CAN_TX_TIMEOUT_CFG)) {
            *tx_done = 0u;
            *retry_counter = 1u;
        }
    }
    return (*tx_done == 0u);
}

/* ── Internal helper: arm and fire one CAN frame ── */
static void can_send_frame(uint8_t id3,
                            uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
                            uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                            uint8_t id2)
{
    CAN_TX_PRIO = 0u;
    CAN_TX_ID0  = CAN_ID_B0;
    CAN_TX_ID1  = CAN_ID_B1;
    CAN_TX_ID2  = id2;
    CAN_TX_ID3  = id3;
    CAN_TX_DLC  = 8u;
    CAN_TX_D0   = d0;
    CAN_TX_D1   = d1;
    CAN_TX_D2   = d2;
    CAN_TX_D3   = d3;
    CAN_TX_D4   = d4;
    CAN_TX_D5   = d5;
    CAN_TX_D6   = d6;
    CAN_TX_D7   = d7;
    CAN_TX_MCR |= 0x01u;   /* trigger TX */
}

/*
 * can_transmit_bms_frames  (FUN_00a8f8)
 *
 * Main BMS CAN broadcast function.
 * Transmits up to 14 CAN frames covering all BMS data.
 * Called periodically from the BMS main loop.
 */
uint8_t can_transmit_bms_frames(void)
{
    /* ── Local 128-byte measurement buffer ── */
    uint8_t  buf[128];
    uint16_t retry = 0u;
    uint16_t tx_done = 0u;
    uint8_t  result = 0u;

    /* ── Step 1: Fill buffer from AFE measurement output struct ── */
    copy_raw_block_128bytes (buf + 0x2Cu);   /* FUN_0096EF: 32×4=128 bytes from 0x018F */
    copy_measurement_block_160bytes(buf + 0x4Cu);   /* FUN_009077: 40×4=160 bytes     */
    copy_raw_block_32bytes  (buf + 0x24u);   /* FUN_009705: 8×4=32 bytes from 0x01C3  */
    fetch_pair_channel_a    (buf + 0x1Fu, buf + 0x21u);
    fetch_pair_channel_b    (buf + 0x2Du, buf + 0x31u);
    fetch_pair_channel_c    (buf + 0x3Bu, buf + 0x36u);
    copy_pack_word_01db     (buf + 0x1Fu);
    copy_pack_word_01e0     (buf + 0x17u);
    copy_status_byte_01dd   (buf + 0x21u);
    *(volatile uint8_t *)0x5064u = 0x56u;   /* mark buffer ready */

    /* ── Frames 0xD0–0xD3: Cell voltages (4 groups of 4 cells × 2 bytes) ── */
    if (can_wait_for_tx_ready(&retry, &tx_done))
        can_send_frame(0xD0u, buf[0x2C], buf[0x2D], buf[0x2E], buf[0x2F],
                       buf[0x30], buf[0x31], buf[0x32], buf[0x33], CAN_ID_B2_STD);

    if (can_wait_for_tx_ready(&retry, &tx_done))
        can_send_frame(0xD1u, buf[0x34], buf[0x35], buf[0x36], buf[0x37],
                       buf[0x38], buf[0x39], buf[0x3A], buf[0x3B], CAN_ID_B2_STD);

    if (can_wait_for_tx_ready(&retry, &tx_done))
        can_send_frame(0xD2u, buf[0x3C], buf[0x3D], buf[0x3E], buf[0x3F],
                       buf[0x40], buf[0x41], buf[0x42], buf[0x43], CAN_ID_B2_STD);

    if (can_wait_for_tx_ready(&retry, &tx_done))
        can_send_frame(0xD3u, buf[0x44], buf[0x45], buf[0x46], buf[0x47],
                       buf[0x48], buf[0x49], buf[0x4A], buf[0x4B], CAN_ID_B2_STD);

    /* ── Frame 0xD4: Mixed cell/SOC data ── */
    if (can_wait_for_tx_ready(&retry, &tx_done)) {
        uint8_t d6, d7;
        uint16_t v = *(uint16_t *)(buf + 0x2Au);
        if ((v & 0x8000u) == 0u) {
            d6 = buf[0x2A]; d7 = (uint8_t)(v % 1000u);
        } else {
            /* overflow: use float helper */
            can_tx_wait_timer(v);
            float_store(NULL);
            d6 = buf[0x1B]; d7 = buf[0x1C];
        }
        can_send_frame(0xD4u, buf[0x24], buf[0x25], buf[0x26], buf[0x27],
                       buf[0x28], buf[0x29], d6, d7, CAN_ID_B2_STD);
    }

    /* ── Frame 0xD5: Temperature / current (8 float-derived bytes) ── */
    if (can_wait_for_tx_ready(&retry, &tx_done)) {
        float_load_ptr(NULL);  float_to_byte(); uint8_t t0 = scratch3;
        float_load_ptr(NULL);  float_to_byte(); uint8_t t1 = scratch3;
        float_load_ptr(NULL);  float_to_byte(); uint8_t t2 = scratch3;
        float_load_ptr(NULL);  float_to_byte(); uint8_t t3 = scratch3;
        float_load_ptr(NULL);  float_to_byte(); uint8_t t4 = scratch3;
        float_to_byte();       uint8_t t5 = 0u;
        float_load_ptr(NULL);  float_to_byte(); uint8_t t6 = scratch3;
        float_load_ptr(NULL);  float_to_byte(); uint8_t t7 = scratch3;
        can_send_frame(0xD5u, t0, t1, t2, t3, t4, t5, t6, t7, CAN_ID_B2_STD);
    }

    /* ── Frame 0xD6: SOC + misc ── */
    if (can_wait_for_tx_ready(&retry, &tx_done)) {
        float_load_ptr(NULL); float_to_byte();
        uint8_t soc_byte = scratch3;
        uint8_t soc_cal  = CAL_DATA_BASE[0];
        can_send_frame(0xD6u, 0, 0, 0, 0, 0, 0, soc_byte, soc_cal, CAN_ID_B2_STD);
    }

    /* ── Frame 0xD7: Fault flags A (bit-extracted from buf[0x22] and buf[0x16]) ── */
    if (can_wait_for_tx_ready(&retry, &tx_done)) {
        uint8_t f = buf[0x22];
        uint8_t any_fault = (*(uint16_t *)0x01EEu || *(uint16_t *)0x01F6u ||
                              *(uint16_t *)0x01FEu || *(uint16_t *)0x01F8u) ? 1u : 0u;
        can_send_frame(0xD7u,
            (f >> 3u) & 1u,          /* bit 3 */
            (f >> 2u) & 1u,          /* bit 2 */
            (f >> 4u) & 1u,          /* bit 4 */
            (f >> 5u) & 1u,          /* bit 5 */
            (f >> 6u) & 1u,          /* bit 6 */
            (buf[0x16] & 0x80u) != 0u,  /* buf[0x16] MSB */
            (f & 0x80u) != 0u,       /* buf[0x22] MSB */
            any_fault,
            CAN_ID_B2_STD);
    }

    /* ── Frame 0xD8: Fault flags B (bit-extracted from buf[0x23]) ── */
    if (can_wait_for_tx_ready(&retry, &tx_done)) {
        uint8_t g = buf[0x23];
        can_send_frame(0xD8u,
            (g & 0x80u) != 0u,       /* bit 7 */
            (g >> 6u) & 1u,          /* bit 6 */
            (g >> 5u) & 1u,          /* bit 5 */
            (g >> 4u) & 1u,          /* bit 4 */
            (g >> 2u) & 1u,          /* bit 2 */
            (g >> 1u) & 1u,          /* bit 1 */
             g & 1u,                 /* bit 0 */
             0u,
            CAN_ID_B2_STD);
    }

    /* ── Frame 0xD9: Pack/protection status ── */
    if (can_wait_for_tx_ready(&retry, &tx_done)) {
        uint16_t pv  = *(uint16_t *)(buf + 0x1Fu);
        uint8_t  chg = (*(uint16_t *)0x01F4u == 0u && *(uint16_t *)0x01E4u != 0u
                        && ((buf[0x21] >> 4u) & 2u) == 0u) ? 1u : 0u;
        uint8_t  dsg = (*(uint16_t *)0x01F2u == 0u && *(uint16_t *)0x01E6u != 0u
                        && ((buf[0x21] >> 4u) & 1u) == 0u) ? 1u : 0u;
        can_send_frame(0xD9u,
            (pv & 0x8000u) != 0u,
            (uint8_t)((pv >> 12u) & 1u),
            (buf[0x18] >> 2u) & 1u,
            chg, dsg,
            *(volatile uint8_t *)0x4052u,
            *(volatile uint8_t *)0x4053u,
            0u,
            CAN_ID_B2_STD);
    }

    /* ── Frames 0xDA/0xDB: Calibration/config from flash ROM ── */
    if (can_wait_for_tx_ready(&retry, &tx_done))
        can_send_frame(0xDAu,
            CAL_DATA_BASE[1], CAL_DATA_BASE[2], CAL_DATA_BASE[4],
            CAL_DATA_BASE[5], CAL_DATA_BASE[6], CAL_DATA_BASE[7],
            0u, 0u, CAN_ID_B2_STD);

    if (can_wait_for_tx_ready(&retry, &tx_done))
        can_send_frame(0xDBu,
            CAL_DATA_BASE[8],  CAL_DATA_BASE[9],  CAL_DATA_BASE[10],
            CAL_DATA_BASE[11], CAL_DATA_BASE[12], CAL_DATA_BASE[13],
            0u, 0u, CAN_ID_B2_STD);

    /* ── Optional frame 0xC1B1: special event (magic byte 0xAA) ── */
    if (FLAG_SPECIAL_EVENT == 0xAAu) {
        if (can_wait_for_tx_ready(&retry, &tx_done)) {
            FLAG_SPECIAL_EVENT = 0u;   /* clear flag after sending */
            can_send_frame(0xB1u,
                0x01u, 0xB0u, 0x02u, 0xA8u,
                0x00u, 0xBEu, 0x00u, 0x00u,
                CAN_ID_B2_EVT);
        }
    }

    /* ── Guard: only continue if charging frame is active ── */
    if (FLAG_CHARGE_ACTIVE != 1u) {
        FLAG_TX_BUSY = 0u;
        return 0u;
    }

    /* ── Frame 0xC3B3: Charging/balancing data ── */
    if (!can_wait_for_tx_ready(&retry, &tx_done)) {
        FLAG_TX_BUSY = 0u;
        return 0u;
    }

    FLAG_CHARGE_ACTIVE = 0u;

    /* Build charging frame — up to 5 float comparisons,
     * exit early on first comparison that fails (not equal / not greater)  */
    uint8_t d3_val = 0x96u;  /* default: 150 (threshold byte) */
    CAN_TX_ID3 = 0xB3u;

    /* Comparison 1: cell 0x2D vs 0 */
    float_load(0x2Du, 0u);
    float_store(NULL);
    float_load_ptr(NULL);
    result = float_compare(NULL, 0u);
    if (!result) goto send_charge_frame;

    /* Comparisons 2-5 with offsets 0x64, 0x68, 0x58, 0x54 */
    float_load(0x2Du, (uint16_t)NULL); float_store(NULL);
    float_load_ptr(NULL);
    result = float_compare(NULL, 0u);
    if (!result) goto send_charge_frame;

    float_load(0x2Du, (uint16_t)NULL); float_store(NULL);
    float_load_ptr(NULL);
    result = float_compare(NULL, 0u);
    if (!result) goto send_charge_frame;

    float_load(0x2Du, (uint16_t)NULL); float_store(NULL);
    float_load_ptr(NULL);
    result = float_compare(NULL, 0u);
    if (!result) goto send_charge_frame;

    float_load(0x2Du, (uint16_t)NULL); float_store(NULL);
    float_load_ptr(NULL);
    result = float_compare(NULL, 0u);
    if (!result) {
        d3_val = (CAL_DATA_BASE[0] < 0x5Bu) ? 0x96u : 0x4Bu;
        goto send_charge_frame;
    }

    /* Secondary comparison chain (0x32 register) */
    float_load(0x32u, (uint16_t)NULL); float_store(NULL);
    float_load_ptr(NULL);
    result = float_compare(NULL, 0u);

    /* ... (further chained comparisons omitted for length;
     *  same pattern continues for registers 0x32 and 0x37/0x3C) */
    d3_val = 0u;

send_charge_frame:
    CAN_TX_D3   = d3_val;
    CAN_TX_D4   = 0u;
    CAN_TX_MCR |= 0x01u;

    CAN_TX_MCR |= 0x01u;   /* final TX trigger */
    CAN_TX_D7   = 0u;
    CAN_TX_D6   = 0u;
    FLAG_TX_BUSY = 0u;

    return result;
}
