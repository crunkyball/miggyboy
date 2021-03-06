#include <stdio.h>
#include <assert.h>

#include "utils.h"

#include PLATFORM_INCLUDE(PLATFORM_NAME/platform_debug.h)

bool FileRead(const char* pFileName, byte* pBuffer, int bufferSize)
{
    FILE* pFile = fopen(pFileName, "rb");

    bool bSuccess = false;

    if (pFile != NULL)
    {
        int ret = fseek(pFile, 0, SEEK_END);
        assert(ret == 0);

        int fileSize = ftell(pFile);
        rewind(pFile);

        assert(fileSize <= bufferSize);

        int sizeToRead = MIN(fileSize, bufferSize);
        int sizeRead = (int)fread(pBuffer, 1, sizeToRead, pFile);
        bSuccess = sizeToRead == sizeRead;

        fclose(pFile);
    }
    else
    {
        DebugPrint("Failed to open file");
    }

    return bSuccess;
}

bool FileWrite(const char* pFileName, byte* pBuffer, int bufferSize)
{
    FILE* pFile = fopen(pFileName, "wb");

    bool bSuccess = false;

    if (pFile != NULL)
    {
        int sizeWrote = (int)fwrite(pBuffer, 1, bufferSize, pFile);
        bSuccess = sizeWrote == bufferSize;

        fclose(pFile);
    }

    return bSuccess;
}
