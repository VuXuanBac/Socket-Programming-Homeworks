#pragma once

#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Debugging.h"

#define uint			unsigned int
#define ushort			unsigned short
#define ulong			unsigned long

#define stream			char*

#define FM_EXIST_ONLY	0x00
#define FM_WRITE_ONLY	0x02
#define FM_READ_ONLY	0x04
#define FM_READ_WRITE	0x06

#define FOM_READ		"rb"
#define FOM_WRITE		"wb"
#define FOM_APPEND		"ab"

#define UEOF			((uint)-1)

#pragma region Path

/// <summary>
/// Open a file
/// </summary>
/// <param name="path">The path to the file</param>
/// <param name="mode">The open mode. See FOM_ for some modes</param>
/// <returns>The FILE* object point to the opened file. NULL if fail to open</returns>
FILE* OpenFile(const char* path, const char* mode);

/// <summary>
/// Change file pointer to a specific position
/// </summary>
/// <param name="fp">The FILE* object point to a opened file</param>
/// <param name="relative">The relative position. See SEEK_ for some relative</param>
/// <param name="position">The absolute position. Positive means move forward after relative position. Negative means move backward before relative position</param>
/// <returns>1 if success. 0 otherwise</returns>
int MoveFilePointer(FILE* fp, int relative, int position);

/// <summary>
/// Close a file
/// </summary>
/// <param name="fp">The FILE* object point to the file want to close</param>
/// <returns></returns>
int CloseFile(FILE* fp);

/// <summary>
/// Create a folder at specific path and name
/// </summary>
/// <param name="path">The path to new folder</param>
/// <returns>1 if success. 0 otherwise</returns>
int CreateFolder(const char* path);

/// <summary>
/// Create a unique path (for file/folder) in a specific folder. In fact, it is a random number.
/// </summary>
/// <param name="folderpath">The path to exists folder</param>
/// <param name="folderlen">The size in bytes of "folderpath" field</param>
/// <returns>Created unique path</returns>
char* CreateUniquePath(const char* folderpath, uint folderlen);

/// <summary>
/// Check a path (file/folder) exists with specific mode
/// </summary>
/// <param name="path">The path want to check</param>
/// <param name="mode">The path mode. See FM_ for some modes</param>
/// <returns>1 if exists. 0 otherwise</returns>
int IsExist(const char* path, int mode = FM_EXIST_ONLY);

/// <summary>
/// Delete a file with specific path
/// </summary>
/// <param name="path">The path to the file want to delete</param>
/// <returns>1 if success. 0 otherwise</returns>
int RemoveFile(const char* path);

#pragma endregion

#pragma region File IO

/// <summary>
/// Write a byte stream to a file.
/// </summary>
/// <param name="fp">The FILE* object point to the opened file</param>
/// <param name="length">The length of the byte stream</param>
/// <param name="data">The byte stream want to write</param>
/// <param name="owrite_success">[Output] Number of bytes write successfully</param>
/// <returns>1 if success. 0 if owrite_success less than length</returns>
int WriteToFile(FILE* fp, uint length, const stream data, uint* owrite_success = NULL);

/// <summary>
/// Read a file and get a byte stream from it
/// </summary>
/// <param name="fp">The FILE* object point to the opened file</param>
/// <param name="length">The size in bytes expected to read</param>
/// <param name="odata">[Output:NotNull] The read byte stream</param>
/// <param name="oread_success">[Output] Number of bytes read successfully</param>
/// <returns>1 if success. 0 if oread_success less than length (reach EOF). -1 if have some errors on file.</returns>
int ReadFromFile(FILE* fp, uint length, stream* odata, uint* oread_success = NULL);

#pragma endregion

#pragma region ByteStream

/// <summary>
/// Cast value in a byte stream to unsigned int.
/// </summary>
/// <param name="value">A pointer to byte stream</param>
/// <returns>The cast value</returns>
uint ToUnsignedInt(const stream value);

/// <summary>
/// Created a stream object and Allocate memory for it
/// </summary>
/// <param name="size">The size of the stream in bytes</param>
/// <returns>The created stream object. NULL if fail to allocate memory</returns>
stream CreateStream(uint size);

/// <summary>
/// Free memory for the stream object
/// </summary>
/// <param name="bytestream">The stream object</param>
void DestroyStream(stream bytestream);

/// <summary>
/// Create a new memory space and Copy [length] bytes from [source] to it.
/// </summary>
/// <param name="source">The source bytes</param>
/// <param name="length">Number of bytes want to copy</param>
/// <param name="start">The first byte in destination will hold the 0th byte of source</param>
/// <returns>New memory space contains content of source. NULL if fail to allocate memory</returns>
stream Clone(const stream source, uint length, uint start = 0);

#pragma endregion
