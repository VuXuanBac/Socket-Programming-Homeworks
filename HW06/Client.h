#pragma once

#pragma region Header Declarations

#include "ApplicationLibrary.h"

#pragma endregion

#pragma region Constants Definitions

#define INPUT_FLAGS ">>"
#define OUTPUT_FLAGS "**"

#pragma endregion

#pragma region Function Declarations

#pragma region Handle Request & Response
/// <summary>
/// Send requests: Encrypt/Decrypt Request + Data Requests + Upload End Request
/// </summary>
/// <param name="socket">The socket to the server</param>
/// <param name="request_type">The request type. See RT_ for some</param>
/// <param name="key">The key for encrypt/decrypt request</param>
/// <param name="file">The file path want to encrypt/decrypt</param>
/// <returns>1 if success. 0 if fail. -1 if have fatal errors</returns>
int SendRequest(SOCKET socket, int request_type, int key, const char* file);

/// <summary>
/// Handle the response from remote process: Collect message segmentations, Extract content and Write result to file
/// </summary>
/// <param name="socket">The connected socket used to communicate with remote process</param>
/// <param name="request_type">The type of the request sent before</param>
/// <param name="file">The file use for encrypt/decrypt before</param>
/// <returns>1 if success. 0 if fail. -1 if have errors that the socket should be closed</returns>
int HandleResponse(SOCKET socket, int request_type, const char* file);

#pragma endregion

#pragma region Handle I/O

/// <summary>
/// Print menu to console. The menu is shown once for each window after the connection established .
/// </summary>
void PrintMenu();

/// <summary>
/// Get request detail from User input: The key and The file for encryption/decryption
/// </summary>
/// <param name="option_str">A string describe the request type [Just for displaying to console]</param>
/// <param name="okey">[Output:NotNull] The encrypt/decrypt key from input</param>
/// <param name="ofile">[Output:NotNull] The file path want to encrypt/decrypt</param>
/// <returns>1 if user input is valid. 0 otherwise</returns>
int GetRequest(const char* option_str, int* okey, char** ofile);

/// <summary>
/// Get user command.
/// </summary>
/// <param name="ocode">[Output:NotNull] The request type</param>
/// <param name="okey">[Output:NotNull] The key for encrypt/decrypt</param>
/// <param name="ofile">[Output:NotNull] The file want to encrypt/decrypt</param>
/// <returns>1 if user input is valid. 0 otherwise</returns>
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

#pragma endregion