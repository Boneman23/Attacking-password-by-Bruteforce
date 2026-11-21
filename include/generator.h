#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>

// Tinh charset_size ^ password_length.
// Tra ve 1 neu thanh cong, 0 neu tham so khong hop le hoac tran uint64_t.
int calculate_search_space(uint64_t *total,
                           int charset_size,
                           int password_length);

// Chuyen index thanh chuoi co dung password_length ky tu.
// out phai co it nhat password_length + 1 byte.
// Tra ve 1 neu thanh cong, 0 neu index ngoai mien hoac tham so sai.
int generate_password(uint64_t index,
                      const char *charset,
                      int password_length,
                      char *out);

#endif
