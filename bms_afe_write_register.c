/*
 * bms_afe_write_register.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009415
 * Original name    : FUN_009415
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  WRITE counterpart to afe_read_register (FUN_009288).
 *  Instead of reading bytes back from the AFE, this function
 *  WRITES a sequence of bytes out (up to 2 bytes).
 *
 *  Key differences from FUN_009288:
 *    - Uses ROM constants at 0x8092/0x8093 (not 0x8090/0x8091)
 *      → different I2C slave address or sub-address for writes
 *    - NO read phase: sends write-address, then loops writing
 *      data bytes until byte_count == 0
 *    - param_1 is a pointer to a struct: {addr_byte, data_byte[2]}
 *      confirmed from: ld A,(0x01,X) [addr byte],
 *                      ldw X,(X) then rrwa×2 [data 16-bit word]
 *    - Ends with single 300-tick delay (not 1000+300)
 *    - Called by: FUN_0096E6 (trigger_afe_conversion, cmd 0x66)
 *
 *  Confirmed disassembly phases:
 *    Setup:  byte_count=2; reg_addr=ROM[0x8092]; sub_addr=ROM[0x8093]
 *            Override reg_addr from param_1->addr_byte
 *            Override sub_addr from param_1->data >> 14 (2-bit shift)
 *    Phase 1: bus_set_rw(1); while(!bus_check_ack(0x301)){}
 *    Phase 2: bus_write_addr(0x1000); while(!bus_check_ack(0x782)){}
 *    Phase 3: bus_write_data(param_1->first_byte)
 *             while(!bus_check_ack(0x784)){}
 *    Phase 4: loop byte_count times:
 *               pick next byte from param_1->data
 *               bus_write_data(byte); while(!bus_check_ack(0x784)){}
 *               byte_count--
 *    Phase 5: bus_set_cs(1); delay(300, 0)
 */

#include <stdint.h>
#include <stdbool.h>

/* AFE write-channel slave address (different from read channel) */
#define AFE_WRITE_ADDR_HI   (*(const volatile uint8_t *)0x8092)
#define AFE_WRITE_ADDR_LO   (*(const volatile uint8_t *)0x8093)

extern void    bus_set_cs      (uint8_t level);
extern void    bus_set_rw      (uint8_t direction);
extern uint8_t bus_check_ack   (uint16_t reg);
extern void    bus_write_addr  (uint16_t addr);
extern void    bus_write_data  (uint8_t data);
extern void    delay_us        (uint16_t ticks, uint16_t pad);

#define BUS_ACK_REG         0x0301u
#define BUS_ADDR_WRITE_CMD  0x1000u
#define BUS_TXRDY_REG       0x0782u
#define BUS_TXDONE_REG      0x0784u
#define WRITE_BYTE_COUNT    2u

/* AFE write descriptor — layout confirmed from disassembly:
 *   byte[0]  : sub-address / register offset byte
 *   word[0]  : 16-bit data value to write (2 bytes)             */
typedef struct {
    uint8_t  addr_byte;   /* register sub-address                */
    uint16_t data_word;   /* data payload (written MSB-first)    */
} afe_write_desc_t;

/*
 * afe_write_register
 *
 * Writes a 2-byte value to the AFE over the write bus channel.
 * param_1 points to an afe_write_desc_t — a register address byte
 * followed by the 16-bit data to write.
 *
 * Called by: send_afe_command_0x66 (FUN_0096E6) with cmd=0x66,
 *            and other configuration write paths.
 */
void afe_write_register(const afe_write_desc_t *param_1)
{
    uint8_t byte_count = WRITE_BYTE_COUNT;
    uint8_t reg_hi     = AFE_WRITE_ADDR_HI;            /* ROM 0x8092 */
    uint8_t reg_lo     = AFE_WRITE_ADDR_LO;            /* ROM 0x8093 */

    /* Override with values from the write descriptor */
    reg_hi = param_1->addr_byte;
    reg_lo = (uint8_t)(param_1->data_word >> 14);      /* 2-bit shift confirmed */

    /* Phase 1: set write direction, wait for ACK */
    bus_set_rw(1u);
    while (!bus_check_ack(BUS_ACK_REG)) {}

    /* Phase 2: send write-address command, wait for tx ready */
    bus_write_addr(BUS_ADDR_WRITE_CMD);
    while (!bus_check_ack(BUS_TXRDY_REG)) {}

    /* Phase 3: send first data byte, wait for done */
    bus_write_data(reg_hi);
    while (!bus_check_ack(BUS_TXDONE_REG)) {}

    /* Phase 4: send remaining bytes from data_word */
    while (byte_count != 0u)
    {
        uint8_t next_byte;
        uint8_t offset = (uint8_t)(WRITE_BYTE_COUNT - byte_count);

        /* Extract byte at offset from data_word (MSB first) */
        next_byte = (uint8_t)((param_1->data_word >> (8u * (1u - offset))) & 0xFFu);

        bus_write_data(next_byte);
        while (!bus_check_ack(BUS_TXDONE_REG)) {}

        byte_count--;
    }

    /* Phase 5: de-assert CS, 300-tick delay */
    bus_set_cs(1u);
    delay_us(300u, 0u);
}
