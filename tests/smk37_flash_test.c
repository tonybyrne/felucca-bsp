/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
/* The real RAM entry points must reject protected writes even with an overridden allow-list. */
#define BOARD_SMK37 1
#if TEST_OVERRIDE
#define FL_RANGE_OK(off, n) 1
#endif
#define main fm1_driver_original_main
#include "flash_driver_test.c"
#undef main

int main(void)
{
    uint32_t s, protected_count = 0;
    reset();
    for (s = 0; s < 0x100000u; s += 0x1000u) {
        int protected = s < 0x4000u || (s >= 0xC0000u && s < 0xC2000u) ||
            (s >= 0xF4000u && s < 0xFE000u) || s >= 0xFF000u;
        int app = (s >= 0x9D000u && s < 0xC0000u) || (s >= 0xC2000u && s <= 0xEA000u);
        int loader = s >= 0x4000u && s < 0xC0000u;
        CHECK(!!FL_PROTECTED(s, 0x1000u) == protected);
        CHECK(!!FL_LDR_RANGE_OK(s, 0x1000u) == loader);
#if !TEST_OVERRIDE
        CHECK(!!FL_RANGE_OK(s, 0x1000u) == app);
#else
        (void)app;
#endif
        if (!protected)
            continue;
        protected_count++;
        CHECK(!FL_GUARD(s, 0x1000u));
        CHECK(!FL_WRITE_OK(s, 0x1000u));
        CHECK(fl_erase4k_ram(s, TOOK) == FL_ERR_RANGE);
        CHECK(fl_erase4k_begin_ram(s, SAVED, ISSUED) == FL_ERR_RANGE);
        CHECK(fl_prog_ram(s, RAM(0x1C20000u), 256u, TOOK) == FL_ERR_RANGE);
        CHECK(fl_prog_ram(s + 0xF00u, RAM(0x1C20000u), 256u, TOOK) == FL_ERR_RANGE);
    }
    CHECK(protected_count == 17u);
    CHECK(!FL_GUARD(0xBF000u, 0x2000u));
    CHECK(!FL_GUARD(0x3F00u, 0x200u));
    CHECK(!FL_GUARD(0xBB000u, 0xFFFFF000u));
    CHECK(fl_erase4k_ram(0xFFFFF000u, TOOK) == FL_ERR_RANGE);
    CHECK(fl_erase4k_begin_ram(0xFFFFF000u, SAVED, ISSUED) == FL_ERR_RANGE);
    CHECK(fl_prog_ram(UINT32_MAX, RAM(0x1C20000u), 2u, TOOK) == FL_ERR_RANGE);
    CHECK(flushes == 0 && syncs == 0);
#if !TEST_OVERRIDE
    CHECK(FL_GUARD(0xEA000u, 0x1000u));
    CHECK(!FL_GUARD(0xEA000u, 256u));
    CHECK(!FL_GUARD(0xEAF00u, 256u));
    CHECK(!FL_GUARD(0xE9000u, 0x2000u));
#endif
    printf("SMK flash guard: %u checks, %u failures\n", checks, fails);
    return fails != 0;
}
