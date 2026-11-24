/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * Copyright (C) 2026 Tony Byrne */
/* Adapted from Felucca's SMK input layer. Factory trim is read-only; the scan owns the ADC. */
#pragma once
#include <stdint.h>
#include "fm1_time.h"
#include "fm1_gpio.h"
#ifndef SMK_REG
#define SMK_REG(a) (*(volatile uint32_t *)(a))
#endif
#define SMK_SPI_CON SMK_REG(0x11E00u)
#define SMK_SPI_BAUD SMK_REG(0x11E04u)
#define SMK_SPI_ADR SMK_REG(0x11E0Cu)
#define SMK_SPI_CNT SMK_REG(0x11E10u)
#define SMK_IOMAP SMK_REG(0x51020u)
#define SMK_USBIO SMK_REG(0x16A0Cu)
#define SMK_ADC_CON SMK_REG(0x13100u)
#define SMK_ADC_RES SMK_REG(0x13104u)
#define SMK_WLA SMK_REG(0x11900u)
#define FM1_NCOL 16u
#define FM1_NBTN 18u
#define FM1_NENC 8u
#define FM1_NKEY 37u
#define SMK_KEY_F3 5u
#define FM1_OCT_BOTH ((1u << 4) | (1u << 5))
static const uint8_t SMK_KEYC[37][2] = {
    {0x10,0x52},{0x20,0x72},{0x30,0x92},{0x40,0xb2},{0x50,0xd2},{0x60,0xf2},{0x70,0xe2},
    {0x80,0xc2},{0x90,0xa2},{0xa0,0x82},{0xc0,0x62},{0xd0,0x42},{0xe0,0x22},{0xf0,0x12},
    {0xb0,0x32},{0x11,0x33},{0x21,0x43},{0x31,0x53},{0x41,0x63},{0x51,0x73},{0x61,0x83},
    {0x71,0x93},{0x81,0xa3},{0x91,0xb3},{0xa1,0xc3},{0xb1,0xd3},{0xc1,0xe3},{0xd1,0x23},
    {0xe1,0x13},{0xf1,0x14},{0x64,0x24},{0x74,0x34},{0x84,0x44},{0x94,0x54},{0xa4,0xd4},
    {0xb4,0xf4},{0xc4,0xe4}};
