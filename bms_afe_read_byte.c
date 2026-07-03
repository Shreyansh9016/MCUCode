/*
 * bms_afe_read_byte.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x00935E
 * Original name    : FUN_00935e
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Simplified single-byte variant of afe_read_register (FUN_009288).
 *  Same I2C/SPI phase sequence but:
 *    - Reads exactly ONE byte (no byte_count loop)
 *    - Returns that byte directly as the function return value
 *    - Ends with TWO delays: 1000 ticks + 300 ticks
 *    - Does NOT read AFE slave address from flash (0x8090/0x8091);
 *      uses push A / push A stack frame instead
 *
 *  Used by FUN_00962F, FUN_009644, and similar pair-readers
 *  that need one byte per BQ76952 register command.
 *
 *  Confirmed disassembly phases:
 *    Phase 1: bus_set_cs(0); while(bus_check_status(0x302)!=0){}
 *    Phase 2: bus_set_rw(1); while(!bus_check_ack(0x301)){}
 *    Phase 3: bus_write_addr(0x1000); while(!bus_check_ack(0x782)){}
 *    Phase 4: bus_write_data(param_1); while(!bus_check_ack(0x784)){}
 *    Phase 5: bus_set_cs(1); while(bus_check_status(0x302)!=0){}
 *    Phase 6: bus_set_rw(1); while(!bus_check_ack(0x301)){}
 *    Phase 7: bus_write_addr(0x1001); while(!bus_check_ack(0x302)){}
 *    Phase 8: while(!bus_check_ack(0x340)){} → read ONE byte
 *    Phase 9: bus_send_stop(0); bus_set_cs(1)
 *    Phase 10: *out_ptr = byte_read
 *    Phase 11: delay(1000, 0); bus_send_stop(1)
 *    Phase 12: delay(300, 0)
 *    Return:   the byte read
 */

#include <stdint.h>

extern void    bus_set_cs      (uint8_t level);
extern void    bus_set_rw      (uint8_t direction);
extern uint8_t bus_check_status(uint16_t reg);
extern uint8_t bus_check_ack   (uint16_t reg);
extern void    bus_write_addr  (uint16_t addr);
extern void    bus_write_data  (uint8_t data);
extern uint8_t bus_read_byte   (void);
extern void    bus_send_stop   (uint8_t level);
extern void    delay_us        (uint16_t ticks, uint16_t pad);

#define BUS_STATUS_REG      0x0302u
#define BUS_ACK_REG         0x0301u
#define BUS_ADDR_WRITE_CMD  0x1000u
#define BUS_ADDR_READ_CMD   0x1001u
#define BUS_TXRDY_REG       0x0782u
#define BUS_TXDONE_REG      0x0784u
#define BUS_RXDATA_REG      0x0340u

/*
 * afe_read_single_byte
 *
 * Sends command byte cmd_byte to the AFE and reads back exactly
 * one byte. Ends with a 1000+300 tick double-delay.
 *
 * Returns the byte received from the AFE.
 *
 * Called by: read_cell_pair_lo_hi(), read_cell_pair_cd(), etc.
 */
uint8_t afe_read_single_byte(uint8_t cmd_byte)
{
    uint8_t result;

    /* Phase 1: assert CS, wait for bus idle */
    bus_set_cs(0u);
    while (bus_check_status(BUS_STATUS_REG) != 0u) {}

    /* Phase 2: set read direction, wait for ACK */
    bus_set_rw(1u);
    while (!bus_check_ack(BUS_ACK_REG)) {}

    /* Phase 3: send write-address command, wait for transmit ready */
    bus_write_addr(BUS_ADDR_WRITE_CMD);
    while (!bus_check_ack(BUS_TXRDY_REG)) {}

    /* Phase 4: send command byte, wait for done */
    bus_write_data(cmd_byte);
    while (!bus_check_ack(BUS_TXDONE_REG)) {}

    /* Phase 5: repeated-start, wait for bus idle */
    bus_set_cs(1u);
    while (bus_check_status(BUS_STATUS_REG) != 0u) {}

    /* Phase 6: set read direction, wait for ACK */
    bus_set_rw(1u);
    while (!bus_check_ack(BUS_ACK_REG)) {}

    /* Phase 7: issue read command, wait for data ready */
    bus_write_addr(BUS_ADDR_READ_CMD);
    while (!bus_check_ack(BUS_STATUS_REG)) {}

    /* Phase 8: wait for data available, read ONE byte */
    while (!bus_check_ack(BUS_RXDATA_REG)) {}
    result = bus_read_byte();

    /* Phase 9: stop, de-assert CS */
    bus_send_stop(0u);
    bus_set_cs(1u);

    /* Phase 11: post-read delay 1000 ticks */
    delay_us(1000u, 0u);
    bus_send_stop(1u);

    /* Phase 12: second delay 300 ticks */
    delay_us(300u, 0u);

    return result;
}
