#include "Utilities.h"

int IsExist(const char* path, int mode)
{
    return (_access(path, mode) == 0);
}

int CreateFolder(const char* path)
{
    return _mkdir(path);
}

FILE* OpenFile(const char* path, const char* mode)
{
    FILE* fp;
    fopen_s(&fp, path, mode);
    if (fp == NULL) {
#ifdef _ERROR_DEBUGGING
        if (mode[0] == 'a')
            printf("[%s] Fail to open file '%s' to %s.\n", ERROR_FLAGS, path, "append");
        else if (mode[0] == 'w')
            printf("[%s] Fail to open file '%s' to %s.\n", ERROR_FLAGS, path, "write");
        else if (mode[0] == 'r')
            printf("[%s] Fail to open file '%s' to %s.\n", ERROR_FLAGS, path, "read");
#endif
        return NULL;
    }
    return fp;
}

int WriteToFile(FILE* fp, size_t length, const char* data, size_t* write_success)
{
    size_t write_count = fwrite(data, sizeof(char), length, fp);
    int status = 1;
    if (write_count < length) {
#ifdef _ERROR_DEBUGGING
        printf("[%s] Unexpected error occurs when writing file\n", WARNING_FLAGS);
#endif // _ERROR_DEBUGGING
        status = 0;
    }
    if (write_success != NULL)
        *write_success = write_count;
    return status;
}

int ReadFromFile(FILE* fp, size_t length, char** odata, size_t* read_success)
{
    size_t read_count = fread(*odata, sizeof(char), length, fp);
    int status = 1;
    if (read_count < length) {
        if (ferror(fp)) {
#ifdef _ERROR_DEBUGGING
            printf("[%s] Unexpected error occurs when reading file\n", WARNING_FLAGS);
#endif // _ERROR_DEBUGGING
            status = -1;
        }
        status = 0;
    }
    if (read_success != NULL)
        *read_success = read_count;
    return status;
}

int CloseFile(FILE* fp)
{
    return fclose(fp);
}

char* Clone(const char* source, int length, int start)
{
    char* _clone = (char*)malloc((size_t)length + start);

    if (_clone == NULL) {
#ifdef _ERROR_DEBUGGING
        printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
#endif
    }
    else
        memcpy_s(_clone + start, length, source, length);
    return _clone;
}