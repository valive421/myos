#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#include "types.h"

void serial_init(void);
void serial_write_char(char c);
void serial_write_str(const char* s);
void serial_write_hex32(uint32_t value);

#endif
