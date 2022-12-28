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

#define MAX_CONNECTIONS SOMAXCONN

#define SEGMENTATION_HEADER_REMAIN_SIZE 2
#define SEGMENTATION_HEADER_CURRENT_SIZE 2
#define SEGMENTATION_HEADER_SIZE 4

#define INT_MAX_LEN 10

#define ERROR_MESSAGE "Failed: String contains non-number character."
#pragma endregion

#pragma region Type Definitions

#define ADDRESS SOCKADDR_IN
#define IP IN_ADDR
#define MESSAGE char*

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
/// Bind a socket to an address [IPv4, Port]
/// </summary>
/// <param name="socket">The socket want to bind</param>
/// <param name="addr">A socket address [IPv4, Port]</param>
/// <returns>1 is bind successfully. 0 otherwise</returns>
int BindSocket(SOCKET socket, ADDRESS addr);

/// <summary>
/// Create a socket address that used IPv4 and Port Number
/// </summary>
/// <param name="ip">The IPv4 Address</param>
/// <param name="port">The Port number</param>
/// <returns>Created socket address</returns>
ADDRESS CreateSocketAddress(IP ip, int port);

/// <summary>
/// Set listen state for a socket.
/// </summary>
/// <param name="socket">The socket used for listen connections</param>
/// <param name="connection_numbers">Size of socket connections pending queue. Default SOMAXCONN (Set by underlying service provider)</param>
/// <returns>1 if has no errors. 0 otherwise</returns>
int ListenConnections(SOCKET socket, int connection_numbers = MAX_CONNECTIONS);

/// <summary>
/// Extract and Accept the first connection from listener socket pending queue.
/// This function blocks the program if the pending queue is empty
/// </summary>
/// <param name="listener">The listener socket used to listen connections</param>
/// <param name="osender_address">The address of the process establishes the connection (by connect())</param>
/// <returns>A socket for the top connection on pending queue.</returns>
SOCKET GetConnectionSocket(SOCKET listener, ADDRESS* osender_address = NULL);

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
/// Calculate sum of digits in the string
/// </summary>
/// <param name="str">The string input</param>
/// <param name="strlen">Number of bytes will be processed</param>
/// <returns>-1 if string contains not-digit characters. Otherwise, return an integer is the sum of digits</returns>
int GetSumDigitOnString(const char* str, int strlen);

/// <summary>
/// Handle requests: Read requests from buffer, Processing requests and Send response back.
/// </summary>
/// <param name="socket">The connected socket to the remote process</param>
/// <returns>1 if have no errors. 0 if request cant be processed completely. 
/// -1 if have errors and the socket cant be used anymore (lost connection to remote process)</returns>
int HandleRequest(SOCKET socket);

/// <summary>
/// Extract port number from command-line arguments.
/// If has error, use default port number [predefined, See: DEFAULT_PORT]
/// </summary>
/// <param name="argc">Number of Arguments [From main()]</param>
/// <param name="argv">Arguments value [From main()]</param>
/// <param name="oport">[Output] The extracted port number</param>
/// <returns>1 if extract successfully. 0 otherwise</returns>
int ExtractCommand(int argc, char* argv[], int* oport);

/// <summary>
/// Create a INADDR_ANY IP Address
/// </summary>
/// <returns>The INADDR_ANY IP</returns>
IP CreateDefaultIP();

/// <summary>
/// Create a new memory space and Copy <length> bytes from <root> to it.
/// </summary>
/// <param name="source">The source bytes</param>
/// <param name="length">Number of bytes want to copy</param>
/// <param name="start">The first byte in destination will hold the 0th byte of source</param>
/// <returns>New memory space contains content of source. NULL if fail to allocate memory</returns>
char* Clone(const char* source, int length, int start = 0);

/// <summary>
/// Create a Message object that used to send back to client.
/// The first bit of Message is the message's status: See STATUS_ definitions
/// </summary>
/// <param name="status">The status (flag) for the message</param>
/// <param name="message">The response message want to send</param>
/// <returns>A message with status</returns>
MESSAGE CreateMessage(int status, const char* message);

/// <summary>
/// Free memory for Message object
/// </summary>
/// <param name="m">The message want to free</param>
void DestroyMessage(MESSAGE m);

#pragma endregion