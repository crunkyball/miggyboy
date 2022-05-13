#include <stdio.h>
#include <assert.h>

#include "utils.h"

bool readFile(const char* pFileName, byte* pBuffer, int bufferSize)
{
    FILE* pFile = fopen(pFileName, "rb");

    bool bSuccess = false;

    if (pFile != NULL)
    {
        int ret = fseek(pFile, 0, SEEK_END);
        assert(ret == 0);

        int fileSize = ftell(pFile);
        rewind(pFile);

        if (fileSize <= bufferSize)
        {
            int sizeRead = (int)fread(pBuffer, 1, fileSize, pFile);
            bSuccess = (sizeRead == fileSize);
        }
        else
        {
            fprintf(stderr, "Buffer size too small for file\n");
        }

        fclose(pFile);
    }
    else
    {
        perror("Failed to open file");
    }

    assert(bSuccess);   //Failed to load file!
    return bSuccess;
}
