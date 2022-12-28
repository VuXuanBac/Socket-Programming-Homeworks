#pragma once

#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations

#include "Debugging.h"
#include "Utilities.h"

#include <WinSock2.h>
#include <WS2tcpip.h>

#ifdef _ERROR_DEBUGGING
#include <stdio.h>
#endif

#pragma endregion

#pragma region Constants Definitions

#define MAX_CONNECTIONS SOMAXCONN

#define MESSAGE_MAX_SIZE		(10 * 1024 + 5)

#define SEGMENT_HEADER_SIZE		4
#define SEGMENT_MAX_SIZE		(MESSAGE_MAX_SIZE + SEGMENT_HEADER_SIZE)

#define UDP						0
#define TCP						1

#define CLOSE_NORMAL			0
#define CLOSE_SAFELY			1

#define SS_FREE					0 // wait for another request
#define SS_RECH					1 // receive segment header
#define SS_RECC					2 // receive message in segment
#define SS_RECA					4 // receive ack
#define SS_SEND					8 // send
#define SS_SENA					16 // send ack
#pragma endregion

#pragma region Type Definitions

#define ADDRESS					SOCKADDR_IN
#define IP						IN_ADDR
#define OCRCALLBACK				LPWSAOVERLAPPED_COMPLETION_ROUTINE

typedef struct socketex {

	WSAOVERLAPPED overlapped; // The Overlapped object to handle receive/send

	SOCKET socket; // The socket use for communication

	WSABUF buffer; // Buffer object for storing data while receiving/sending

	stream data; // The data want to send or expect to receive.

	uint expected_transfer; // Expected transfer (send/receive) bytes

	OCRCALLBACK callback; // Overlapped Completion Routine Callback, called after a IO operation completes

	int status; // Current operation that the SOCKETEX object is working. See SS_ for some status

}SOCKETEX;

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

/// <summary>
/// Attach a WSAEVENT for a socket
/// </summary>
/// <param name="socket">The socket want to attach an event</param>
/// <param name="socket_event">The event used for the socket</param>
/// <param name="listen_event">The event type that the socket will wait on. See FD_ for some event type</param>
/// <returns>1 if success. 0 otherwise</returns>
int AttachEventForSocket(SOCKET socket, WSAEVENT socket_event, long listen_event = (FD_READ | FD_CLOSE));

/// <summary>
/// Listen a set of events that change from unsignaled state to signaled state
/// </summary>
/// <param name="events">A set of events</param>
/// <param name="count">Number of events in the set. Note that events from 0 to count-1 must be a valid event, otherwise an error will be throwed</param>
/// <param name="is_alertable">1 if want place current thread into alertable state (can execute completion routine). 0 otherwise</param>
/// <param name="time_wait">The waiting time for listenning</param>
/// <returns>-1 if have errors or time up. 99 if wait on completion routine. otherwise return the index of the first event that change state [0, count-1]</returns>
int ListenEvents(WSAEVENT* events, int count, int is_alertable = 1, int time_wait = WSA_INFINITE);

/// <summary>
/// Signal (Set) a Event object
/// </summary>
/// <param name="socket_event">The event object</param>
void SignalEvent(WSAEVENT socket_event);

/// <summary>
/// Unsignal (Reset) a Event object
/// </summary>
/// <param name="socket_event">The event object</param>
void UnSignalEvent(WSAEVENT socket_event);

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
/// Send a byte stream to the remote machine connected by a socket.
/// </summary>
/// <param name="sender">The connected socket to the remote machine</param>
/// <param name="send_until_succ">1 if want to resend unsuccessful bytes. 0 otherwise</param>
/// <param name="bytes">The number of bytes want to send</param>
/// <param name="byte_stream">The byte stream want to send</param>
/// <param name="obyte_sent">[Output] Number of bytes sent successfully</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected [Never if send_until_succ=1]. -1 if have some fatal errors that the socket should be closed</returns>
int Send(SOCKET sender, int send_until_succ, uint bytes, const stream byte_stream, uint* obyte_sent = NULL);


/// <summary>
/// Create a Segment from Message (stream bytes) and Send it using a connected socket to a remote machine
/// Segment = Header (SEGMENT_HEADER_SIZE) | Message (Size indicated by Header)
/// </summary>
/// <param name="sender">The connected socket to the remote machine</param>
/// <param name="send_until_succ">1 if want to resend unsuccessful bytes. 0 otherwise</param>
/// <param name="message">The message want to send</param>
/// <param name="message_len">The message size in bytes</param>
/// <param name="obyte_sent">[Output] The number of bytes sent successfully.</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected [Never if send_until_succ=1]. -1 if have some fatal errors that the socket should be closed</returns>
int SendSegment(SOCKET sender, int send_until_succ, const stream message, uint message_len, uint* obyte_sent = NULL);

