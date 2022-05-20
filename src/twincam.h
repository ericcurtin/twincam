#pragma once

#include <syslog.h>

extern bool to_syslog;

void print(const char* const fmt, ...);
void eprint(const char* const fmt, ...);
