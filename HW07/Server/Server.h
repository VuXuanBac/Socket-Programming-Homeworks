#pragma once

#pragma region Header Declarations

#include <process.h>
#include "ApplicationLibrary.h"

#pragma endregion

#pragma region Constant Definitions

#define DEFAULT_TEMP_FOLDER "\\temp\\"

#define CS_FREE				0 // response complete and wait for another request
#define CS_RECEIVING		1 // receive file
#define CS_RESPONDING		2 // send response to client

#pragma endregion

#pragma region Type Definitions

typedef struct _client_info {

	SOCKETEX socketex; // Socket use for sending and receiving

	int request_type; // RT_ENCRYPT || RT_DECRYPT

	uint key; // encryption|decryption key

	uint temp_file_position; // The current file pointer int temp file (same as the successfully sent bytes)

	//int status; // See CS_ for some client status

	char* temp_file_path; // The path to the temp file.

} CLIENTINFO;

#pragma endregion

#pragma region Function Declarations

#pragma region Thread and Session

/// <summary>
/// Listen event signal from main() when new client be connected and start new session to this client.
/// Run this function on a separate thread from main() supports Completion Routine on Overlapped IO operations.
/// </summary>
/// <param name="arguments_wsaevent">A WSAEVENT object that will be invoked when new client be accepted from main()</param>
/// <returns>0 always.</returns>
unsigned __stdcall RunOverlappedIO(void* arguments_wsaevent);

/// <summary>
/// Create a Thread for running RunOverlappedIO() function.
/// Run RunOverlappedIO() function on a separate thread from main() supports Completion Routine on Overlapped IO operations.
/// </summary>
/// <param name="listener_event">A WSAEVENT object that will be invoked when new client be accepted from main()</param>
/// <returns>The HANDLE of the created thread</returns>
HANDLE CreateThread(WSAEVENT listener_event);

#pragma endregion


#pragma region Client Manager
/// <summary>
/// Get a pointer to CLIENTINFO object from a pointer to OVERLAPPED object.
/// [This is an utility function to retrieve CLIENTINFO* from OVERLAPPED* extracted from Completion Routine Callback.]
/// </summary>
/// <param name="socketex_overlapped">A pointer to a OVERLAPPED object belong to the CLIENTINFO object</param>
/// <returns>A pointer to the CLIENTINFO object contains the OVERLAPPED object</returns>
CLIENTINFO* GetClientInfo(OVERLAPPED* socketex_overlapped);

/// <summary>
/// Create a CLIENTINFO object (initialize default value for all fields) contains infos about a client identified by a SOCKET object.
/// </summary>
/// <param name="socket">The SOCKET object identifys the client</param>
/// <returns>The CLIENTINFO object</returns>
CLIENTINFO CreateClientInfo(SOCKET socket);

/// <summary>
/// Reset value for some fields in CLIENTINFO object before using it for new session.
/// </summary>
/// <param name="client">A pointer to the CLIENTINFO object</param>
void Reset(CLIENTINFO* client);

/// <summary>
/// Append new client (identified by a SOCKET object) to Application Client Manager.
/// </summary>
/// <param name="socket">The SOCKET object identify the client</param>
/// <returns>The index of new client in Application Client Manager. -1 if Application Client Manager full</returns>
int AppendSocketToManager(SOCKET socket);

/// <summary>
/// Remove client from Application Client Manager.
/// </summary>
/// <param name="client">A pointer to the client</param>
void RemoveClientFromManager(CLIENTINFO* client);
#pragma endregion

#pragma region Winsock Completion IO
/// <summary>
/// Completion Routine function called after a Overlapped IO operation completes.
/// [This function will be feed automatically by Winsock. Just assign it to the "callback" field in a SOCKETEX object]
/// </summary>
/// <param name="error">The errors occured during the Overlapped IO operation</param>
/// <param name="transfered_bytes">Number of bytes transfer successfully during Overlapped IO operation</param>
/// <param name="overlapped">The overlapped object manages the Overlapped IO operation</param>
/// <param name="flags">The flags return after Overlapped IO operation completes</param>
void CALLBACK RoutineCallback(DWORD error, DWORD transfered_bytes, LPWSAOVERLAPPED overlapped, DWORD flags);

/// <summary>
/// Invoke Overlapped IO to receive a ACK packet after sending response to client.
/// </summary>
/// <param name="client">The communicated client</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int ReceiveAckSendStatus(CLIENTINFO* client);

