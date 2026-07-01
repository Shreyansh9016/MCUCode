#include <stdint.h>
#include <string.h>

/* Fixed RAM region starting at 0x0100 - exact layout/names are guesses
   until confirmed against a map file or further cross-referencing. */

#define BUF1_ADDR   ((uint8_t *)0x0100)   /* 0x100-0x13F, 64 bytes */
#define BUF2_ADDR   ((uint8_t *)0x0140)   /* 0x140-0x167, 40 bytes */
#define BUF3_ADDR   ((uint8_t *)0x0168)   /* 0x168-0x177, 16 bytes */

/* Individually-cleared fields, 0x178-0x18D (~18 bytes) */
#define FIELD_178   (*(volatile uint16_t *)0x0178)  /* wide access ("_DAT_") */
#define FIELD_17A   (*(volatile uint8_t  *)0x017A)
#define FIELD_17B   (*(volatile uint8_t  *)0x017B)
#define FIELD_17C   (*(volatile uint8_t  *)0x017C)
#define FIELD_17D   (*(volatile uint8_t  *)0x017D)
#define FIELD_17E   (*(volatile uint8_t  *)0x017E)
#define FIELD_17F   (*(volatile uint8_t  *)0x017F)
#define FIELD_180   (*(volatile uint8_t  *)0x0180)
#define FIELD_181   (*(volatile uint8_t  *)0x0181)
#define FIELD_182   (*(volatile uint8_t  *)0x0182)
#define FIELD_183   (*(volatile uint8_t  *)0x0183)
#define FIELD_184   (*(volatile uint8_t  *)0x0184)
#define FIELD_185   (*(volatile uint8_t  *)0x0185)
#define FIELD_186   (*(volatile uint8_t  *)0x0186)
#define FIELD_187   (*(volatile uint8_t  *)0x0187)
#define FIELD_188   (*(volatile uint16_t *)0x0188)  /* wide access ("_DAT_") */
#define FIELD_18A   (*(volatile uint8_t  *)0x018A)
#define FIELD_18B   (*(volatile uint16_t *)0x018B)  /* wide access ("_DAT_") */
#define FIELD_18D   (*(volatile uint16_t *)0x018D)  /* wide access ("_DAT_") */

void state_ram_clear(void)   /* was FUN_008b3b */
{
    memset(BUF1_ADDR, 0, 0x40);
    memset(BUF2_ADDR, 0, 0x28);
    memset(BUF3_ADDR, 0, 0x10);

    FIELD_178 = 0;
    FIELD_17A = 0;
    FIELD_17D = 0;
    FIELD_17B = 0;
    FIELD_17E = 0;
    FIELD_17C = 0;
    FIELD_17F = 0;
    FIELD_180 = 0;
    FIELD_181 = 0;
    FIELD_182 = 0;
    FIELD_183 = 0;
    FIELD_184 = 0;
    FIELD_185 = 0;
    FIELD_186 = 0;
    FIELD_187 = 0;
    FIELD_188 = 0;
    FIELD_18A = 0;
    FIELD_18B = 0;
    FIELD_18D = 0;
}