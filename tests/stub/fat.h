#pragma once
#include <stdbool.h>
typedef struct { bool (*startup)(void); bool (*isInserted)(void); } DISC_INTERFACE;
static inline bool fatMountSimple(const char *n, const DISC_INTERFACE *i) { (void)n; (void)i; return true; }
static inline void fatUnmount(const char *n) { (void)n; }
