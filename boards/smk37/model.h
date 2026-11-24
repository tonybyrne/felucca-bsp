/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
#pragma once
#if !BOARD_SMK37
#error "SMK model configuration is for BOARD_SMK37 builds only"
#endif
#ifndef SMK_MODEL
#define SMK_MODEL "SMK-37 Pro"
#endif
#define OTA_AREA 0xBB000u
#define OTA_AREA_LEN 0x5000u
#define OTA_REC_NAME "ota-" SMK_MODEL "_016"
_Static_assert(sizeof("ota-" SMK_MODEL "_900") <= 24u, "SMK loader identity fits the handshake");
