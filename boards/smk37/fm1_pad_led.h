/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
/* WS2812 transport on SPI1 group B, PC10 data. PC9 remains an analog input. */
#pragma once
#include "../../chip/fm1_gpio.h"
#include "fm1_lcd_hw.h"
static void smk_pad_encode(uint8_t packet[420], const uint8_t rgb[16][3])
{
    static const uint8_t chain[16] = {0,15,14,13,12,11,10,9,1,2,3,4,5,6,7,8};
    for (unsigned i = 0; i < 420; i++) packet[i] = 0;
    for (unsigned p = 0; p < 16; p++)
        for (unsigned c = 0; c < 3; c++) {
            uint8_t v = rgb[p][c == 0 ? 1 : c == 1 ? 0 : 2];
            for (unsigned b = 0; b < 8; b++) packet[36 + chain[p] * 24 + c * 8 + b] = v & (0x80u >> b) ? 0xF0 : 0xC0;
        }
}
static int smk_pad_flush(const uint8_t rgb[16][3])
{
    uint8_t packet[420] __attribute__((aligned(4)));
    smk_pad_encode(packet, rgb);
    FM1_LCD_PC_OUT &= ~(1u << 10);
    FM1_PR(FM1_PC, FM1_PU) &= ~(1u << 10);
    FM1_PR(FM1_PC, FM1_PD) &= ~(1u << 10);
    FM1_PR(FM1_PC, FM1_DIE) |= 1u << 10;
    FM1_LCD_PC_DIR &= ~(1u << 10);
    FM1_LCD_IOMAP_CON1 |= 0x10u;
    fm1_lcd_baud(8);
    FM1_LCD_SPI_CON |= 0x4000u;
    FM1_LCD_SPI_ADR = (uint32_t)(uintptr_t)packet;
    FM1_LCD_SPI_CNT = sizeof packet;
    int rc = fm1_lcd_wait();
    if (rc) fm1_lcd_spi_off();
    FM1_LCD_IOMAP_CON1 &= ~0x10u;
    fm1_lcd_baud(4);
    if (rc) fm1_lcd_hw_init();
    return rc;
}
