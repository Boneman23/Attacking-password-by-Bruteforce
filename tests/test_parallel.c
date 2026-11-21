/*
 * test_parallel.c
 *
 * Test hardcode cho run_parallel(). Khong dung framework ngoai, chi dung
 * printf + assert de de chay tren moi may co GCC + OpenMP.
 *
 * Cac truong hop kiem tra:
 * 1. Target o dau khong gian (index 0)
 * 2. Target o giua khong gian
 * 3. Target o cuoi khong gian (index N-1)
 * 4. Target khong ton tai trong khong gian
 * Moi truong hop chay voi nhieu so thread (1, 2, 4) va ca hai kieu
 * scheduling (static, dynamic), kiem tra attempts == N va found/found_index
 * dung nhu ky vong.
 */

#include "common.h"
#include "parallel.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

static void check_case(const char *case_name,
                        SearchConfig config,
                        uint64_t expected_total,
                        int expected_found,
                        uint64_t expected_found_index,
                        int threads,
                        ScheduleKind schedule)
{
    config.num_threads = threads;

    Result r = run_parallel(&config, schedule);

    int ok = 1;
    if (r.attempts != expected_total) {
        printf("  [FAIL] attempts: got %llu, expected %llu\n",
               (unsigned long long)r.attempts,
               (unsigned long long)expected_total);
        ok = 0;
    }
    if (r.found != expected_found) {
        printf("  [FAIL] found: got %d, expected %d\n", r.found, expected_found);
        ok = 0;
    }
    if (expected_found && r.found_index != expected_found_index) {
        printf("  [FAIL] found_index: got %llu, expected %llu\n",
               (unsigned long long)r.found_index,
               (unsigned long long)expected_found_index);
        ok = 0;
    }

    const char *sched_name = (schedule == SCHEDULE_STATIC) ? "static" : "dynamic";
    if (ok) {
        printf("  [PASS] %s | threads=%d | schedule=%s | time=%.6fs\n",
               case_name, threads, sched_name, r.elapsed_time);
        g_pass++;
    } else {
        printf("  ^^^^^ %s | threads=%d | schedule=%s\n", case_name, threads, sched_name);
        g_fail++;
    }
}

int main(void)
{
    /* charset = "0123456789", password_length = 3 -> N = 1000 */
    const char *charset = "0123456789";
    const int length = 3;
    const uint64_t total = 1000; /* 10^3 */

    /* Voi exhaustive tren toan bo charset, moi chuoi hop le deu nam trong
       khong gian tim kiem, nen truong hop "khong tim thay that su" chi xay
       ra khi target chua ky tu KHONG thuoc charset (cau hinh khong hop le).
       Day cung la truong hop can kiem tra theo muc 3.6: khong am tham chay
       voi du lieu sai. */
    struct {
        const char *name;
        const char *target;
        int found;
        uint64_t found_index;
        uint64_t expected_total; /* 0 neu config khong hop le */
    } cases[] = {
        {"target_dau",           "000", 1, 0,   total},
        {"target_giua",          "042", 1, 42,  total},
        {"target_cuoi",          "999", 1, 999, total},
        {"target_khong_hop_le",  "0X0", 0, 0,   0}, /* 'X' khong thuoc charset */
    };

    int thread_options[] = {1, 2, 4};
    ScheduleKind schedule_options[] = {SCHEDULE_STATIC, SCHEDULE_DYNAMIC};

    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        printf("Test case: %s (target=\"%s\")\n", cases[c].name, cases[c].target);

        SearchConfig config;
        config.charset = charset;
        config.target = cases[c].target;
        config.password_length = length;
        config.num_threads = 1;
        config.chunk_size = 16;

        for (size_t t = 0; t < sizeof(thread_options) / sizeof(thread_options[0]); t++) {
            for (size_t s = 0; s < sizeof(schedule_options) / sizeof(schedule_options[0]); s++) {
                check_case(cases[c].name, config, cases[c].expected_total,
                           cases[c].found, cases[c].found_index,
                           thread_options[t], schedule_options[s]);
            }
        }
        printf("\n");
    }

    printf("Tong ket: %d PASS, %d FAIL\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
