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

#define ERROR_MESSAGE "Not found infomation"

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
/// Receive a message from an UDP socket buffer.
/// </summary>
/// <param name="receiver">The receiver socket</param>
/// <param name="omessage">[Output] The message extracted from datagram</param>
/// <param name="osender_addr">[Output] The sender's address in the datagram</param>
/// <returns>1 if have no errors. 0 otherwise</returns>
int Receive(SOCKET receiver, char** omessage, ADDRESS* osender_addr);

/// <summary>
/// Send a message to an address
/// </summary>
/// <param name="sender">The sender socket</param>
/// <param name="message">The message want to send</param>
/// <param name="receiver">The receiver's address</param>
/// <param name="byte_sent">[Output] Number of bytes are sent successfully.</param>
/// <returns>1 if have no errors. 0 otherwise</returns>
int Send(SOCKET sender, const char* message, ADDRESS receiver, int* obyte_sent = NULL);

/// <summary>
/// Translate a Domain Name to IPv4 Addresses.
/// </summary>
/// <param name="name">The domain name</param>
/// <param name="oinfos">[Output] Addresses' Info</param>
/// <returns>1 if translate successfully. 0 if has error</returns>
int TranslateDomainName(const char* name, ADDRINFO** oinfos);

/// <summary>
/// Handle request from Client that is a Domain Name String.
/// [Translate the domain name to IPv4 Addresses]
/// </summary>
/// <param name="name">The domain name want to translate</param>
/// <param name="sender">The socket used to send result to client</param>
/// <param name="receiver">The client's address</param>
void HandleDomainNameRequest(const char* name, SOCKET sender, ADDRESS receiver);

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
/// Try parse a string to a IPv4 Address
/// </summary>
/// <param name="str">The string want to parse</param>
/// <param name="oip">[Output] The result IPv4 Address</param>
/// <returns>1 if parse successfully. 0 otherwise</returns>
int TryParseIPString(const char* str, IP* oip);

/// <summary>
/// Extract IPv4 Address from Socket Address and convert to a string
/// </summary>
/// <param name="addr">The Socket Address</param>
/// <returns>IPv4 in string</returns>
char* GetIPString(ADDRESS addr);

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