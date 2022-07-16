#include <stdio.h>
#include <stdarg.h>

#include "platform_debug.h"

extern void DebugPrint(const char* pFmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, pFmt);
    vsprintf(buf, pFmt, args);
    va_end(args);

    printf(buf);
}
