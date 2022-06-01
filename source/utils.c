#include <stdio.h>
#include <assert.h>

#include "utils.h"

#include "windows/platform_debug.h"

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
        assert(0);
    }

    assert(bSuccess);
    return bSuccess;
}
