#pragma once

#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations

#include <stdio.h>
#include <stdlib.h>

#include "Debugging.h"
#include "Utilities.h"
#include "SocketLibrary.h"
#include "Crypto.h"

#pragma endregion

#pragma region Constants Definitions

#define MESSAGE_HEADER_CODE_SIZE	1
#define MESSAGE_HEADER_LENGTH_SIZE	4
#define MESSAGE_HEADER_SIZE			(MESSAGE_HEADER_CODE_SIZE + MESSAGE_HEADER_LENGTH_SIZE)
#define MESSAGE_PAYLOAD_MAX_SIZE	(MESSAGE_MAX_SIZE - MESSAGE_HEADER_SIZE)

#define ACK_PACKET_SIZE				4

#define RECEIVE_TIMEOUT_INTERVAL	10000
#define USER_INPUT_MAX_SIZE			1023
#define MAX_CLIENTS					777

#define DEFAULT_PORT				6600
#define DEFAULT_IP					"127.0.0.1"

#define MC_ENCRYPT					0
#define MC_DECRYPT					1
#define MC_DATA						2
#define MC_ERROR					3
#define MC_INVALID					-1

#define RT_ENCRYPT					0
#define RT_DECRYPT					1
#define RT_INVALID					2

#define FILE_EXTENSION_SIZE			5
#define ENCRYPT_FILE_EXTENSION		".enc"
#define DECRYPT_FILE_EXTENSION		".dec"

#define NULLSTR						""
#pragma endregion

#pragma region Type Definitions

#define MESSAGE						char*

#pragma endregion

#pragma region Function Declarations

/// <summary>
/// Create a MESSAGE object: Code (1 byte) | Length (4 byte) | Payload (Size depend on "Length")
/// [Version bytestream-payload]
/// </summary>
/// <param name="code">The code for the message. See MC_ for some codes</param>
/// <param name="byte_stream">The payload data</param>
/// <param name="length">The length of the payload</param>
/// <param name="omessage_len">[Output:NotNull] The length of the created message</param>
/// <returns>The created MESSAGE. NULL if fail to allocate memory</returns>
MESSAGE CreateMessage(int code, const stream byte_stream, uint length, uint* omessage_len);

/// <summary>
/// Create a MESSAGE object: Code (1 byte) | Length (4 byte) | Payload (Size depend on "Length")
/// [Version integer-payload]
/// </summary>
/// <param name="code">The code for the message. See MC_ for some codes</param>
/// <param name="value">The payload data</param>
/// <param name="omessage_len">[Output:NotNull] The length of the created message</param>
/// <returns>The created MESSAGE. NULL if fail to allocate memory</returns>
MESSAGE CreateMessage(int code, uint value, uint* omessage_len);

/// <summary>
/// Extract data (Payload) from MESSAGE object.
/// </summary>
/// <param name="message">The MESSAGE object</param>
/// <param name="message_len">The MESSAGE's size in bytes</param>
/// <param name="opayload">[Output:NotNull] The payload data</param>
/// <param name="olength">[Output:NotNull] The payload size in bytes</param>
/// <returns>The MESSAGE's code. See MC_ for some message's code</returns>
int ExtractMessage(const MESSAGE message, uint message_len, stream* opayload, uint* olength);

/// <summary>
/// Free memory for the MESSAGE object
/// </summary>
/// <param name="m">The MESSAGE object</param>
void DestroyMessage(MESSAGE m);

/// <summary>
/// Create a Encrypt/Decrypt MESSAGE object and Send it to the remoted machine [Block]
/// Message code = MC_ENCRYPT or MC_DECRYPT
/// </summary>
/// <param name="sender">The socket used for sending the request</param>
/// <param name="request_type">The request type from user. See RT_ for some requests type</param>
/// <param name="key">The key (from user) used in encrypt/decrypt shift cipher</param>
/// <returns>1 if success. 0 if send fail or allocate memory fail. -1 if have fatal error that the socket should be closed</returns>
int SendEncryptDecryptMessage(SOCKET sender, int request_type, int key);

/// <summary>
/// Create a Data MESSAGE object and Send it to the remoted machine [Block]
/// Message code = MC_DATA
/// </summary>
/// <param name="sender">The socket used for sending the request</param>
/// <param name="content">The data (payload) of the message. Use NULLSTR if want to create a Upload End message</param>
/// <param name="content_len">The size of payload. Use 0 if want to create a Upload End message</param>
/// <returns>1 if success. 0 if send fail or allocate memory fail. -1 if have fatal error that the socket should be closed</returns>
int SendDataMessage(SOCKET sender, const stream content, uint content_len);

/// <summary>
/// Receive a MESSAGE object and Extract information from it [Block[
/// </summary>
/// <param name="receiver">The connected socket to receive</param>
/// <param name="ocode">[Output:NotNull] The message code</param>
/// <param name="opayload">[Output:NotNull] The message payload</param>
/// <param name="olength">[Output:NotNull] The message payload length</param>
/// <returns>1 if success. 0 if the message is invalid. -1 if have fatal error that the socket should be closed</returns>
int ReceiveMessage(SOCKET receiver, int* ocode, stream* opayload, uint* olength);

/// <summary>
/// Receive a ACK Packet (A Segment with Header = 0 (0x 0000 0000)) from SOCKET
/// </summary>
/// <param name="receiver">The connected socket used for receiving</param>
/// <returns>1 if success. 0 if number of bytes receive less than expected [Never if recv_until_succ=1]. -1 if have some fatal errors that the socket should be closed</returns>
int ReceiveACK(SOCKET receiver);

/// <summary>
/// Create a ACK Packet (A Segment with Header = 0 (0x 0000 0000)) and Send them using SOCKET
/// </summary>
/// <param name="sender">The connected socket used for sending</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected [Never if send_until_succ=1]. -1 if have some fatal errors that the socket should be closed</returns>
int SendACK(SOCKET sender);

/// <summary>
/// Create a Data MESSAGE object and Send it to the remoted machine [Overlapped]
/// Message code = MC_DATA
/// </summary>
/// <param name="sender">The socket extend used for sending the request</param>
/// <param name="content">The data (payload) of the message. Use NULLSTR if want to create a Upload End message</param>
/// <param name="content_len">The size of payload. Use 0 if want to create a Upload End message</param>
/// <returns>1 if success [send right away]. 99 if will send in the future. 0 if send fail or allocate memory fail. -1 if have fatal error that the socket should be closed</returns>
int SendDataMessage(SOCKETEX* sender, const stream content, uint content_len);

/// <summary>
/// [Overlapped] Create a ACK Packet (A Segment with Header = 0 (0x 0000 0000)) and Send them using SOCKETEX
/// </summary>
/// <param name="sender">The socket extend used for sending the ACK Packet</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int SendACK(SOCKETEX* sender);

/// <summary>
/// [Overlapped] Receive a ACK Packet (A Segment with Header = 0 (0x 0000 0000)) from SOCKETEX
/// </summary>
/// <param name="receiver">The socket extend used for receving the ACK Packet</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int ReceiveACK(SOCKETEX* receiver);

#pragma endregion
