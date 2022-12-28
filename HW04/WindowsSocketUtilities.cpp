#include "WindowsSocketUtilities.h"

#pragma region Socket Common

int WSInitialize()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA wsa_data;
	if (WSAStartup(version, &wsa_data)) {
#ifdef _ERROR_DEBUGGING
		printf("[%s] %s\n", ERROR_FLAGS, _INITIALIZE_FAIL);
#endif // _ERROR_DEBUGGING
		WSACleanup();
		return 0;
	}
	return 1;
}

int WSCleanup()
{
	return WSACleanup();
}

ADDRESS CreateSocketAddress(IP ip, int port)
{
	ADDRESS addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = ip;
	return addr;
}

SOCKET CreateSocket(int protocol)
{
	SOCKET s = INVALID_SOCKET;
	if (protocol == UDP) {
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	else if (protocol == TCP) {
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

#ifdef _ERROR_DEBUGGING
	if (s == INVALID_SOCKET) {
		printf("[%s] %s\n", ERROR_FLAGS, _CREATE_SOCKET_FAIL);
	}
#endif // _ERROR_DEBUGGING

	return s;
}

int CloseSocket(SOCKET socket, int mode, int flags)
{
	if (socket == INVALID_SOCKET)
		return 1;
	int is_ok = 1;
	if (mode == CLOSE_SAFELY) {
		if (shutdown(socket, flags) == SOCKET_ERROR) {
			is_ok = 0;
#ifdef _ERROR_DEBUGGING
			printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SHUTDOWN_SOCKET_FAIL);
#endif // _ERROR_DEBUGGING

		}
	}
	if (closesocket(socket) == SOCKET_ERROR) {
		is_ok = 0;
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _CLOSE_SOCKET_FAIL);
#endif // _ERROR_DEBUGGING
	}
	return is_ok;
}

int BindSocket(SOCKET socket, ADDRESS addr)
{
	if (bind(socket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		int err = WSAGetLastError();
		if (err == WSAEADDRINUSE) {
			printf("[%s:%d] %s\"\n", ERROR_FLAGS, err, _ADDRESS_IN_USE);
		}
		else if (err == WSAEINVAL) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _BOUNDED_SOCKET);
		}
		else {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _BIND_SOCKET_FAIL);
		}
#endif // _ERROR_DEBUGGING
		return 0;
	}
	return 1;
}

int SetListenState(SOCKET socket, int connection_numbers)
{
	int ret = listen(socket, connection_numbers);
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		int err = WSAGetLastError();
		if (err == WSAEINVAL) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _NOT_BOUND_SOCKET);
		}
		else if (err == WSAEMFILE) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _REACH_SOCKETS_LIMIT);
		}
		else {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _LISTEN_SOCKET_FAIL);
		}
#endif // _ERROR_DEBUGGING
		return 0;
	}
	return 1;
}

SOCKET GetConnectionSocket(SOCKET listener, ADDRESS* osender_address)
{
	int sender_addr_len = sizeof(SOCKADDR_IN);
	int* addr_len = osender_address == NULL ? NULL : &sender_addr_len;

	SOCKET result = accept(listener, (SOCKADDR*)osender_address, addr_len);

#ifdef _ERROR_DEBUGGING
	if (result == INVALID_SOCKET) {
		int err = WSAGetLastError();
		if (err == WSAEINVAL) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _NOT_LISTEN_SOCKET);
		}
		else {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _ACCEPT_SOCKET_FAIL);
		}
	}
#endif // _ERROR_DEBUGGING

	return result;
}

int EstablishConnection(SOCKET socket, ADDRESS address)
{
	int ret = connect(socket, (SOCKADDR*)&address, sizeof(address));
	if (ret == SOCKET_ERROR) {

#ifdef _ERROR_DEBUGGING
		int err = WSAGetLastError();
		if (err == WSAECONNREFUSED) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _CONNECTION_REFUSED);
		}
		else if (err == WSAEHOSTUNREACH) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HOST_UNREACHABLE);
		}
		else if (err == WSAETIMEDOUT) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _ESTABLISH_CONNECTION_TIMEOUT);
		}
		else if (err == WSAEISCONN) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HAS_CONNECTED);
		}
		else {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _ESTABLISH_CONNECTION_FAIL);
		}
#endif // _ERROR_DEBUGGING

		return 0;
	}
	return 1;
}

IP CreateDefaultIP()
{
	IP addr;
	addr.s_addr = htonl(INADDR_ANY);
	return addr;
}

#pragma endregion

#pragma region Utilities

int SetReceiveTimeout(SOCKET socket, int interval)
{
	int _interval = interval;
	int ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&_interval, sizeof(_interval));
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SET_TIMEOUT_FAIL);
#endif
		return 0;
	}
	return 1;
}

int TryParseIPString(const char* str, IP* oip)
{
	return inet_pton(AF_INET, str, oip) == 1;
}

ushort ToNetworkByteOrder(ushort value)
{
	return htons(value);
}

ulong ToNetworkByteOrder(ulong value)
{
	return htonl(value);
}

ushort ToHostByteOrder(ushort value)
{
	return ntohs(value);
}

ulong ToHostByteOrder(ulong value)
{
	return ntohl(value);
}

#pragma endregion


#pragma region Send and Receive

int Send(SOCKET sender, int bytes, const char* byte_stream)
{
	int ret = send(sender, byte_stream, bytes, 0);
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		int err = WSAGetLastError();
		if (err == WSAEHOSTUNREACH) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HOST_UNREACHABLE);
		}
		else if (err == WSAECONNABORTED || err == WSAECONNRESET) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
		}
		else {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _SEND_FAIL);
		}
