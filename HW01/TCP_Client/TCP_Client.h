#pragma once
#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations

#include <stdio.h>
#include <stdlib.h>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "CommonDefinitions.h"
#pragma endregion

#pragma region Constants Definitions

#define SEGMENTATION_HEADER_REMAIN_SIZE 2
#define SEGMENTATION_HEADER_CURRENT_SIZE 2
#define SEGMENTATION_HEADER_SIZE 4

#pragma endregion

#pragma region Function Declarations

/// <summary>
/// Initialize Winsock 2.2
/// </summary>
/// <returns>1 if initialize successfully, 0 otherwise</returns>
int WSInitialize();

/// <summary>
/// Clean up Winsock 2.2
/// </summary>
/// <returns>1 if close successfully, 0 otherwise</returns>
int WSCleanup();

/// <summary>
/// Create a TCP/UDP Socket.
/// </summary>
/// <param name="protocol">TCP or UDP</param>
/// <returns>
/// Created TCP/UDP Socket.
/// INVALID_SOCKET if protocol is unexpected or have error on Winsock
/// </returns>
SOCKET CreateSocket(int protocol);

/// <summary>
/// Close a created Socket
/// </summary>
/// <param name="socket">The socket want to close</param>
/// <param name="mode">CLOSE_NORMAL or CLOSE_SAFELY (Shutdown before close)</param>
/// <param name="flags">Specify how to close socket safely. Some flags: SD_RECEIVE, SD_SEND, SD_BOTH (Manifest constants for shutdown()). This param is not used with CLOSE_NORMAL </param>
/// <returns>1 if close successfully, 0 otherwise</returns>
int CloseSocket(SOCKET socket, int mode, int flags = 0);

/// <summary>
/// Create a socket address that used IPv4 and Port Number
/// </summary>
/// <param name="ip">The IPv4 Address</param>
/// <param name="port">The Port number</param>
/// <returns>Created socket address</returns>
ADDRESS CreateSocketAddress(IP ip, int port);

/// <summary>
/// Establish a connection to a specified socket address
/// </summary>
/// <param name="socket">The socket want to connect</param>
/// <param name="address">The socket address of a remote process want to connect to</param>
/// <returns>1 if success. 0 otherwise, has errors</returns>
int EstablishConnection(SOCKET socket, ADDRESS address);

/// <summary>
/// Write a byte stream to the connected socket buffer, to send
/// </summary>
/// <param name="sender">The connected socket that is used for send message</param>
/// <param name="bytes">Number of bytes expected to send</param>
/// <param name="message">The bytes tream want to send</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected. -1 if have errors that the socket should be closed</returns>
int WriteSocketBuffer(SOCKET sender, int bytes, const char* message);

/// <summary>
/// Segmentation a message into pieces and Send them with a connected socket.
/// Each piece attached with the header consists of SEGMENTATION_HEADER_CURRENT_SIZE first bytes
/// is the length of message in the piece (not include header size) and SEGMENTATION_HEADER_REMAIN_SIZE next bytes
/// is the number of bytes on message that has not been sent.
/// </summary>
/// <param name="sender">The connected socket used for sending</param>
/// <param name="message">The message want to segmentation and send</param>
/// <param name="message_len">The length of the message</param>
/// <param name="obyte_sent">[Output] The bytes sent successfully</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected. -1 if have errors that the socket should be closed</returns>
int SegmentationSend(SOCKET sender, const char* message, int message_len, int* obyte_sent);

/// <summary>
/// Read a bytes stream from a connected socket 
/// </summary>
/// <param name="receiver">The connected socket that is used for receiving bytes stream</param>
/// <param name="bytes">Number of bytes want to read</param>
/// <param name="omessage">[Output] The byte streams read</param>
/// <returns>1 if read successfully. 0 if cant read completely. -1 if have errors that the socket should be closed</returns>
int ReadSocketBuffer(SOCKET receiver, int bytes, char** omessage);

/// <summary>
/// Read and extract message from a byte stream.
/// </summary>
/// <param name="receiver">The connected socket that is used for receiving byte streams</param>
/// <param name="omessage">[Output] The extracted message, after removing SEGMENTATION_HEADER_SIZE first bytes from byte stream</param>
/// <param name="omessage_len">[Output] The message size, in bytes</param>
/// <param name="oremain">[Output] Number of bytes in root message that have not been received</param>
/// <returns>1 if read successfully. 0 if cant read completely. -1 if have errors that the socket should be closed</returns>
int SegmentationReceive(SOCKET receiver, char** omessage, int* omessage_len, int* oremain);

/// <summary>
/// Handle the response from remote process: Collect message segmentations, Merge them and Print to console
/// </summary>
/// <param name="socket">The connected socket used to communicate with remote process</param>
/// <returns>1 if success. 0 if has error when receive responses and merge messages inside them.</returns>
int HandleResponse(SOCKET socket);

/// <summary>
/// Extract infomation in Message object and Print the message to console.
/// </summary>
/// <param name="message">The response Message object</param>
/// <param name="title">The title for message, which is shown previous. NULL for not use</param>
/// <returns>1 if has another response after this message, 0 otherwise</returns>
int PrintResponse(const MESSAGE message, const char* title = NULL);

/// <summary>
/// Merge many segmentation responses in a connected socket.
/// </summary>
/// <param name="socket">The connected socket used to receive segmentation</param>
/// <param name="omessage">[Output] The merged message</param>
/// <returns>1 if read successfully. 0 if cant read message completely. -1 if have errors that the socket should be closed</returns>
int MergeSegmentationMessage(SOCKET socket, MESSAGE* omessage);

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

/// <summary>
/// Try parse a string to a IPv4 Address
/// </summary>
/// <param name="str">The string want to parse</param>
/// <param name="oip">[Output] The result IPv4 Address</param>
/// <returns>1 if parse successfully. 0 otherwise</returns>
int TryParseIPString(const char* str, IP* oip);

/// <summary>
/// Set receive timeout interval for socket.
/// </summary>
/// <param name="socket">The socket want to set timeout</param>
/// <param name="interval">The timeout interval</param>
/// <returns>1 if set successfully, 0 otherwise</returns>
int SetReceiveTimeout(SOCKET socket, int interval);

/// <summary>
/// Create a new memory space and Copy <length> bytes from <root> to it.
/// </summary>
/// <param name="source">The source bytes</param>
/// <param name="length">Number of bytes want to copy</param>
/// <param name="start">The first byte in destination will hold the 0th byte of source</param>
/// <returns>New memory space contains content of source. NULL if fail to allocate memory</returns>
char* Clone(const char* source, int length, int start = 0);

/// <summary>
/// Free memory for Message object
/// </summary>
/// <param name="m">The message want to free</param>
void DestroyMessage(MESSAGE m);

#pragma endregion