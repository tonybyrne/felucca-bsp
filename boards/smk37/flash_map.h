/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * Copyright (C) 2026 Tony Byrne */
/* Reference Felucca SMK allow-list, adapted to felucca-bsp's policy interface.
 * Half-open bounds; no extension of the reference's writable windows. */
#pragma once
#define FL_HEAD_HI      0x00004000u
#define FL_CAL_LO       0x000C0000u
#define FL_CAL_HI       0x000C2000u
#define FL_USRRES_LO    0x000F4000u
#define FL_USRRES_HI    0x000FE000u
#define FL_KEYMAC_LO    0x000FF000u
#define FL_APP_LO       0x00004000u
#define FL_APP_HI       0x0009D000u
#define FL_REC_LO       0x0009D000u
#define FL_REC_HI       0x000C0000u
#define FL_PLAIN_LO     0x0009D000u
#define FL_DATA_LO      0x0009D000u
#define FL_DATA_HI      0x000BB000u
#define FL_OTA_LO       0x000BB000u
#define FL_OTA_HI       0x000C0000u
#define FL_SMP_LO       0x000C2000u
#define FL_SMP_HI       0x000EA000u
#define FL_STOCKTAG_LO  0x000EA000u
#define FL_STOCKTAG_HI  0x000EB000u
#define FL_STOCKTAG_OFF 0x000EAFF8u
#define FL_STOCKTAG_OK(off, n) ((uint32_t)(off) == FL_STOCKTAG_LO && (uint32_t)(n) == FL_STOCKTAG_HI - FL_STOCKTAG_LO)
#define FL_WIN_SAFE(lo, hi) ((lo) % 0x1000u == 0u && (hi) % 0x1000u == 0u && (lo) < (hi) && \
    (lo) >= FL_HEAD_HI && (hi) <= FL_KEYMAC_LO && \
    ((hi) <= FL_CAL_LO || (lo) >= FL_CAL_HI) && ((hi) <= FL_USRRES_LO || (lo) >= FL_USRRES_HI))
_Static_assert(FL_WIN_SAFE(FL_DATA_LO, FL_DATA_HI), "SMK: FL_DATA overlaps a protected range");
_Static_assert(FL_WIN_SAFE(FL_OTA_LO, FL_OTA_HI), "SMK: FL_OTA overlaps a protected range");
_Static_assert(FL_WIN_SAFE(FL_SMP_LO, FL_SMP_HI), "SMK: FL_SMP overlaps a protected range");
_Static_assert(FL_WIN_SAFE(FL_STOCKTAG_LO, FL_STOCKTAG_HI) && FL_STOCKTAG_LO == FL_SMP_HI &&
    FL_STOCKTAG_OFF + 8u == FL_STOCKTAG_HI, "SMK: the stock marker sector");
_Static_assert(FL_WIN_SAFE(FL_APP_LO, FL_APP_HI), "SMK: FL_APP overlaps a protected range");
_Static_assert(FL_WIN_SAFE(FL_REC_LO, FL_REC_HI), "SMK: FL_REC overlaps a protected range");
_Static_assert(FL_REC_HI == FL_CAL_LO && FL_OTA_HI <= FL_REC_HI && FL_OTA_LO >= FL_REC_LO,
    "SMK: update records must remain in VM below BTIF");
_Static_assert(FL_DATA_HI <= FL_OTA_LO && FL_APP_HI <= FL_DATA_LO, "SMK: windows overlap");
#define FL_LDR_RANGE_OK(off, n) (FL_IN(off, n, FL_APP_LO, FL_APP_HI) || FL_IN(off, n, FL_REC_LO, FL_REC_HI))
#define FL_STORE_OK(off, n) (FL_IN(off, n, FL_DATA_LO, FL_DATA_HI) || FL_IN(off, n, FL_SMP_LO, FL_SMP_HI))
#ifndef FL_RANGE_OK
#define FL_RANGE_OK(off, n) (FL_STORE_OK(off, n) || FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || FL_STOCKTAG_OK(off, n))
#endif
#define FL_HITS(off, n, lo, hi) ((uint32_t)(off) < (hi) && \
    ((uint32_t)(off) >= (lo) || (uint32_t)(n) > (lo) - (uint32_t)(off)))
#define FL_PROTECTED(off, n) (FL_HITS(off, n, 0u, FL_HEAD_HI) || FL_HITS(off, n, FL_CAL_LO, FL_CAL_HI) || \
    FL_HITS(off, n, FL_USRRES_LO, FL_USRRES_HI) || (uint32_t)(off) >= FL_KEYMAC_LO || \
    (uint32_t)(n) > FL_KEYMAC_LO - (uint32_t)(off))
#define FL_GUARD(off, n) (FL_RANGE_OK(off, n) && !FL_PROTECTED(off, n))
