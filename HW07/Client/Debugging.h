#pragma once

#pragma region Constants Definitions

#define _ERROR_DEBUGGING

#define WAIT			99
#define SUCCESS				1
#define FAIL				0
#define FATAL_ERROR			-1
#define INVALID_ARGUMENTS	-2

#define INFO_FLAGS			"INF"
#define ERROR_FLAGS			"ERR"
#define WARNING_FLAGS		"WAR"

#define _NOT_SPECIFY_PORT			"Port number is not specified. Default port used!"
#define _CONVERT_PORT_FAIL			"Fail to convert port number from command-line. Default port used!"
#define _CONVERT_ARGUMENTS_FAIL		"Fail to extract port number and ip address from command-line arguments."

#define _ALLOCATE_MEMORY_FAIL		"Fail to allocate memory."
#define _INVALID_PARAMETER			"Some parameters is not valid."

#define _INITIALIZE_FAIL			"Fail to initialize Winsock 2.2!"
#define _BIND_SOCKET_FAIL			"Fail to bind socket with the address."
#define _CREATE_SOCKET_FAIL			"Fail to create a socket."
#define _SHUTDOWN_SOCKET_FAIL		"Fail to shutdown the socket."
#define _CLOSE_SOCKET_FAIL			"Fail to close the socket."
#define _SET_TIMEOUT_FAIL			"Fail to set receive timeout for socket."
#define _SET_BUFFER_SIZE_FAIL		"Fail to set buffer size for socket"
#define _RECEIVE_FAIL				"Fail to receive message from remote process."
#define _SEND_FAIL					"Fail to send message to remote process."
#define _LISTEN_SOCKET_FAIL			"Fail to set socket to listen state."
#define _ACCEPT_SOCKET_FAIL			"Fail to accept a connection with the socket."
#define _ESTABLISH_CONNECTION_FAIL	"Fail to establish connection to the address."
#define _GET_OVERLAPPED_RESULT_FAIL	"Fail to get overlapped IO result."
#define _ATTACH_NOTIFICATION_FAIL	"Fail to attach a notification message for the socket."
#define _ATTACH_EVENT_FAIL			"Fail to attach a receive event for the socket."
#define _LISTEN_EVENTS_FAIL			"Fail to listen on sockets' events."

#define _INVALID_EVENT				"The attached event handle is invalid"

#define _ADDRESS_IN_USE				"Address in Use. \"Another process already bound to the address,\""
#define _BOUNDED_SOCKET				"Invalid Socket. \"The socket already bound to another address.\""
#define _HOST_UNREACHABLE			"Host Unreachable. \"The remote process is unreachable at this time.\""
#define _CONNECTION_DROP			"Connection to the remote process has been drop."
#define _CONNECTION_REFUSED			"Remote process refused to establish connection. Try again later."
#define _ESTABLISH_CONNECTION_TIMEOUT "Establish connection to remote process timeout. No connection established."
#define _HAS_CONNECTED				"There is another connection established before on this socket."

#define _NOT_BOUND_SOCKET			"Invalid Socket. \"The socket need to be bound to an address.\""
#define _NOT_LISTEN_SOCKET			"Invalid Socket. \"The socket need to be set to listen state.\""

#define _MESSAGE_TOO_LARGE			"Message Too Large. \"The buffer size is not large enough! Some data from remote process lost,\""
#define _MESSAGE_EXTREME_LARGE		"Message Too Large. \"The message size is larger than the maximum supported by the underlying transport.\""
#define _SEND_NOT_ALL				"Not all bytes was sent."
#define _RECEIVE_UNEXPECTED_SEGMENT	"Receive unexpected segment."
#define _TOO_MUCH_BYTES				"The number of bytes required is too much"

#define _TOO_MANY_SOCKETS			"Too many open sockets."
#define _TOO_MANY_CLIENTS			"Too many clients."
#define _TOO_MANY_THREADS			"Too many threads are running. Can not create one more thread."
#define _INSUFFICIENT_RESOURCES		"Insufficient resources for creating one more thread."

#define _OVERLAPPED_IO_INCOMPLETE	"Overlapped IO operation does not complete."
#define _OVERLAPPED_RECEIVE_RETURN_IMMEDIATELY "Unexpected result when overlapped receive return immediately."
#pragma endregion