/// <summary>
/// Receive a byte stream from a connected socket buffer.
/// </summary>
/// <param name="receiver">The connected socket to the remote machine</param>
/// <param name="recv_until_succ">1 if want to try to read all expected bytes from socket buffer. 0 otherwise</param>
/// <param name="bytes">Number of bytes expected to receive. Not exceed SEGMENT_MAX_SIZE</param>
/// <param name="obyte_stream">[Output:NotNull] The read byte stream. Not null if success</param>
/// <param name="obyte_read">[Output] Number of bytes read successfully</param>
/// <returns>1 if success. 0 if number of bytes receive less than expected [Never if recv_until_succ=1]. -1 if have some fatal errors that the socket should be closed</returns>
int Receive(SOCKET receiver, int recv_until_succ, uint bytes, stream* obyte_stream, uint* obyte_read = NULL);

/// <summary>
/// Receive a Segment from a connected socket. 
/// Segment = Header (SEGMENT_HEADER_SIZE) | Message (Size indicated by Header)
/// </summary>
/// <param name="receiver">The connected socket to the remote machine</param>
/// <param name="recv_until_succ">1 if want to try to read all expected bytes from socket buffer. 0 otherwise</param>
/// <param name="omessage">[Output:NotNull] The message piece in the read segment</param>
/// <param name="omessage_len">[Output:NotNull] The size of the message in bytes</param>
/// <param name="obyte_read">[Output] Number of bytes read successfully. Useful only if recv_until_succ = 0</param>
/// <returns>1 if success. 0 if number of bytes receive less than expected [Never if recv_until_succ=1]. -1 if have some fatal errors that the socket should be closed</returns>
int ReceiveSegment(SOCKET receiver, int recv_until_succ, stream* omessage, uint* omessage_len, uint* obyte_read = NULL);

#pragma endregion

#pragma region Send and Receive Overlapped

/// <summary>
/// [Overlapped] Send data contains in "buffer" field in a SOCKETEX object
/// </summary>
/// <param name="sender">A pointer to SOCKETEX object</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int Send(SOCKETEX* sender);

/// <summary>
/// [Overlapped] Create a Segment object and send it with a SOCKETEX object
/// </summary>
/// <param name="sender">A pointer to SOCKETEX object</param>
/// <param name="message">The segment content (the message)</param>
/// <param name="message_len">The size of the message</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. 0 if too much bytes. -1 if have fatal error that the socket should be closed</returns>
int SendSegment(SOCKETEX* sender, const stream message, uint message_len);

/// <summary>
/// [Overlapped] Continue sending remain bytes in SOCKET buffer.
/// This function should be invoked after invoking SendSegment() and number of bytes sent successfully less than number of bytes expected to send
/// </summary>
/// <param name="receiver">A pointer to SOCKETEX object used for sending</param>
/// <param name="bytes">Number of bytes want to send</param>
/// <param name="sent_success">Number of bytes sent successfully.</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int ContinueSend(SOCKETEX* sender, uint bytes, uint sent_success);

/// <summary>
/// [Overlapped] Receive data into the "buffer" field in a SOCKETEX object
/// </summary>
/// <param name="receiver">A pointer to SOCKETEX object</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int Receive(SOCKETEX* receiver);

/// <summary>
/// [Overlapped] Continue receive remain bytes from SOCKET buffer.
/// This function should be invoked after invoking ReceiveSegmentHeader() or ReceiveSegmentContent() and number of bytes received successfully less than number of bytes expected to receive.
/// </summary>
/// <param name="receiver">A pointer to SOCKETEX object used for receiving</param>
/// <param name="bytes">Number of bytes want to receive</param>
/// <param name="received_success">Number of bytes received successfully.</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int ContinueReceive(SOCKETEX* receiver, uint bytes, uint received_success);

/// <summary>
/// Get overlapped result after a Overlapped IO operation complete on a SOCKETEX object
/// </summary>
/// <param name="sockex">A pointer to the SOCKETEX object</param>
/// <param name="obytes">[Output] Number of bytes transfer successfully</param>
/// <param name="oflags">[Output] The flags return after the IO operation completes</param>
/// <returns>1 if success. -1 if have fatal errors that the socket should be closed</returns>
int GetOverlappedResult(SOCKETEX* sockex, uint* obytes, uint* oflags);

