#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

bool FileRead(const char* pFileName, byte* pBuffer, int bufferSize);

#endif
