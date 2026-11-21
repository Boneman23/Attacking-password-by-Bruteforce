#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define MAX_PASSWORD_LENGTH 64

typedef enum {
    SCHEDULE_STATIC = 0,
    SCHEDULE_DYNAMIC = 1
} ScheduleKind;

typedef struct {
    const char *charset;       // Tap ky tu duoc phep
    const char *target;        // Mat khau muc tieu gia lap
    int password_length;       // Do dai co dinh cua target
    int num_threads;           // So thread; sequential khong dung
    int chunk_size;            // Chunk cho dynamic scheduling
} SearchConfig;

typedef struct {
    uint64_t attempts;         // So ung vien da kiem tra
    uint64_t found_index;      // Chi so tim thay; UINT64_MAX neu khong tim thay
    double elapsed_time;       // Giay
    int found;                 // 1 neu tim thay, 0 neu khong
} Result;

#endif
