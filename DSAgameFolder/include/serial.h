#ifndef SERIAL_H
#define SERIAL_H
#include <stdbool.h>

// Opens a serial port. Returns false if it cannot be opened.
bool serial_open(const char *port, int baud);

// Reads a byte, or returns -1 if none is waiting.
int serial_read_key(void);

bool serial_is_open(void);
void serial_close(void);

#endif
