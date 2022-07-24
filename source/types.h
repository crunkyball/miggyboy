#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define STRINGIFY(X) #X
#define PLATFORM_INCLUDE(X) STRINGIFY(X)

#if defined(_WIN32)
#define PLATFORM_NAME windows
#elif defined(__linux__)
#undef linux    //Non-standard definition in GCC
#define PLATFORM_NAME linux
#else
#error Unknown platform!
#endif

#ifdef _DEBUG
#define DEBUG_ENABLED 1
#else
#define DEBUG_ENABLED 0
#endif

typedef uint8_t byte;
typedef int cycles;

typedef void(*CallbackFunc)();

#endif
