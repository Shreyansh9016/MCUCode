#include <stdint.h>

extern void         table_segment_begin(uint8_t *anchor);      /* was FUN_009705 */
extern mcu_float3_t int_to_float3(uint16_t raw);                /* was FUN_00f893 */
extern void         float_op_const(uint16_t const_addr);        /* was FUN_00f4a5 */
extern void         store_table_entry(void *dest, ...);         /* was FUN_00f817 */

#define BIAS_CONST_ADDR   0x8080   /* used only for entry 0 - anchor/offset? */
#define SCALE_CONST_ADDR  0x808c   /* shared with FUN_008c33 - same curve family */

void build_calibration_table_tail(void)   /* was FUN_008edf */
{
    uint16_t raw = 0xea;
    uint8_t  step = 6;   /* alternates 6/7 - fractional step accumulation */

    table_segment_begin(/* local anchor */ 0);

    /* Entry 0 - special case, uses BIAS constant not SCALE constant */
    mcu_float3_t v0 = int_to_float3(raw);
    float_op_const(BIAS_CONST_ADDR);
    store_table_entry(/* dest 0 */ 0);
    raw += step;           /* -> 0xf0, step becomes 7 next */

    /* Entries 1-3 - regular scale constant, same as FUN_008c33 */
    for (int i = 1; i < 4; i++) {
        mcu_float3_t v = int_to_float3(raw);
        float_op_const(SCALE_CONST_ADDR);
        store_table_entry(/* dest i*4 */ 0);
        raw += step;
        step = (step == 6) ? 7 : 6;   /* alternation - confirm against real deltas */
    }
}