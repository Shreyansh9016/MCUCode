/*
 * bms_send_cmd_0x66_write.c
 *
 * Original address : 0x0096E6  /  FUN_0096e6
 *
 * Confirmed disassembly:
 *   pushw X
 *   ld A, #0x66        ; command byte 0x66
 *   callf 0x009415     ; afe_write_register (WRITE path, not read)
 *   popw X; retf
 *
 * Unlike all previous register readers, this uses FUN_009415
 * (afe_write_register, the WRITE bus path at 0x8092/0x8093).
 * Command 0x66 in the BQ76952 is a SUBCOMMAND trigger:
 *   Writing 0x66 to address 0x3E/0x3F triggers a subcommand.
 *   0x0066 = RESET subcommand (soft reset the AFE).
 *
 * This is called from trigger_afe_conversion (FUN_009055), which
 * is followed by polling RAM[0x024D] for the completion flag —
 * consistent with a reset or start-of-conversion trigger that
 * takes time to complete in the background.
 */

#include <stdint.h>

#define BQ76952_SUBCMD_TRIGGER_CMD  0x66u

extern void afe_write_register(const void *desc);  /* FUN_009415 */

void send_subcommand_trigger(void *param_1)
{
    /* param_1 is passed through to afe_write_register as the
     * descriptor pointer — the actual command byte 0x66 is
     * hardcoded in this wrapper (ld A, #0x66 before callf).    */
    (void)param_1;
    afe_write_register((const void *)(uintptr_t)BQ76952_SUBCMD_TRIGGER_CMD);
}
