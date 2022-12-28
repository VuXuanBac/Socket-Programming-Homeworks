#pragma once

#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations

#include <stdio.h>
#include <stdlib.h>

#include "Utilities.h"
#include "Debugging.h"
#include "WindowsSocketUtilities.h"
#include "Crypto.h"

#pragma endregion

#pragma region Constants Definitions

#define FILE_BUFF_MAX_SIZE 10240

#define RECEIVE_TIMEOUT_INTERVAL 10000
#define USER_INPUT_MAX_SIZE 1023
#define MESSAGE_MAX_SIZE 1023

#define DEFAULT_PORT 6600
#define DEFAULT_IP "127.0.0.1"

#define MC_ENCRYPT	0
#define MC_DECRYPT	1
#define MC_UPLOAD	2
#define MC_ERROR	3
#define MC_INVALID -1

#define RT_ENCRYPT	0
#define RT_DECRYPT	1
#define RT_INVALID	2

#define SIZE_LENGTH 4
#define CODE_LENGTH 1
#pragma endregion

#pragma region Type Definitions

#define MESSAGE char*

#pragma endregion

#pragma region Function Declarations

MESSAGE CreateMessage(int code, const char* byte_stream, uint length);

MESSAGE CreateMessage(int code, uint value);

int ExtractMessage(const MESSAGE message, char** obyte_stream, uint* olength);

void DestroyMessage(MESSAGE m);

uint ToUnsignedInt(const char* value);
#pragma endregion
