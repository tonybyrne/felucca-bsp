/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
/* The loader's LCD transport may only change PB5..7 and PC8, never PB1 or PC9. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t regs[11];
static uint32_t *lcd_reg(uint32_t a)
{
    const uint32_t addr[] = {0x50040u,0x50048u,0x50080u,0x50088u,0x51020u,
        0x11D00u,0x11D04u,0x11D08u,0x11D0Cu,0x11D10u};
    for (unsigned i = 0; i < sizeof addr / sizeof addr[0]; i++)
        if (a == addr[i]) return regs + i;
    abort();
}
static void fm1_delay_ms(uint32_t n) { (void)n; }
#define FM1_LCD_REG(a) (*lcd_reg(a))
#include "../boards/smk37/fm1_lcd_hw.h"
#include "../boards/smk37/fm1_pad_led.h"
int main(void)
{
    for (unsigned i = 0; i < 11; i++) regs[i] = UINT32_MAX;
    fm1_lcd_hw_init();
    assert(regs[0] == (UINT32_MAX & ~0xE0u));
    assert(regs[1] == (UINT32_MAX & ~0xE0u));
    assert(regs[2] == UINT32_MAX && regs[3] == (UINT32_MAX & ~0x100u));
    assert(regs[4] == (UINT32_MAX & ~0x10u));
    assert(regs[5] == 0x4021u && regs[6] == 4u);
    fm1_lcd_send_cmd(0x11);
    assert(regs[7] == 0x11u && !(regs[0] & (1u << 5)) && !(regs[2] & (1u << 8)));
    uint8_t pixel[2] = {0x12, 0x34};
    fm1_lcd_send_data(pixel, sizeof pixel);
    assert((regs[0] & (1u << 5)) && regs[9] == 2u);
    fm1_lcd_deselect();
    assert(regs[2] == UINT32_MAX);
    fm1_lcd_backlight(0);
    assert((regs[1] & (1u << 1)) && (regs[3] & (1u << 9)));
    uint8_t rgb[16][3] = {{1,2,3}}, packet[420];
    rgb[8][1] = 128;
    rgb[1][0] = 128;
    smk_pad_encode(packet, rgb);
    for (unsigned i = 0; i < 36; i++) assert(packet[i] == 0);
    assert(packet[36+6] == 0xF0 && packet[36+7] == 0xC0);
    assert(packet[36+8+7] == 0xF0 && packet[36+8] == 0xC0);
    assert(packet[36+16+6] == 0xF0 && packet[36+16+7] == 0xF0);
    assert(packet[36+24] == 0xF0 && packet[36+15*24+8] == 0xF0);
    puts("SMK LCD pin checks passed");
    return 0;
}
