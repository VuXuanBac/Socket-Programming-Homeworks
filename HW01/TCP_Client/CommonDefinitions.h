#pragma once
#pragma region Constants Definitions

#define RECEIVE_TIMEOUT_INTERVAL 10000
#define APPLICATION_BUFF_MAX_SIZE 1024
#define USER_INPUT_MAX_SIZE 1023
#define MESSAGE_MAX_SIZE 1023

#define DEFAULT_PORT 5555
#define DEFAULT_IP "127.0.0.1"

#define UDP 0
#define TCP 1

#define CLOSE_NORMAL 0
#define CLOSE_SAFELY 1

#define STATUS_OK 1
#define STATUS_ERROR 0
#define STATUS_OK_END 2

#define STATUS_OK_CHAR '+'
#define STATUS_ERROR_CHAR '-'
#define STATUS_OK_END_CHAR '0'

#pragma endregion

#pragma region Type Definitions

#define ADDRESS SOCKADDR_IN
#define IP IN_ADDR
#define MESSAGE char*

#pragma endregion

#pragma region Error Debugging

#define INFO_FLAGS "INF"
#define ERROR_FLAGS "ERR"
#define WARNING_FLAGS "WAR"
#define USER_INPUT_FLAGS ">>"

#define _NOT_SPECIFY_PORT "Port number is not specified. Default port used!"
#define _CONVERT_PORT_FAIL "Fail to convert port number from command-line. Default port used!"
#define _CONVERT_ARGUMENTS_FAIL "Fail to extract port number and ip address from command-line arguments."

#define _ALLOCATE_MEMORY_FAIL "Fail to allocate memory."

#define _INITIALIZE_FAIL "Fail to initialize Winsock 2.2!"
#define _BIND_SOCKET_FAIL "Fail to bind socket with the address."
#define _CREATE_SOCKET_FAIL "Fail to create a socket."
#define _SHUTDOWN_SOCKET_FAIL "Fail to shutdown the socket."
#define _CLOSE_SOCKET_FAIL "Fail to close the socket."
#define _SET_TIMEOUT_FAIL "Fail to set receive timeout for socket."
#define _RECEIVE_FAIL "Fail to receive message from remote process."
#define _SEND_FAIL "Fail to send message to the remote process."
#define _LISTEN_SOCKET_FAIL "Fail to set socket to listen state."
#define _ACCEPT_SOCKET_FAIL "Fail to accept a connection with the socket."

#define _TRANSLATE_DOMAIN_FAIL "Fail to translate the domain name."
#define _TRANSLATE_IP_FAIL "Fail to translate the IP address."

#define _ADDRESS_IN_USE "Address in Use. \"Another process already bound to the address,\""
#define _BOUNDED_SOCKET "Invalid Socket. \"The socket already bound to another address.\""
#define _HOST_UNREACHABLE "Host Unreachable. \"The remote process is unreachable at this time.\""
#define _MESSAGE_TOO_LARGE "Message Too Large. \"The buffer size is not large enough! Some data from remote process lost,\""
#define _MESSAGE_EXTREME_LARGE "Message Too Large. \"The message size is larger than the maximum supported by the underlying transport.\""
#define _SEND_NOT_ALL "Not all bytes was sent."

#define _RECEIVE_UNEXPECTED_MESSAGE "Receive an invalid message."
#define _CONNECTION_DROP "Connection to the remote process has been drop."
#define _TOO_MUCH_BYTES "The number of bytes read is higher than the application buffer size."
#define _NOT_BOUND_SOCKET "Invalid Socket. \"The socket need to be bound to an address.\""
#define _NOT_LISTEN_SOCKET "Invalid Socket. \"The socket need to be set to listen state.\""
#define _REACH_SOCKETS_LIMIT "Too many open sockets"

#define _CONNECTION_REFUSED "Connection refused. \"Remote process refused to establish connection. Try again later.\""
#define _ESTABLISH_CONNECTION_TIMEOUT "Establish connection to remote process timeout. No connection established."
#define _HAS_CONNECTED "There is another connection established before on this socket."
#define _ESTABLISH_CONNECTION_FAIL "Fail to establish connection to the address."
#define _CONNECTION_DROP "Connection to the remote process has been drop."
#pragma endregion