#endif // _ERROR_DEBUGGING

		return -1;
	}
	else if (ret < bytes) {
#ifdef _ERROR_DEBUGGING
		printf("[%s] %s\n", WARNING_FLAGS, _SEND_NOT_ALL);
#endif // _ERROR_DEBUGGING
		return 0;
	}
	return 1;
}

int SegmentationSend(SOCKET sender, const char* message, int message_len, int* obyte_sent)
{
	int start_byte = 0; // start byte in message.
	unsigned short bsend = 0; // number of bytes will send, not include header size.
	unsigned short bremain = 0; // number of bytes remain.

	char content[APPLICATION_BUFF_MAX_SIZE];
	while (start_byte < message_len) {
		// Prepare content for sending: header (number of bytes send | number of bytes remain) + body (a part of message)
		bsend = message_len - start_byte;
		if (bsend + SEGMENT_HEADER_SIZE > APPLICATION_BUFF_MAX_SIZE) {
			bsend = APPLICATION_BUFF_MAX_SIZE - SEGMENT_HEADER_SIZE;
		}
		bremain = message_len - start_byte - bsend;

		int bsend_bigendian = ToNetworkByteOrder(bsend); // uniform with many architectures.
		int bremain_bigendian = ToNetworkByteOrder(bremain);

		memcpy_s(content, SEGMENT_HEADER_CURRENT_SIZE, &bsend_bigendian, SEGMENT_HEADER_CURRENT_SIZE);
		memcpy_s(content + SEGMENT_HEADER_CURRENT_SIZE, SEGMENT_HEADER_REMAIN_SIZE, &bremain_bigendian, SEGMENT_HEADER_REMAIN_SIZE);
		memcpy_s(content + SEGMENT_HEADER_SIZE, bsend, message + start_byte, bsend);
		// Send
		int ret = Send(sender, bsend + SEGMENT_HEADER_SIZE, content);
		if (ret == 1) {
			start_byte += bsend;
		}
		else {
			if (obyte_sent != NULL)
				*obyte_sent = start_byte;
			return ret;
		}
	}
	if (obyte_sent != NULL)
		*obyte_sent = start_byte;
	return 1;
}
int ReliableSend(SOCKET socket, const char* message, int message_len)
{
	int sent_byte;
	int status = SegmentationSend(socket, message, message_len, &sent_byte);
	if (status == -1) {
		return -1;
	}
	else if (status == 0) {
		int succ_count = sent_byte;
		while (status == 0) {
			status = SegmentationSend(socket, message + succ_count, message_len - succ_count, &sent_byte);
			if (status == -1)
				return -1;
			succ_count += sent_byte;
		}
	}
	return status;
}


int Receive(SOCKET receiver, int bytes, char** obyte_stream)
{
	if (bytes > APPLICATION_BUFF_MAX_SIZE)
	{
#ifdef _ERROR_DEBUGGING
		printf("[%s] %s\n", WARNING_FLAGS, _TOO_MUCH_BYTES);
#endif // _ERROR_DEBUGGING
		bytes = APPLICATION_BUFF_MAX_SIZE;
	}
	char buffer[APPLICATION_BUFF_MAX_SIZE];

	int ret = recv(receiver, buffer, bytes, 0);
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		int err = WSAGetLastError();
		if (err == WSAECONNABORTED || err == WSAECONNRESET) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
		}
		else {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _RECEIVE_FAIL);
		}
#endif // _ERROR_DEBUGGING

		return -1;
	}
	else if (ret == 0) {
		return -1;
	}
	else if (ret < bytes) {
#ifdef _ERROR_DEBUGGING
		printf("[%s] %s\n", WARNING_FLAGS, _RECEIVE_UNEXPECTED_MESSAGE);
#endif // _ERROR_DEBUGGING
		* obyte_stream = NULL;
		return 0;
	}
	else {
		*obyte_stream = Clone(buffer, bytes);
	}
	return 1;
}

int ReceiveSegment(SOCKET receiver, char** obyte_stream, int* ostream_len, int* oremain)
{
	*obyte_stream = NULL;
	char* read;
	int remain;
	int current;
	// read number of bytes remain | number of bytes current
	int ret = Receive(receiver, SEGMENT_HEADER_SIZE, &read);
	if (ret == 1) {
		current = ToHostByteOrder(*(unsigned short*)read);
		remain = ToHostByteOrder(*(unsigned short*)(read + SEGMENT_HEADER_CURRENT_SIZE));

		*ostream_len = current;
		*oremain = remain;
		free(read);
	}
	else {
		*ostream_len = 0;
		*oremain = 0;
		return ret;
	}

	// read message content
	ret = Receive(receiver, current, &read);
	if (ret == 1) {
		*obyte_stream = Clone(read, current);
		free(read);
	}
	else {
		*ostream_len = 0;
		*oremain = 0;
		return ret;
	}
	return 1;
}

int SegmentationReceive(SOCKET socket, char** omessage)
{
	char* _message;
	int mlen, remain, start_byte = 0;
	int status = 1;
	*omessage = NULL;
	while (1) {
		status = ReceiveSegment(socket, &_message, &mlen, &remain);
		if (status != 1) {
			free(_message);
			return status;
		}
		if (*omessage == NULL) {
			*omessage = (char*)malloc((size_t)mlen + remain);
			if (*omessage == NULL) {
#ifdef _ERROR_DEBUGGING
				printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
#endif // _ERROR_DEBUGGING
				free(_message);
				return 0;
			}
		}
		memcpy_s(*omessage + start_byte, mlen, _message, mlen);
		start_byte += mlen;
		free(_message);
		if (remain <= 0)
			break;
	}
	return status;
}

#pragma endregion