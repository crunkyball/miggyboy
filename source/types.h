#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define STRINGIFY(X) #X
#define PLATFORM_INCLUDE(X) STRINGIFY(X)

#ifdef _WIN32
#define PLATFORM_NAME windows
#else
#error Unknown platform!
#endif

#define DEBUG_ENABLED 1

typedef uint8_t byte;
typedef int cycles;

typedef void(*CallbackFunc)();

#endif
