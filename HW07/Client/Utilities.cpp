#include "Utilities.h"

#pragma region Path

FILE* OpenFile(const char* path, const char* mode)
{
    FILE* fp;
    fopen_s(&fp, path, mode);
    if (fp == NULL) {
#ifdef _ERROR_DEBUGGING
        if (mode[0] == 'a')
            printf("[%s:%d] Fail to open file '%s' to %s.\n", ERROR_FLAGS, errno, path, "append");
        else if (mode[0] == 'w')
            printf("[%s:%d] Fail to open file '%s' to %s.\n", ERROR_FLAGS, errno, path, "write");
        else if (mode[0] == 'r')
            printf("[%s:%d] Fail to open file '%s' to %s.\n", ERROR_FLAGS, errno, path, "read");
#endif
    }
    return fp;
}

int CloseFile(FILE* fp)
{
    if (fp != NULL)
        return fclose(fp);
    return INVALID_ARGUMENTS;
}

int RemoveFile(const char* path)
{
    return (remove(path) == 0);
}

int MoveFilePointer(FILE* fp, int relative, int position)
{
    return fseek(fp, position, relative) == 0;
}

char* CreateUniquePath(const char* folderpath, uint folderlen)
{
    uint filelen = 20; // len of time_t in string
    char* filepath = Clone(folderpath, folderlen + filelen);
    if (filepath != NULL) {
        char* time_str = CreateStream(20); // long long max value = 9,223,372,036,854,775,807
        sprintf_s(time_str, 20, "%lld", time(0));
        memcpy_s(filepath + folderlen, filelen, time_str, filelen);
        free(time_str);
    }
    return filepath;
}

int IsExist(const char* path, int mode)
{
    return (_access(path, mode) == 0);
}

int CreateFolder(const char* path)
{
    return (_mkdir(path) == 0);
}

#pragma endregion

#pragma region File IO

int WriteToFile(FILE* fp, uint length, const stream data, uint* write_success)
{
    if (fp == NULL)
        return INVALID_ARGUMENTS;
    uint write_count = fwrite(data, sizeof(char), length, fp);
    int status = SUCCESS;
    if (write_count < length) {
#ifdef _ERROR_DEBUGGING
        printf("[%s:%d] Unexpected error occurs when writing file\n", WARNING_FLAGS, errno);
#endif // _ERROR_DEBUGGING
        status = FAIL;
    }
    if (write_success != NULL)
        *write_success = write_count;
    return status;
}

int ReadFromFile(FILE* fp, uint length, stream* odata, uint* read_success)
{
    if (fp == NULL || odata == NULL)
        return INVALID_ARGUMENTS;
    *odata = NULL;

    stream _data = CreateStream(length);
    if (_data == NULL)
        return FAIL;

    uint read_count = fread(_data, sizeof(char), length, fp);

    int status = SUCCESS;
    if (read_count < length) {
        if (ferror(fp)) {
#ifdef _ERROR_DEBUGGING
            printf("[%s:%d] Unexpected error occurs when reading file\n", WARNING_FLAGS, errno);
#endif // _ERROR_DEBUGGING
            DestroyStream(_data);
            return FATAL_ERROR;
        }
        status = FAIL; // eof
    }
    *odata = _data;
    if (read_success != NULL)
        *read_success = read_count;
    return status;
}

#pragma endregion

#pragma region ByteStream

uint ToUnsignedInt(const stream value)
{
    return *(uint*)value;
}

stream CreateStream(uint size)
{
    stream s = (stream)malloc(size);

    if (s == NULL) {
#ifdef _ERROR_DEBUGGING
        printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
#endif
    }
    return s;
}

stream Clone(const stream source, uint length, uint start)
{
    stream _clone = CreateStream(length + start);

    if (_clone != NULL)
        memcpy_s(_clone + start, length, source, length);
    return _clone;
}

void DestroyStream(stream bytestream)
{
    free(bytestream);
}

#pragma endregion
