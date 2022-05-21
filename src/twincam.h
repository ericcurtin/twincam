#pragma once

#include <syslog.h>

extern bool to_syslog;
extern bool print_available_cameras;

void print(const char* const fmt, ...);
void eprint(const char* const fmt, ...);
