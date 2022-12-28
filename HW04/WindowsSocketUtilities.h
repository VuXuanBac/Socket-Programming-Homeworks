#pragma once

#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations

#ifdef _ERROR_DEBUGGING
#include <stdio.h>
#endif

#include "Utilities.h"

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma endregion

#pragma region Constants Definitions

#define APPLICATION_BUFF_MAX_SIZE 1024
#define MAX_CONNECTIONS SOMAXCONN

#define SEGMENT_HEADER_REMAIN_SIZE 2
#define SEGMENT_HEADER_CURRENT_SIZE 2
#define SEGMENT_HEADER_SIZE 4

#define UDP 0
#define TCP 1

#define CLOSE_NORMAL 0
#define CLOSE_SAFELY 1

#pragma endregion

#pragma region Type Definitions

#define ADDRESS SOCKADDR_IN
#define IP IN_ADDR

#pragma endregion

#pragma region Function Declarations

#pragma region Socket Common

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
/// Create a INADDR_ANY IP Address
/// </summary>
/// <returns>The INADDR_ANY IP</returns>
IP CreateDefaultIP();

/// <summary>
/// Create a socket address that used IPv4 and Port Number
/// </summary>
/// <param name="ip">The IPv4 Address</param>
/// <param name="port">The Port number</param>
/// <returns>Created socket address</returns>
ADDRESS CreateSocketAddress(IP ip, int port);

#pragma endregion

#pragma region Server

/// <summary>
/// Bind a socket to an address [IPv4, Port]
/// </summary>
/// <param name="socket">The socket want to bind</param>
/// <param name="addr">A socket address [IPv4, Port]</param>
/// <returns>1 is bind successfully. 0 otherwise</returns>
int BindSocket(SOCKET socket, ADDRESS addr);

/// <summary>
/// Set listen state for a socket.
/// </summary>
/// <param name="socket">The socket used for listen connections</param>
/// <param name="connection_numbers">Size of socket connections pending queue. Default SOMAXCONN (Set by underlying service provider)</param>
/// <returns>1 if has no errors. 0 otherwise</returns>
int SetListenState(SOCKET socket, int connection_numbers = MAX_CONNECTIONS);

/// <summary>
/// Extract and Accept the first connection from listener socket pending queue.
/// This function blocks the program if the pending queue is empty
/// </summary>
/// <param name="listener">The listener socket used to listen connections</param>
/// <param name="osender_address">The address of the process establishes the connection (by connect())</param>
/// <returns>A socket for the top connection on pending queue.</returns>
SOCKET GetConnectionSocket(SOCKET listener, ADDRESS* osender_address = NULL);

#pragma endregion

#pragma region Client

/// <summary>
/// Establish a connection to a specified socket address
/// </summary>
/// <param name="socket">The socket want to connect</param>
/// <param name="address">The socket address of a remote process want to connect to</param>
/// <returns>1 if success. 0 otherwise, has errors</returns>
int EstablishConnection(SOCKET socket, ADDRESS address);

#pragma endregion


#pragma region Send and Receive

/// <summary>
/// Write a byte stream to the connected socket buffer and send
/// </summary>
/// <param name="sender">The connected socket that is used for sending byte stream</param>
/// <param name="bytes">Number of bytes expected to send</param>
/// <param name="byte_stream">The byte stream want to send</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected. -1 if have errors that the socket should be closed</returns>
int Send(SOCKET sender, int bytes, const char* byte_stream);

/// <summary>
/// Segmentation a message into pieces/segment and Send them with a connected socket.
/// Each piece attached with the header consists of SEGMENT_HEADER_CURRENT_SIZE first bytes
/// is the length of message in the piece (not include header size) and SEGMENT_HEADER_REMAIN_SIZE next bytes
/// is the number of bytes on message that has not been sent.
/// </summary>
/// <param name="sender">The connected socket used for sending</param>
/// <param name="message">The message want to segmentation and send</param>
/// <param name="message_len">The length of the message</param>
/// <param name="obyte_sent">[Output] Number of bytes sent successfully</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected. -1 if have errors that the socket should be closed</returns>
int SegmentationSend(SOCKET sender, const char* message, int message_len, int* obyte_sent);

int ReliableSend(SOCKET socket, const char* message, int message_len);

/// <summary>
/// Read a byte stream from a connected socket 
/// </summary>
/// <param name="receiver">The connected socket that is used for receiving bytes stream</param>
/// <param name="bytes">Number of bytes want to read</param>
/// <param name="obyte_stream">[Output] The byte streams read</param>
/// <returns>1 if read successfully. 0 if cant read fully. -1 if have errors that the socket should be closed</returns>
int Receive(SOCKET receiver, int bytes, char** obyte_stream);

/// <summary>
/// Read and Extract one part of/a segment of a message from a connected socket.
/// </summary>
/// <param name="receiver">The connected socket that is used for receiving byte streams</param>
/// <param name="obyte_stream">[Output] The extracted segment, after removing SEGMENT_HEADER_SIZE first bytes from byte stream</param>
/// <param name="ostream_len">[Output] The segment size, in bytes</param>
/// <param name="oremain">[Output] Number of bytes in source message that have not been received</param>
/// <returns>1 if success. 0 if cant read fully. -1 if have errors that the socket should be closed</returns>
int ReceiveSegment(SOCKET receiver, char** obyte_stream, int* ostream_len, int* oremain);

/// <summary>
/// Read segments from a connected socket and Merge them into a complete message.
/// </summary>
/// <param name="socket">The connected socket used to receive segments</param>
/// <param name="omessage">[Output] The merged message</param>
/// <returns>1 if success. 0 if cant read fully. -1 if have errors that the socket should be closed</returns>
int SegmentationReceive(SOCKET socket, char** omessage);

#pragma endregion

#pragma region Utilities

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
/// Convert value from Network Byte Order (BE) to Running Machine Byte Order.
/// </summary>
/// <param name="value">The value</param>
/// <returns>The converted value in Network Byte Order</returns>
ushort ToNetworkByteOrder(ushort value);

/// <summary>
/// Convert value from Network Byte Order (BE) to Running Machine Byte Order.
/// </summary>
/// <param name="value">The value</param>
/// <returns>The converted value in Network Byte Order</returns>
ulong ToNetworkByteOrder(ulong value);

/// <summary>
/// Convert value from Running Machine Byte Order to Network Byte Order (BE).
/// </summary>
/// <param name="value">The value</param>
/// <returns>The converted value in Running Machine Byte Order</returns>
ushort ToHostByteOrder(ushort value);

/// <summary>
/// Convert value from Running Machine Byte Order to Network Byte Order (BE).
/// </summary>
/// <param name="value">The value</param>
/// <returns>The converted value in Running Machine Byte Order</returns>
ulong ToHostByteOrder(ulong value);
#pragma endregion



#pragma endregion
