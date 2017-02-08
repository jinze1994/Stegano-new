#ifndef HABIT_H
#define HABIT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

static inline void Panic(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	exit(1);
}

static inline void Assert(bool ok, const char* fmt, ...) {
	if (!ok) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		exit(1);
	}
}

#endif
