/*
 * generator_stub.c
 *
 * LUU Y QUAN TRONG:
 * Day la ban CAI DAT THAM KHAO do TV2 tu viet tam thoi, dung de bien dich
 * va kiem thu doc lap parallel.c TRUOC KHI TV1 ban giao file generator.c
 * chinh thuc. Khi tich hop, PHAI thay the file nay bang generator.c that
 * cua TV1 (cung ten ham, cung header generator.h nen khong can sua parallel.c).
 * Khong nop file nay thay cho generator.c cua TV1 trong ban cuoi cung.
 */

#include "generator.h"
#include <string.h>

int calculate_search_space(uint64_t *total, int charset_size, int password_length)
{
    if (total == NULL || charset_size <= 0 || password_length <= 0) {
        return 0;
    }

    uint64_t result = 1;
    for (int i = 0; i < password_length; i++) {
        /* Kiem tra tran truoc khi nhan */
        if (result > UINT64_MAX / (uint64_t)charset_size) {
            return 0;
        }
        result *= (uint64_t)charset_size;
    }

    *total = result;
    return 1;
}

int generate_password(uint64_t index, const char *charset, int password_length, char *out)
{
    if (charset == NULL || out == NULL || password_length <= 0) {
        return 0;
    }

    int charset_size = (int)strlen(charset);
    if (charset_size <= 0) {
        return 0;
    }

    /* Dien tu vi tri cuoi ve dau: ky tu cuoi thay doi nhanh nhat */
    for (int pos = password_length - 1; pos >= 0; pos--) {
        int digit = (int)(index % (uint64_t)charset_size);
        out[pos] = charset[digit];
        index /= (uint64_t)charset_size;
    }
    out[password_length] = '\0';

    /* Neu index con du sau khi dien het L vi tri -> index vuot mien */
    if (index != 0) {
        return 0;
    }

    return 1;
}
