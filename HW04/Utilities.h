#pragma once

#include <io.h>
#include <stdio.h>
#include <direct.h>
#include <memory.h>
#include <stdlib.h>

#define uint unsigned int
#define ushort unsigned short
#define ulong unsigned long

#define FM_EXIST_ONLY 0x00
#define FM_WRITE_ONLY 0x02
#define FM_READ_ONLY 0x04
#define FM_READ_WRITE 0x06

FILE* OpenFile(const char* path, const char* mode);

int WriteToFile(FILE* fp, size_t length, const char* data, size_t* write_success = NULL);

int ReadFromFile(FILE* fp, size_t length, char** odata, size_t* read_success = NULL);

int CloseFile(FILE* fp);

int CreateFolder(const char* path);

int IsExist(const char* path, int mode = FM_EXIST_ONLY);

/// <summary>
/// Create a new memory space and Copy [length] bytes from [source] to it.
/// </summary>
/// <param name="source">The source bytes</param>
/// <param name="length">Number of bytes want to copy</param>
/// <param name="start">The first byte in destination will hold the 0th byte of source</param>
/// <returns>New memory space contains content of source. NULL if fail to allocate memory</returns>
char* Clone(const char* source, int length, int start = 0);