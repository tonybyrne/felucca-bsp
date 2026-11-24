/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Copyright (C) 2026 Tony Byrne */
/* SMK transport using the FM-1 API: SPI1 group A, PB6 clock, PB7 data, PB5 D/C, PC8 CS.
 * No backlight GPIO; PC9 stays analog. Panel settings are from fm1-to-smk37/docs/01-hardware.md. */
#pragma once
#include <stdint.h>
#include "../../chip/fm1_cc.h"
#ifndef FM1_LCD_REG
#define FM1_LCD_REG(addr) (*(volatile uint32_t *)(addr))
#endif
#define FM1_LCD_PB_OUT FM1_LCD_REG(0x50040u)
#define FM1_LCD_PB_DIR FM1_LCD_REG(0x50048u)
#define FM1_LCD_PC_OUT FM1_LCD_REG(0x50080u)
#define FM1_LCD_PC_DIR FM1_LCD_REG(0x50088u)
#define FM1_LCD_IOMAP_CON1 FM1_LCD_REG(0x51020u)
#define FM1_LCD_SPI_CON FM1_LCD_REG(0x11D00u)
#define FM1_LCD_SPI_BAUD FM1_LCD_REG(0x11D04u)
#define FM1_LCD_SPI_BUF FM1_LCD_REG(0x11D08u)
#define FM1_LCD_SPI_ADR FM1_LCD_REG(0x11D0Cu)
#define FM1_LCD_SPI_CNT FM1_LCD_REG(0x11D10u)
#define FM1_LCD_CS (1u << 8)
#define FM1_LCD_DC (1u << 5)
#define FM1_LCD_CLK (1u << 6)
#define FM1_LCD_DO (1u << 7)
static uint32_t fm1_lcd_timeouts;

FM1_INLINE void fm1_lcd_hw_init(void)
{
    FM1_LCD_PC_OUT |= FM1_LCD_CS;
    FM1_LCD_PC_DIR &= ~FM1_LCD_CS;
    FM1_LCD_PB_OUT &= ~(FM1_LCD_DC | FM1_LCD_CLK | FM1_LCD_DO);
    FM1_LCD_PB_DIR &= ~(FM1_LCD_DC | FM1_LCD_CLK | FM1_LCD_DO);
    FM1_LCD_IOMAP_CON1 &= ~0x10u;
    FM1_LCD_SPI_CON = 0x4021u;
    FM1_LCD_SPI_BAUD = 4u;
}
FM1_INLINE void fm1_lcd_backlight(uint32_t on) { (void)on; } /* Always on in hardware. */
FM1_INLINE void fm1_lcd_baud(uint32_t b) { FM1_LCD_SPI_BAUD = b; }
static int fm1_lcd_wait(void)
{
    uint32_t n;
    int rc = 0;
    for (n = 0; n < 4000000u && !(FM1_LCD_SPI_CON & 0x8000u); n++) ;
    if (n == 4000000u) {
        fm1_lcd_timeouts++;
        rc = -1;
    }
    FM1_LCD_SPI_CON |= 0x4000u;
    return rc;
}
FM1_INLINE void fm1_lcd_spi_off(void) { FM1_LCD_SPI_CON &= ~1u; }
FM1_INLINE void fm1_lcd_deselect(void) { FM1_LCD_PC_OUT |= FM1_LCD_CS; }
FM1_INLINE void fm1_lcd_send_cmd(uint8_t c)
{
    FM1_LCD_PB_OUT &= ~FM1_LCD_DC;
    FM1_LCD_PC_OUT &= ~FM1_LCD_CS;
    FM1_LCD_SPI_CON |= 0x4000u;
    FM1_LCD_SPI_BUF = c;
}
FM1_INLINE void fm1_lcd_send_data(const void *p, uint32_t n)
{
    FM1_LCD_PB_OUT |= FM1_LCD_DC;
    FM1_LCD_PC_OUT &= ~FM1_LCD_CS;
    FM1_LCD_SPI_CON |= 0x4000u;
    FM1_LCD_SPI_ADR = (uint32_t)(uintptr_t)p;
    FM1_LCD_SPI_CNT = n;
}

static void smk_lcd_panel_init(void)
{
    static const uint8_t setup[] = {
        0x36,1,0x00, 0x3A,1,0x05, 0xB2,5,0x0C,0x0C,0,0x33,0x33,
        0xB7,1,0x35, 0xBB,1,0x32, 0xC2,1,0x01, 0xC3,1,0x15, 0xC4,1,0x20,
        0xC6,1,0x0F, 0xD0,2,0xA4,0xA1,
        0xE0,14,0xD0,8,0x0E,9,9,5,0x31,0x33,0x48,0x17,0x14,0x15,0x31,0x34,
        0xE1,14,0xD0,8,0x0E,9,9,5,0x31,0x33,0x48,0x17,0x14,0x15,0x31,0x34,
        0x21,0
    };
    uint8_t data[14] __attribute__((aligned(4)));
    uint32_t at = 0, i;
    fm1_delay_ms(100);
    fm1_lcd_send_cmd(0x11);
    fm1_lcd_wait();
    fm1_lcd_deselect();
    fm1_delay_ms(120);
    while (at < sizeof setup) {
        uint8_t cmd = setup[at++], n = setup[at++];
        fm1_lcd_send_cmd(cmd);
        fm1_lcd_wait();
        fm1_lcd_deselect();
        for (i = 0; i < n; i++) data[i] = setup[at++];
        if (n) {
            fm1_lcd_send_data(data, n);
            fm1_lcd_wait();
            fm1_lcd_deselect();
        }
    }
}
