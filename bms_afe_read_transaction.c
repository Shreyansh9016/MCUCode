/*
 * bms_afe_read_transaction.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009288
 * Original name    : FUN_009288
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  This is the CORE AFE READ TRANSACTION function — the most
 *  complex in the batch. It is called 34+ times throughout the
 *  firmware. Each call performs one complete I2C/SPI read cycle
 *  from the BQ76952 AFE, reading a specified number of bytes and
 *  storing the result in a caller-supplied buffer.
 *
 *  Confirmed stack layout (from disassembly of 0x009288):
 *    SP+0x05: byte_count (initialised to 2)
 *    SP+0x03: reg_addr_hi  (from DAT_008090)
 *    SP+0x04: reg_addr_lo  (from DAT_008091)
 *    SP+0x01: result_ptr_hi
 *    SP+0x02: result_ptr_lo
 *    SP+0x06: data_byte (used during multi-byte read loop)
 *
 *  DAT_008090 / DAT_008091 (flash ROM at 0x8090/0x8091):
 *    These are read-only constants — the I2C slave address or
 *    SPI register base address of the BQ76952. Confirmed: these
 *    addresses are in your string table region (read-only flash).
 *
 *  ── Phase 1: Bus idle wait (confirmed from disassembly) ──
 *
 *  set_cs_low(0)          ; FUN_009DE0('\0') — assert chip-select LOW
 *  do {
 *    check_status(0x0302) ; FUN_009EEE(0x302) — poll bus-busy flag
 *  } while (status != 0);  ; jrne → loop while non-zero (busy)
 *
 *  ── Phase 2: Issue read command ──
 *
 *  set_rw_bit(1)          ; FUN_009DD2('\x01') — set read direction
 *  do {
 *    check_ack(0x0301)    ; FUN_009E6A(0x301) — wait for ACK
 *  } while (!ack);         ; jreq → loop while zero (no ack)
 *
 *  ── Phase 3: Send register address ──
 *
 *  write_reg_addr(0x1000) ; FUN_009E56(0x1000) — send reg address
 *  do {
 *    check_ack(0x0782)    ; FUN_009E6A(0x782) — wait for addr ACK
 *  } while (!ack);
 *
 *  ── Phase 4: Send data byte ──
 *
 *  write_data(data_byte)  ; FUN_009E66(SP+0x06) — send payload byte
 *  do {
 *    check_ack(0x0784)    ; FUN_009E6A(0x784) — wait for data ACK
 *  } while (!ack);
 *
 *  ── Phase 5: Second bus phase (repeated-start / read phase) ──
 *
 *  set_cs_high(1)         ; FUN_009DE0('\x01')
 *  do {
 *    check_status(0x0302) ; FUN_009EEE — wait for bus idle again
 *  } while (status != 0);
 *
 *  set_rw_bit(1)          ; FUN_009DD2('\x01')
 *  do {
 *    check_ack(0x0301)    ; FUN_009E6A — wait for read ACK
 *  } while (!ack);
 *
 *  send_read_cmd(0x1001)  ; FUN_009E56(0x1001) — issue read request
 *  do {
 *    check_ack(0x0302)    ; FUN_009E6A — wait for read ready
 *  } while (!ack);
 *
 *  ── Phase 6: Receive bytes into buffer ──
 *
 *  do {
 *    if (check_ack(0x0340)) {     ; data available?
 *      *buf_ptr = read_byte()     ; FUN_009E52 → read one byte
 *      byte_count--               ; dec counter
 *    }
 *  } while (byte_count != 0);
 *
 *  ── Phase 7: Finalise ──
 *
 *  send_stop(0)           ; FUN_009E0A('\0') — release bus
 *  set_cs_high(1)         ; FUN_009DE0('\x01')
 *
 *  Store 16-bit combined result (reg_addr_hi:reg_addr_lo) into
 *  the caller-supplied output pointer (param_2).
 *
 *  Delay(1000, 0)         ; FUN_0097F6(1000, 0) — post-read delay
 *  send_complete(1)       ; FUN_009E0A('\x01') — signal done
 *
 *  This is a standard I2C register-read transaction sequence
 *  typical of BQ76952 communication: assert, address, data,
 *  repeated-start, read N bytes, stop.
 */

#include <stdint.h>
#include <stdbool.h>

/* ── AFE slave address constants (from read-only flash) ── */
#define AFE_REG_ADDR_HI   (*(const volatile uint8_t *)0x8090)
#define AFE_REG_ADDR_LO   (*(const volatile uint8_t *)0x8091)

