#pragma once
#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations
#include <stdio.h>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "CommonDefinitions.h"
#pragma endregion

#pragma region Constant Definitions
#define RESPONSE_TITLE "IP Addresses:\n"
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
/// INVALID_SOCKET if Protocol is unexpected or have error on Winsock
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
/// Extract infomation in Message object and Print the message to console.
/// </summary>
/// <param name="message">The response Message object</param>
/// <param name="title">The title for message, which is shown previous. NULL for not use</param>
/// <returns>1 if has another response after this message, 0 otherwise</returns>
int PrintResponse(const MESSAGE message, const char* title = NULL);

/// <summary>
/// Extract message from server and show it to console.
/// </summary>
/// <param name="socket">The socket to receive message</param>
/// <param name="sender">The expected sender</param>
void HandleResponse(SOCKET socket, ADDRESS sender);

/// <summary>
/// Receive a message from a UDP socket buffer.
/// </summary>
/// <param name="receiver">The receiver socket</param>
/// <param name="omessage">[Output] The message extracted from datagram</param>
/// <param name="osender_addr">[Output] The sender's address extracted from datagram</param>
/// <returns>1 if have no errors. 0 otherwise</returns>
int Receive(SOCKET receiver, char** omessage, ADDRESS* osender_addr);

/// <summary>
/// Send a Message to a Address
/// </summary>
/// <param name="sender">The sender socket</param>
/// <param name="message">The message want to send</param>
/// <param name="receiver">The receiver's address</param>
/// <param name="byte_sent">[Output] Number of bytes are sent successfully.</param>
/// <returns>1 if have no errors. 0 otherwise</returns>
int Send(SOCKET sender, const char* message, ADDRESS receiver, int* obyte_sent = NULL);

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
#pragma endregion