/// <summary>
/// Invoke Overlapped IO to receive a Segment Header from client
/// </summary>
/// <param name="client">The communicated client</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int ReceiveRequestHeader(CLIENTINFO* client);

/// <summary>
/// Invoke Overlapped IO to receive a Segment Content from client
/// </summary>
/// <param name="client">The communicated client</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. 0 if too much bytes. -1 if have fatal error that the socket should be closed</returns>
int ReceiveRequestContent(CLIENTINFO* client);

/// <summary>
/// Invoke Overlapped IO to send an ACK packet after receive a request from client.
/// </summary>
/// <param name="client">The communicated client</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int SendAckReceiveStatus(CLIENTINFO* client);

/// <summary>
/// Invoke Overlapped IO functions depends on current status of SOCKETEX object.
/// This function called by Completion Routine Callback (RoutineCallback()) after finishing an Overlapped IO operation.
/// </summary>
/// <param name="client">The communicated client</param>
/// <param name="sockex_status">Current status of the SOCKETEX object (Or type of already complete operation)</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine.-1 if have fatal error that the socket should be closed. otherwise return 0</returns>
int HandleIOResult(CLIENTINFO* client, int sockex_status);

#pragma endregion

#pragma region Handle Request

/// <summary>
/// Process Upload Request (Message Code = MC_DATA) from a client.
/// [This function only called by Request() after exatract info from a received MESSAGE object]
/// If the Request is Data End Request (payload = NULL), this function will invoke a Overlapped IO function from Respond().
/// </summary>
/// <param name="client">The client send request</param>
/// <param name="payload">The payload of the MESSAGE object. For MC_DATA Message, this may contains data or NULL</param>
/// <param name="payload_length">The size of the payload.</param>
/// <returns>1 or 99 if success [99 if this function invoke Respond()]. 0 if this function invoke Respond() and have errors on file. -1 if have fatal error that the socket should be closed.</returns>
int HandleDataRequest(CLIENTINFO* client, const stream payload, uint payload_length);

/// <summary>
/// Process Encrypt/Decrypt Request (Message Code = MC_ENCRYPT || MC_DECRYPT) from a client.
/// [This function only called by Request() after exatract info from a received MESSAGE object]
/// </summary>
/// <param name="client">The client send request</param>
/// <param name="request_type">The request type (Encrypt or Decrypt). See RT_ for some request types</param>
/// <param name="payload">The payload of the MESSAGE object. For MC_ECNRYPT||MC_DECRYPT Message, this contains the key of the request</param>
/// <returns>1 always</returns>
int HandleEncryptDecryptRequest(CLIENTINFO* client, int request_type, const stream payload);

/// <summary>
/// Handle a request from client after receive successfully a MESSAGE object.
/// This function may calls HandleEncryptDecryptRequest() or HandleDataRequest() depends on the message code.
/// </summary>
/// <param name="client">The client send request</param>
/// <returns>1 or 99 if success [99 if this function invoke a Overlapped IO operation]. 0 if receive invalid message from client. -1 if have fatal error that the socket should be closed</returns>
int Request(CLIENTINFO* client);

#pragma endregion

#pragma region Handle Response
/// <summary>
/// Encrypt/Decrypt data read from a file.
/// [This is a utility function only called from Respond() function to process data before sending response]
/// </summary>
/// <param name="request_type">RT_ENCRYPT or RT_DECRYPT</param>
/// <param name="key">The key used for shift cipher</param>
/// <param name="tempfp">A pointer to a file contains the data want to process. (Must move the file pointer before calling this function)</param>
/// <param name="oresult">[Output:NotNull] The encrypted/decrypted result data</param>
/// <param name="oresult_len">[Output:NotNull] The size in bytes of "oresult"</param>
/// <returns>1 if success. 0 if the file reach EOF. -1 if have some errors on file</returns>
int ProcessData(int request_type, int key, FILE* tempfp, stream* oresult, uint* oresult_len);

/// <summary>
/// Process data from temp file (contains data to encrypt/decrypt) and Send response to Client.
/// </summary>
/// <param name="client">The client will send response to</param>
/// <returns>1 or 99 if success [99 if this function invoke a Overlapped IO operation]. 0 if have errors on file. -1 if have fatal error that the socket should be closed</returns>
int Respond(CLIENTINFO* client);

#pragma endregion

#pragma endregion