/*
 * bms_send_command_0x66.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009055
 * Original name    : FUN_009055
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  The simplest of the group. Directly calls FUN_0096E6 and returns.
 *  No stack manipulation, no result.
 *
 *  FUN_0096E6 disassembly (confirmed):
 *    pushw X             ; save caller's X (param_1)
 *    ld    A, #0x66      ; load fixed command byte 0x66
 *    callf 0x009415      ; send_command(A=0x66)
 *    popw  X             ; restore X
 *    retf
 *
 *  FUN_009415 is the low-level command dispatcher — called with
 *  a single byte in A, it sends that byte as a command or register
 *  write to the AFE (BQ76952) over the peripheral bus (I2C/SPI).
 *
 *  0x66 in the BQ76952 register map corresponds to a subcommand or
 *  direct command byte. Check your BQ76952 datasheet / SLUUBY4 TRM
 *  for exact mapping (likely a "reset" or "update status" command).
 *
 *  Call site (0x00BADF):
 *    ldw  X, #0x0254     ; load a RAM address (passed as param_1)
 *    callf 0x009055      ; FUN_0096E6 saves/restores X but ignores it
 *    ...
 *    ld   A, 0x024D      ; then polls 0x024D waiting for it to become 1
 *    cp   A, #0x01       ; → confirms this triggers a background operation
 *    jrne .-7            ; that sets a flag at 0x024D when done
 *
 *  The polling loop after the call strongly suggests 0x66 initiates
 *  an asynchronous AFE measurement conversion, and 0x024D is a
 *  "conversion complete" flag set by the interrupt/callback.
 */

#include <stdint.h>

/* Command byte sent to the AFE (BQ76952).                              */
/* 0x66: suspected "start measurement conversion" or "update readings"  */
/* Verify against BQ76952 SLUUBY4 Table 4-1 direct command list.       */
#define AFE_CMD_START_CONVERSION    0x66u

/* RAM flag polled by caller after this call (0x024D = conv. complete) */
#define REG_CONVERSION_DONE   (*(volatile uint8_t *)0x024Du)

/* Forward declaration: low-level AFE command dispatcher (FUN_009415). */
extern void afe_send_command(uint8_t cmd_byte);

/*
 * send_afe_command_0x66 (FUN_0096E6 wrapper)
 *
 * Sends command byte 0x66 to the AFE peripheral bus.
 * Caller is expected to poll REG_CONVERSION_DONE (0x024D)
 * for completion before reading results.
 *
 * param_1 (ptr) — passed through from call site but not used
 *                 internally by this function or FUN_0096E6.
 */
static void send_afe_command_0x66(void)
{{
    afe_send_command(AFE_CMD_START_CONVERSION);
}}

/*
 * trigger_afe_conversion  (FUN_009055 — the actual exported function)
 *
 * Wrapper that calls send_afe_command_0x66 and returns.
 * Original Ghidra: simply callf 0x0096E6; retf.
 *
 * Typical usage pattern at call site:
 *
 *   trigger_afe_conversion(some_ptr);
 *   while (REG_CONVERSION_DONE != 1) {{}}   // poll until done
 *   REG_CONVERSION_DONE = 0;               // clear flag
 */
void trigger_afe_conversion(void *param_1)
{{
    (void)param_1;          /* param_1 preserved/restored but not used */
    send_afe_command_0x66();
}}