/* ── I2C/SPI peripheral control functions ── */
extern void     bus_set_cs       (uint8_t level);      /* FUN_009DE0 */
extern void     bus_set_rw       (uint8_t direction);  /* FUN_009DD2 */
extern uint8_t  bus_check_status (uint16_t reg);       /* FUN_009EEE */
extern uint8_t  bus_check_ack    (uint16_t reg);       /* FUN_009E6A */
extern void     bus_write_addr   (uint16_t addr);      /* FUN_009E56 */
extern void     bus_write_data   (uint8_t data);       /* FUN_009E66 */
extern uint8_t  bus_read_byte    (void);               /* FUN_009E52 */
extern void     bus_send_stop    (uint8_t level);      /* FUN_009E0A */
extern void     delay_us         (uint16_t ticks,      /* FUN_0097F6 */
                                  uint16_t pad);

/* ── Bus register/command constants (confirmed from disassembly) ── */
#define BUS_STATUS_REG       0x0302u   /* bus busy / status flag        */
#define BUS_ACK_REG          0x0301u   /* ACK / NACK flag               */
#define BUS_ADDR_WRITE_CMD   0x1000u   /* issue write-address command   */
#define BUS_ADDR_READ_CMD    0x1001u   /* issue read-address command    */
#define BUS_TXRDY_REG        0x0782u   /* transmit ready flag           */
#define BUS_TXDONE_REG       0x0784u   /* transmit done flag            */
#define BUS_RXRDY_REG        0x0302u   /* receive ready flag            */
#define BUS_RXDATA_REG       0x0340u   /* receive data available flag   */

#define TX_POST_DELAY_TICKS  1000u     /* post-transaction delay        */
#define DEFAULT_BYTE_COUNT   2u        /* default bytes per transaction  */

/*
 * afe_read_register
 *
 * Performs a complete I2C/SPI read transaction to the BQ76952 AFE:
 *   1. Assert bus, wait for idle
 *   2. Write-phase: send register address + data byte
 *   3. Read-phase:  repeated-start, receive bytes into buffer
 *   4. Stop, delay, signal completion
 *
 * param data_byte : command/sub-address byte to send in write phase
 * param out_ptr   : pointer to buffer where received bytes are stored
 *
 * Called 34+ times — once per AFE register read in the measurement
 * cycle (cell voltages, temperatures, pack current, status flags).
 */
void afe_read_register(uint8_t data_byte, uint8_t *out_ptr)
{
    /* Stack locals matching confirmed disassembly layout */
    uint8_t  reg_addr_hi  = AFE_REG_ADDR_HI;   /* from flash 0x8090 */
    uint8_t  reg_addr_lo  = AFE_REG_ADDR_LO;   /* from flash 0x8091 */
    uint8_t  byte_count   = DEFAULT_BYTE_COUNT;
    uint8_t *buf_ptr      = out_ptr;

    /* ── Phase 1: Assert CS, wait for bus idle ── */
    bus_set_cs(0u);
    while (bus_check_status(BUS_STATUS_REG) != 0u) {}

    /* ── Phase 2: Set read direction, wait for ACK ── */
    bus_set_rw(1u);
    while (!bus_check_ack(BUS_ACK_REG)) {}

    /* ── Phase 3: Send register address, wait for ACK ── */
    bus_write_addr(BUS_ADDR_WRITE_CMD);
    while (!bus_check_ack(BUS_TXRDY_REG)) {}

    /* ── Phase 4: Send data/command byte, wait for done ── */
    bus_write_data(data_byte);
    while (!bus_check_ack(BUS_TXDONE_REG)) {}

    /* ── Phase 5: Repeated-start — second bus phase ── */
    bus_set_cs(1u);
    while (bus_check_status(BUS_STATUS_REG) != 0u) {}

    bus_set_rw(1u);
    while (!bus_check_ack(BUS_ACK_REG)) {}

    /* ── Phase 6: Issue read command, wait for ready ── */
    bus_write_addr(BUS_ADDR_READ_CMD);
    while (!bus_check_ack(BUS_RXRDY_REG)) {}

    /* ── Phase 7: Receive bytes into buffer ── */
    do
    {
        if (bus_check_ack(BUS_RXDATA_REG))
        {
            *buf_ptr = bus_read_byte();
            buf_ptr++;
            byte_count--;
        }
    }
    while (byte_count != 0u);

    /* ── Phase 8: Stop transaction, de-assert CS ── */
    bus_send_stop(0u);
    bus_set_cs(1u);

    /* ── Phase 9: Store combined 16-bit result ── */
    /* Matches: ldw Y,(0x0A,SP); ldw (Y),X → *param_2 = (hi<<8|lo) */
    if (out_ptr != (uint8_t *)0u)
    {
        *((uint16_t *)out_ptr) = (uint16_t)((reg_addr_hi << 8) | reg_addr_lo);
    }

    /* ── Phase 10: Post-transaction delay and completion signal ── */
    /* Matches: ldw X,#0x03E8; push X; ldw X,#0; push X;           */
    /*          callf 0x0097F6  (delay 1000 ticks)                  */
    delay_us(TX_POST_DELAY_TICKS, 0u);

    /* Matches: ld A,#0x01; callf 0x009E0A */
    bus_send_stop(1u);
}