static const uint8_t SMK_BTNC[18] = {0x26,0x76,0x96,0x56,0x66,0x36,0x75,0x65,0x25,0x45,0x15,0x86,0x95,0x55,0x85,0x35,0x16,0x46};
static const uint8_t SMK_ENCC[8][2] = {{0x97,0xa7},{0x37,0x47},{0x17,0x27},{0xb7,0xc7},{0x57,0x67},{0x77,0x87},{0xd7,0xe7},{0xf7,0x07}};
static const uint8_t SMK_LEDC[19] = {0x0c,0x12,0,0x10,0x0d,0x1d,2,1,9,8,0x0b,0x0a,0x11,0x19,0x18,0x1a,0x1c,0x1b,0x1e};
static const uint8_t SMK_WHITE[16] = {0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static const uint8_t SMK_PADMUX[8] = {1,2,3,0,4,7,5,6};
static const uint8_t SMK_FADMUX[4] = {1,2,3,0};
static volatile struct {
    uint32_t notes, knotes, pnotes, buttons, pressed, released, notes_pressed, frames;
    uint64_t keys;
    uint8_t raw[16], key_vel[37], ks_stab[37], ks_rel[37], cnt[18];
    uint32_t ks_t1[37];
    uint8_t enc_last[8], enc_hist[8];
    int16_t enc_cnt[8], enc_steps[8];
    uint16_t pad_raw[16], pad_base[16], pad_peak[16];
    uint8_t pad_st[16], pad_low[16];
    uint16_t fad_acc[4], fad_hist[4][2], fad_raw[4], wheel[2], pedal, batt;
    uint8_t fader[4];
} fm1_in;
static uint8_t fm1_led[19];
static uint32_t smk_ledw[2], smk_muxw = 0xFFFFFFC7u;
static uint8_t smk_col, smk_grp, smk_slot, smk_adc_slot = 0xFF;
static const uint8_t *smk_trim;
#define SMK_EXTQ 32u
static volatile uint16_t smk_extq[SMK_EXTQ];
static volatile uint8_t smk_ext_w, smk_ext_r;

static void smk_pin(uint32_t port, uint32_t mask, int mode)
{
    FM1_PR(port, FM1_DIR) |= mask;
    FM1_PR(port, FM1_PU) &= ~mask;
    FM1_PR(port, FM1_PD) &= ~mask;
    if (mode == 2) FM1_PR(port, FM1_DIE) &= ~mask;
    else FM1_PR(port, FM1_DIE) |= mask;
    if (mode == 1) FM1_PR(port, FM1_PU) |= mask;
    if (mode == 0) {
        FM1_PR(port, FM1_OUT) &= ~mask;
        FM1_PR(port, FM1_DIR) &= ~mask;
    }
}
static void smk_common(uint32_t group)
{
    uint32_t v = SMK_USBIO | 0x1000Fu | (1u << 17) | (1u << 18);
    if (group < 2u) v &= ~(1u << (17u + group));
    SMK_USBIO = v;
}
static void smk_shift(uint32_t col)
{
    static uint32_t w;
    w = ~(col < 8u ? 0x01000000u << col : 0x00010000u << (col - 8u));
    w &= smk_ledw[smk_grp] & smk_muxw;
    SMK_SPI_CON |= 0x4000u;
    SMK_SPI_ADR = (uint32_t)(uintptr_t)&w;
    SMK_SPI_CNT = 4;
}
static uint16_t smk_rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static int smk_trim_load(const uint8_t *rec)
{
    static const char mark[8] = "trimdat";
    smk_trim = 0;
    for (unsigned i = 0; i < 8; i++) if (rec[0x224 + i] != (uint8_t)mark[i]) return 0;
    for (unsigned i = 0; i < 53; i++)
        for (unsigned j = 1; j < 5; j++)
            if (smk_rd16(rec + 8 + i * 10 + j * 2) <= smk_rd16(rec + 8 + i * 10 + (j - 1) * 2)) return 0;
    smk_trim = rec;
    return 1;
}
static uint8_t smk_velocity(uint32_t index, uint32_t x)
{
    static const uint16_t defaults[2][5] = {{50,550,720,850,950},{5,640,800,920,980}};
    uint16_t t[5];
    unsigned i;
    for (i = 0; i < 5; i++) t[i] = smk_trim ? smk_rd16(smk_trim + 8 + index * 10 + i * 2) : defaults[index >= 16][i];
    for (i = 0; i < 5 && x > t[i]; i++) ;
    uint32_t v = !i ? 1u : i == 5 ? 1023u : (i - 1u) * 256u + ((x - t[i-1]) << 8) / (t[i] - t[i-1]);
    return (uint8_t)(v >> 3 ? v >> 3 : 1);
}
static void fm1_input_init(void)
{
    smk_pin(FM1_PA, 0x1Bu, 0);
    smk_pin(FM1_PA, (1u<<2)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<9), 1);
    smk_pin(FM1_PH, (1u<<6)|(1u<<8)|(1u<<9), 1);
    smk_pin(FM1_PA, (1u<<8)|(1u<<10), 2);
    smk_pin(FM1_PC, 1u<<9, 2);
    smk_ledw[0] = smk_ledw[1] = 0xFFFFFFFFu;
    smk_common(2);
    SMK_IOMAP = (SMK_IOMAP & ~0x30000u) | 0x20000u;
    SMK_SPI_CON = 0x4021u;
    SMK_SPI_BAUD = 5;
    for (unsigned i = 0; i < 8; i++) fm1_in.enc_last[i] = 0xFF;
    for (unsigned i = 0; i < 16; i++) fm1_in.pad_base[i] = 100;
    smk_col = 0;
    smk_shift(0);
}
static uint32_t smk_closed(uint32_t c) { return (fm1_in.raw[c >> 4] >> (c & 15u)) & 1u; }
static void smk_key_event(uint32_t k, uint32_t vel)
{
    if (vel) fm1_in.key_vel[k] = (uint8_t)vel;
    if (k >= 5 && k < 32) {
        uint32_t b = 1u << (k - 5);
        if (vel) { fm1_in.knotes |= b; fm1_in.notes_pressed |= b; }
        else fm1_in.knotes &= ~b;
        fm1_in.notes = fm1_in.knotes | fm1_in.pnotes;
    } else {
        uint8_t next = (uint8_t)((smk_ext_w + 1u) % SMK_EXTQ);
        if (next != smk_ext_r) { smk_extq[smk_ext_w] = (uint16_t)(k << 8 | vel); smk_ext_w = next; }
    }
    if (vel) fm1_in.keys |= (uint64_t)1u << k;
    else fm1_in.keys &= ~((uint64_t)1u << k);
}
static void smk_frame(void)
{
    uint32_t now = fm1_ticks();
    for (unsigned k = 0; k < 37; k++) {
        uint32_t s = smk_closed(SMK_KEYC[k][0]) | smk_closed(SMK_KEYC[k][1]) << 1, old = fm1_in.ks_stab[k];
        if (s == 2) s = 3;
        if (s == old) { fm1_in.ks_rel[k] = 0; continue; }
        if (!s) {
            if (++fm1_in.ks_rel[k] < 3) continue;
            fm1_in.ks_rel[k] = fm1_in.ks_stab[k] = 0;
            if (old == 3) smk_key_event(k, 0);
            continue;
        }
        fm1_in.ks_rel[k] = 0;
        if (!old) fm1_in.ks_t1[k] = now;
        if (s == 3 && old != 3) {
            uint32_t t = 10u + (now - fm1_in.ks_t1[k]) / (100u * FM1_TICKS_PER_US);
            smk_key_event(k, smk_velocity(16 + k, t < 1023 ? 1023 - t : 1));
        }
        if (s != 1 || old != 3) fm1_in.ks_stab[k] = (uint8_t)s;
    }
    for (unsigned i = 0; i < 18; i++) {
        uint32_t bit = 1u << i, on = !!(fm1_in.buttons & bit);
        if (smk_closed(SMK_BTNC[i]) == on) { fm1_in.cnt[i] = 0; continue; }
        if (++fm1_in.cnt[i] < (on ? 4 : 2)) continue;
        fm1_in.cnt[i] = 0; fm1_in.buttons ^= bit;
        if (on) fm1_in.released |= bit; else fm1_in.pressed |= bit;
    }
    for (unsigned e = 0; e < 8; e++) {
        uint32_t cur = smk_closed(SMK_ENCC[e][0]) | smk_closed(SMK_ENCC[e][1]) << 1;
        if (fm1_in.enc_last[e] == 0xFF) { fm1_in.enc_last[e] = (uint8_t)cur; fm1_in.enc_hist[e] = (uint8_t)(cur * 0x15u); continue; }
        if (cur != fm1_in.enc_last[e]) { fm1_in.enc_last[e] = (uint8_t)cur; continue; }
        if (cur == (fm1_in.enc_hist[e] & 3u)) continue;
        uint32_t h = ((uint32_t)fm1_in.enc_hist[e] << 2 | cur) & 63u;
        fm1_in.enc_hist[e] = (uint8_t)h;
        if (h == 7 || h == 0x1e || h == 0x21 || h == 0x38) fm1_in.enc_cnt[e]--;
        else if (h == 0xb || h == 0x12 || h == 0x2d || h == 0x34) fm1_in.enc_cnt[e]++;
        int steps = fm1_in.enc_cnt[e] / 4;
        fm1_in.enc_steps[e] += steps; fm1_in.enc_cnt[e] -= steps * 4;
    }
    uint32_t w[2] = {0xFFFFu,0xFFFFu};
    for (unsigned i = 0; i < 19; i++) if (fm1_led[i]) w[SMK_LEDC[i] >> 4] &= ~(1u << (SMK_LEDC[i] & 15));
    for (unsigned i = 0; i < 2; i++) smk_ledw[i] = 0xFFFF0038u | w[i];
    smk_grp ^= 1;
    fm1_in.frames++;
}
static void smk_pad_event(uint32_t p, uint32_t vel)
{
    uint32_t n = SMK_WHITE[p], bit = 1u << n;
    if (vel) { fm1_in.key_vel[5+n] = (uint8_t)vel; fm1_in.pnotes |= bit; fm1_in.notes_pressed |= bit; }
    else fm1_in.pnotes &= ~bit;
    fm1_in.notes = fm1_in.knotes | fm1_in.pnotes;
}
static void smk_analog(uint32_t slot, uint32_t raw)
{
    uint32_t a = slot / 3, ch = slot % 3;
    if (ch < 2) {
        uint32_t p = SMK_PADMUX[a] + ch * 8, base = fm1_in.pad_base[p], st = fm1_in.pad_st[p];
        fm1_in.pad_raw[p] = (uint16_t)raw;
        if (!st) {
            if (raw > base + 24) { fm1_in.pad_st[p] = 1; fm1_in.pad_peak[p] = (uint16_t)raw; }
            else if (base > raw) fm1_in.pad_base[p]--;
            else if (base < (smk_trim ? smk_rd16(smk_trim + 8 + p * 10) : 50)) fm1_in.pad_base[p] += 2;
        } else if (st < 0x80) {
            if (raw > fm1_in.pad_peak[p]) fm1_in.pad_peak[p] = (uint16_t)raw;
            if (++fm1_in.pad_st[p] >= 3) {
                fm1_in.pad_st[p] = 0x80; fm1_in.pad_low[p] = 0;
                smk_pad_event(p, smk_velocity(p, fm1_in.pad_peak[p]));
            }
        } else if (raw < base + 12) {
            if (++fm1_in.pad_low[p] >= 2) { fm1_in.pad_st[p] = 0; smk_pad_event(p, 0); }
        } else fm1_in.pad_low[p] = 0;
    } else if (a < 4) {
        uint32_t f = SMK_FADMUX[a], h0 = fm1_in.fad_hist[f][0], h1 = fm1_in.fad_hist[f][1];
        uint32_t m = raw > h0 ? (h0 > h1 ? h0 : raw > h1 ? h1 : raw) : (raw > h1 ? raw : h0 > h1 ? h1 : h0);
        fm1_in.fad_raw[f] = (uint16_t)raw; fm1_in.fad_hist[f][1] = (uint16_t)h0; fm1_in.fad_hist[f][0] = (uint16_t)raw;
        int32_t acc = fm1_in.fad_acc[f]; acc += ((int32_t)m * 16 - acc) / 4;
        fm1_in.fad_acc[f] = (uint16_t)acc;
        uint32_t v = (uint32_t)acc / 128;
        if (v > 127) v = 127;
        if (v > fm1_in.fader[f] + 1u || v + 1u < fm1_in.fader[f] || v == 0 || v == 127) fm1_in.fader[f] = (uint8_t)v;
    } else if (a < 6) fm1_in.wheel[a-4] += ((int32_t)raw - fm1_in.wheel[a-4]) / 4;
    else if (a == 6) fm1_in.pedal = (uint16_t)raw;
    else fm1_in.batt = (uint16_t)raw;
}
static void fm1_input_tick(void)
{
    static const uint8_t channels[3] = {1,8,2};
    if (smk_adc_slot != 0xFF && (SMK_ADC_CON & 0x80u)) smk_analog(smk_adc_slot, SMK_ADC_RES & 1023u);
    SMK_ADC_CON = 0x40; SMK_ADC_CON = 0;
    uint32_t a = FM1_PR(FM1_PA, FM1_IN), h = FM1_PR(FM1_PH, FM1_IN);
    fm1_in.raw[smk_col] = (uint8_t)~(((a>>2)&1u)|((a>>4)&14u)|((a>>5)&16u)|((h>>1)&32u)|((h>>2)&192u));
    if (!smk_col) smk_common(smk_grp);
    FM1_PR(FM1_PA, FM1_OUT) |= 2u; FM1_PR(FM1_PA, FM1_OUT) &= ~2u;
    uint32_t s = smk_slot;
    if (SMK_WLA & (1u << 14)) SMK_WLA &= ~(1u << 14);
    SMK_ADC_CON = 0x400Du | ((uint32_t)channels[s % 3] << 8);
    SMK_ADC_CON |= 0x10u; SMK_ADC_CON |= 0x40u;
    smk_adc_slot = (uint8_t)s; smk_slot = (uint8_t)((s + 1) % 24);
    smk_muxw = 0xFFFFFFC7u | (uint32_t)(smk_slot / 3) << 3;
    smk_col = (uint8_t)((smk_col + 1) & 15u);
    if (!smk_col) smk_frame();
    smk_shift(smk_col);
}
static void fm1_input_scan(void)
{
    for (unsigned i = 0; i < 16; i++) {
        uint32_t t = fm1_ticks(); fm1_input_tick();
        while (fm1_ticks() - t < 100u * FM1_TICKS_PER_US) ;
    }
}
static int32_t fm1_enc_take(uint32_t e)
{
    uint32_t f = irq_save();
    int32_t v = e < 8 ? fm1_in.enc_steps[e] : 0;
    if (e < 8) fm1_in.enc_steps[e] = 0;
    irq_restore(f); return v;
}
static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t f = irq_save(), v = fm1_in.pressed;
    if (released) *released = fm1_in.released;
    fm1_in.pressed = fm1_in.released = 0;
    irq_restore(f); return v;
}
static uint32_t fm1_input_note_edges(void)
{
    uint32_t f = irq_save(), v = fm1_in.notes_pressed;
    fm1_in.notes_pressed = 0; irq_restore(f); return v;
}
static int32_t smk_extkey_take(void)
{
    uint32_t f = irq_save(); int32_t v = -1;
    if (smk_ext_r != smk_ext_w) { v = smk_extq[smk_ext_r]; smk_ext_r = (uint8_t)((smk_ext_r + 1) % SMK_EXTQ); }
    irq_restore(f); return v;
}
static void fm1_led_key(uint32_t id, int on) { if (id < 19) fm1_led[id] = (uint8_t)!!on; }
static void fm1_led_dim_ns(uint32_t ns) { (void)ns; }
static void fm1_led_dim_level(uint32_t low) { (void)low; }
enum { FM1_ADC_BATT = 3, FM1_ADC_MASTER = 4 };
static int32_t smk_master = 512;
static void fm1_adc_init(void) {}
static int32_t fm1_adc_read(uint32_t ch)
{
    if (ch == FM1_ADC_BATT) return fm1_in.batt ? fm1_in.batt : -1;
    if (ch != FM1_ADC_MASTER) return -1;
    smk_master += fm1_enc_take(0) * 24;
    if (smk_master < 0) smk_master = 0;
    if (smk_master > 1023) smk_master = 1023;
    return smk_master;
}
