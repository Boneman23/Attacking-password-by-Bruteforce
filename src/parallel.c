/*
 * parallel.c
 *
 * Trien khai run_parallel() theo API trong parallel.h (muc 3.4 tai lieu phan cong).
 * Thuoc vai tro TV2 - Parallel/OpenMP.
 *
 * Thiet ke:
 * - Vong lap chinh duyet index tu 0 den N-1 bang #pragma omp parallel for.
 * - schedule(runtime) duoc dung de co the chon static/dynamic LUC CHAY,
 *   boi vi OpenMP khong cho phep truyen bien vao truc tiep tham so
 *   "schedule(static)" / "schedule(dynamic, chunk)" trong pragma.
 *   omp_set_schedule() duoc goi TRUOC vung song song de thiet lap kieu
 *   lap lich thuc su se duoc dung khi gap schedule(runtime).
 * - candidate: mang ky tu khai bao BEN TRONG than vong lap -> moi iteration
 *   tren moi thread co vung nho rieng (tuong duong private), khong co
 *   thread nao ghi de len buffer cua thread khac.
 * - attempts: dung reduction(+:attempts) de cong don an toan, tranh
 *   nhieu thread cung tang mot bien shared ma khong dong bo (lost update).
 * - found_index: dung reduction(min:found_index), khoi tao UINT64_MAX,
 *   de lay chi so nho nhat khop voi target ma khong can critical/atomic
 *   thu cong. (Yeu cau GCC co ho tro OpenMP reduction tren kieu so nguyen
 *   khong dau 64-bit, da kiem tra hoat dong tren GCC chuan voi -fopenmp.)
 * - Thoi gian do (elapsed_time) CHI bao gom vung thuc thi thuat toan,
 *   bao gom ca chi phi tao/dieu phoi team OpenMP vi team duoc tao
 *   BEN TRONG ham nay (dung quy uoc da ghi trong muc 3.7 tai lieu).
 *
 * Che do baseline la EXHAUSTIVE: duyet du N chi so, khong dung som khi
 * da tim thay target, de attempts luon bang N va co the so sanh cong
 * bang voi Sequential.
 */

#include "parallel.h"
#include "generator.h"

#include <omp.h>
#include <string.h>
#include <stdint.h>

/* Kiem tra cau hinh truoc khi chay, dung chung quy uoc voi Sequential */
static int validate_config(const SearchConfig *config, uint64_t *out_total)
{
    if (config == NULL || config->charset == NULL || config->target == NULL) {
        return 0;
    }
    if (config->password_length <= 0 ||
        config->password_length > MAX_PASSWORD_LENGTH) {
        return 0;
    }
    if (config->num_threads <= 0) {
        return 0;
    }

    int charset_len = (int)strlen(config->charset);
    if (charset_len <= 0) {
        return 0;
    }

    /* charset khong duoc chua ky tu trung lap */
    for (int i = 0; i < charset_len; i++) {
        for (int j = i + 1; j < charset_len; j++) {
            if (config->charset[i] == config->charset[j]) {
                return 0;
            }
        }
    }

    /* target phai co dung password_length ky tu */
    if ((int)strlen(config->target) != config->password_length) {
        return 0;
    }

    /* moi ky tu cua target phai thuoc charset */
    for (int i = 0; i < config->password_length; i++) {
        if (strchr(config->charset, config->target[i]) == NULL) {
            return 0;
        }
    }

    uint64_t total = 0;
    if (!calculate_search_space(&total, charset_len, config->password_length)) {
        return 0;
    }

    *out_total = total;
    return 1;
}

Result run_parallel(const SearchConfig *config, ScheduleKind schedule)
{
    Result result;
    result.attempts = 0;
    result.found_index = UINT64_MAX;
    result.elapsed_time = 0.0;
    result.found = 0;

    uint64_t total = 0;
    if (!validate_config(config, &total)) {
        /* Cau hinh khong hop le: tra ve Result bao loi theo quy uoc chung,
           khong am tham chay voi du lieu sai (muc 3.6). */
        return result;
    }

    int password_length = config->password_length;
    const char *charset = config->charset;
    const char *target = config->target;
    int chunk = (config->chunk_size > 0) ? config->chunk_size : 1;

    /* Thiet lap kieu lap lich se duoc dung boi schedule(runtime) ben duoi.
       Day la cach chuan de "tham so hoa" static/dynamic trong OpenMP. */
    if (schedule == SCHEDULE_DYNAMIC) {
        omp_set_schedule(omp_sched_dynamic, chunk);
    } else {
        omp_set_schedule(omp_sched_static, chunk);
    }

    uint64_t attempts = 0;
    uint64_t found_index = UINT64_MAX;

    double start_time = omp_get_wtime();

    #pragma omp parallel for schedule(runtime) \
        num_threads(config->num_threads) \
        reduction(+:attempts) reduction(min:found_index)
    for (uint64_t index = 0; index < total; index++) {
        char candidate[MAX_PASSWORD_LENGTH + 1]; /* private: khai bao trong than vong lap */

        if (generate_password(index, charset, password_length, candidate)) {
            attempts++;
            if (strcmp(candidate, target) == 0) {
                if (index < found_index) {
                    found_index = index;
                }
            }
        }
        /* Exhaustive baseline: KHONG break/return som, tiep tuc duyet het */
    }

    double end_time = omp_get_wtime();

    result.attempts = attempts;
    result.found_index = found_index;
    result.found = (found_index != UINT64_MAX) ? 1 : 0;
    result.elapsed_time = end_time - start_time;

    return result;
}