/// <summary>
/// [Overlapped] Receive a Segment Header from a SOCKETEX object.
/// This is a utility function for retrieving Segment Header before retrieving Segment Content. See ReceiveSegmentContent()
/// </summary>
/// <param name="receiver">A pointer to the SOCKETEX object</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. -1 if have fatal error that the socket should be closed</returns>
int ReceiveSegmentHeader(SOCKETEX* receiver);

/// <summary>
/// [Overlapped] Receive a Segment Content (The Message) from a SOCKETEX object.
/// This is a utility function for retrieving Segment Content and should be call after invoking ReceiveSegmentHeader() function.
/// </summary>
/// <param name="receiver">A pointer to the SOCKETEX object</param>
/// <returns>1 if finish immediately. 99 if wait on completion routine. 0 if too much bytes. -1 if have fatal error that the socket should be closed</returns>
int ReceiveSegmentContent(SOCKETEX* receiver, uint bytes);

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
/// Set new size for a Socket Send Buffer
/// </summary>
/// <param name="socket">The socket want to config</param>
/// <param name="size">The size in bytes want to set. Should be a multiple of 4096</param>
/// <returns>1 if set success. 0 otherwise</returns>
int SetSendBufferSize(SOCKET socket, uint size);

/// <summary>
/// Set new size for a Socket Receive Buffer
/// </summary>
/// <param name="socket">The socket want to config</param>
/// <param name="size">The size in bytes want to set. Should be a multiple of 4096</param>
/// <returns>1 if set success. 0 otherwise</returns>
int SetReceiveBufferSize(SOCKET socket, uint size);

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
uint ToNetworkByteOrder(uint value);

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
uint ToHostByteOrder(uint value);

#pragma endregion

#pragma region Socket Extend
/// <summary>
/// Free memory for SOCKETEX object
/// </summary>
/// <param name="sockex"></param>
void DestroySocketExtend(SOCKETEX* sockex);

/// <summary>
/// Prepare "buffer" for SOCKETEX object before receiving or sending
/// </summary>
/// <param name="sockex">A pointer to the SOCKETEX object</param>
/// <param name="bytes">The size of the buffer. It the number of bytes want to send or expect to receive</param>
/// <param name="start_byte_in_data">The start byte in data that the "buf" field will point to.</param>
void PrepareBuffer(SOCKETEX* sockex, uint bytes, uint start_byte_in_data = 0);

/// <summary>
/// Update status for SOCKETEX object
/// </summary>
/// <param name="sockex">The SOCKETEX object want to update status</param>
/// <param name="status">New status. See SS_ for some socket extend status</param>
void UpdateStatus(SOCKETEX* sockex, int status);

/// <summary>
/// Create a SOCKETEX object
/// </summary>
/// <param name="socket">The socket fill the "socket" field.</param>
/// <param name="buffer_size">The initialize buffer size for "buffer" field, 
/// "len" field in buffer will 0 but the "buf" field will be initialized with buffer_size bytes.
/// Most overlapped function will not free the memory allocated for buffer in this function, just change "len" field</param>
/// <param name="callback">The completion routine callback fill "callback" field</param>
/// <returns>Created SOCKETEX object</returns>
SOCKETEX CreateSocketExtend(SOCKET socket, uint buffer_size, OCRCALLBACK callback);

/// <summary>
/// Reset default values for some fields in SOCKETEX object. [Call before starting new session]
/// "socket", "callback" fields will not change.
/// </summary>
/// <param name="sockex">A pointer to the SOCKETEX want to reset</param>
void Reset(SOCKETEX* sockex);

/// <summary>
/// Create a Segment object.
/// Segment = Segment Header (Content len) [SEGMENT_HEADER_SIZE bytes] | Message (Content)
/// </summary>
/// <param name="message">The message (segment content)</param>
/// <param name="message_len">The size in bytes of the message. Not exceed MESSAGE_MAX_SIZE</param>
/// <param name="osegment">[Output:NotNull] Created segment. Remember to allocate memory for this field before passing it to the function</param>
/// <param name="osegment_len">[Output:NotNull] The created segment's size</param>
/// <returns>1 if success. 0 if fail message_len exceed MESSAGE_MAX_SIZE</returns>
int CreateSegment(const stream message, uint message_len, stream* osegment, uint* osegment_len);

#pragma endregion

#pragma endregion
