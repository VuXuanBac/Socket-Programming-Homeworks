#pragma once

#pragma region Header Declarations

#include "ApplicationCommon.h"

#pragma endregion

#pragma region Constants Definitions

#define INPUT_FLAGS ">>"
#define OUTPUT_FLAGS "**"

#pragma endregion

#pragma region Function Declarations

/// <summary>
/// Send request to server and Handle response
/// </summary>
/// <param name="socket">The connected socket to server</param>
/// <param name="request">The request want to send</param>
/// <returns>1 if success. 0 if have some errors while sending or receiving. -1 if have errors that the socket should be closed</returns>
int Run(SOCKET socket, int request_type, int key, const char* file);

/// <summary>
/// Handle the response from remote process: Collect message segmentations, Merge them and Print to console
/// </summary>
/// <param name="socket">The connected socket used to communicate with remote process</param>
/// <returns>1 if success. 0 if cant read response fully. -1 if have errors that the socket should be closed</returns>
int HandleResponse(SOCKET socket, int request_type, const char* file);

/// <summary>
/// Print menu to console. The menu is shown once for each window after the connection established .
/// </summary>
void PrintMenu();

int GetRequest(const char* option_str, int* okey, char** ofile);

/// <summary>
/// Get user command and create a Message from the result.
/// </summary>
/// <param name="omessage">[Output] The created message</param>
/// <returns>1 if success. 0 if some user input is invalid. -1 if user choose a unsupported function</returns>
int HandleInput(int* ocode, int* okey, char** ofile);

/// <summary>
/// Extract port number and ipv4 string from command-line arguments.
/// If has error, set oport = 0 and oip = NULL.
/// </summary>
/// <param name="argc">Number of Arguments [From main()]</param>
/// <param name="argv">Arguments value [From main()]</param>
/// <param name="oport">[Output] The extracted port number</param>
/// <param name="oip">[Output] The extracted ip address</param>
/// <returns>1 if extract successfully. 0 otherwise, has error</returns>
int ExtractCommand(int argc, char* argv[], int* oport, IP* oip);

#pragma endregion