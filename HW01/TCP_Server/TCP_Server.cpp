#include "TCP_Server.h"

int main(int argc, char* argv[])
{
	int running_port;
	ExtractCommand(argc, argv, &running_port);

	if (WSInitialize()) {
		SOCKET listener = CreateSocket(TCP);
		ADDRESS socket_address = CreateSocketAddress(CreateDefaultIP(), running_port);
		if (listener != INVALID_SOCKET) {
			if (BindSocket(listener, socket_address)) {
				if (ListenConnections(listener)) {
					printf("[%s] Listenning at port %d...\n", INFO_FLAGS, running_port);
					while (1) {
						SOCKET connector = GetConnectionSocket(listener);
						while (connector != INVALID_SOCKET) {
							// communicate
							int status = HandleRequest(connector);
							if (status == -1) {
								CloseSocket(connector, CLOSE_SAFELY);
								connector = INVALID_SOCKET;
							}
						}
					}
				}
			}
		}
		CloseSocket(listener, CLOSE_SAFELY);
		WSCleanup();
	}
	printf("[%s] Stopping...\n", INFO_FLAGS);
	return 0;
}

#pragma region Socket Common

int WSInitialize()
{
    WORD version = MAKEWORD(2, 2);
    WSADATA wsa_data;
    if (WSAStartup(version, &wsa_data)) {
        printf("[%s] %s\n", ERROR_FLAGS, _INITIALIZE_FAIL);
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

    if (s == INVALID_SOCKET) {
        printf("[%s] %s\n", ERROR_FLAGS, _CREATE_SOCKET_FAIL);
    }
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
            printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SHUTDOWN_SOCKET_FAIL);
        }
    }
    if (closesocket(socket) == SOCKET_ERROR) {
        is_ok = 0;
        printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _CLOSE_SOCKET_FAIL);
    }
    return is_ok;
}

int BindSocket(SOCKET socket, ADDRESS addr)
{
	if (bind(socket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
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
		return 0;
	}
	return 1;
}

int ListenConnections(SOCKET socket, int connection_numbers)
{
	int ret = listen(socket, connection_numbers);
	if (ret == SOCKET_ERROR) {
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
		return 0;
	}
	return 1;
}

SOCKET GetConnectionSocket(SOCKET listener, ADDRESS* osender_address)
{
	int sender_addr_len = sizeof(SOCKADDR_IN);
	int* addr_len = osender_address == NULL ? NULL : &sender_addr_len;
	SOCKET result = accept(listener, (SOCKADDR*)osender_address, addr_len);
	if (result == INVALID_SOCKET) {
		int err = WSAGetLastError();
		if (err == WSAEINVAL) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _NOT_LISTEN_SOCKET);
		}
		else {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _ACCEPT_SOCKET_FAIL);
		}
	}
	return result;
}

#pragma endregion

#pragma region Send and Receive

int WriteSocketBuffer(SOCKET sender, int bytes, const char* message)
{
	int ret = send(sender, message, bytes, 0);
	if (ret == SOCKET_ERROR) {
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
		return -1;
	}
	else if (ret < bytes) {
		printf("[%s] %s\n", WARNING_FLAGS, _SEND_NOT_ALL);
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
		if (bsend + SEGMENTATION_HEADER_SIZE > APPLICATION_BUFF_MAX_SIZE) {
			bsend = APPLICATION_BUFF_MAX_SIZE - SEGMENTATION_HEADER_SIZE;
		}
		bremain = message_len - start_byte - bsend;

		int bsend_bigendian = htons(bsend); // uniform with many architectures.
		int bremain_bigendian = htons(bremain);

		memcpy_s(content, SEGMENTATION_HEADER_CURRENT_SIZE, &bsend_bigendian, SEGMENTATION_HEADER_CURRENT_SIZE);
		memcpy_s(content + SEGMENTATION_HEADER_CURRENT_SIZE, SEGMENTATION_HEADER_REMAIN_SIZE, &bremain_bigendian, SEGMENTATION_HEADER_REMAIN_SIZE);
		memcpy_s(content + SEGMENTATION_HEADER_SIZE, bsend, message + start_byte, bsend);
		int ret = WriteSocketBuffer(sender, bsend + SEGMENTATION_HEADER_SIZE, content);
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

int ReadSocketBuffer(SOCKET receiver, int length, char** ostream_byte)
{
	if (length > APPLICATION_BUFF_MAX_SIZE)
	{
		printf("[%s] %s\n", WARNING_FLAGS, _TOO_MUCH_BYTES);
		length = APPLICATION_BUFF_MAX_SIZE;
	}
	char buffer[APPLICATION_BUFF_MAX_SIZE];

	int ret = recv(receiver, buffer, length, 0);
	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err == WSAECONNABORTED || err == WSAECONNRESET) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
		}
		else {
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _RECEIVE_FAIL);
		}
		return -1;
	}
	else if (ret == 0) {
		return -1;
	}
	else if (ret < length) {
		printf("[%s] %s\n", WARNING_FLAGS, _RECEIVE_UNEXPECTED_MESSAGE);
		*ostream_byte = NULL;
		return 0;
	}
	else {
		*ostream_byte = Clone(buffer, length);
	}
	return 1;
}

int SegmentationReceive(SOCKET receiver, char** ostream_byte, int* ostream_len, int* oremain)
{
	*ostream_byte = NULL;
	char* read;
	int remain;
	int current;
	// read number of bytes remain | number of bytes current
	int ret = ReadSocketBuffer(receiver, SEGMENTATION_HEADER_SIZE, &read);
	if (ret == 1) {
		current = ntohs(*(unsigned short*)read);
		remain = ntohs(*(unsigned short*)(read + SEGMENTATION_HEADER_CURRENT_SIZE));

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
	ret = ReadSocketBuffer(receiver, current, &read);
	if (ret == 1) {
		*ostream_byte = Clone(read, current);
		free(read);
	}
	else {
		*ostream_len = 0;
		*oremain = 0;
		return ret;
	}
	return 1;
}

#pragma endregion

#pragma region Handle Request

int GetSumDigitOnString(const char* str, int strlen)
{
	int result = 0;
	int i = 0;
	while (i < strlen) {
		if (str[i] >= '0' && str[i] <= '9')
			result += str[i] - '0';
		else if (str[i] == '\0')
			break;
		else // '\0'
			return -1;
		i++;
	}
	return result;
}

int HandleRequest(SOCKET socket)
{
	char* request;
	int request_len;
	int remain;
	int total = 0;
	int status = 1, is_continue = 1;
	while (is_continue) {
		status = SegmentationReceive(socket, &request, &request_len, &remain);
		if (status != 1)
			return status;
		int sum = GetSumDigitOnString(request, request_len);
		free(request);
		if (sum == -1) { // contains alpha characters
			// send response
			MESSAGE response = CreateMessage(STATUS_ERROR, ERROR_MESSAGE);
			status = SegmentationSend(socket, response, strlen(response) + 1, NULL);

			if (status != 1)
				return status;
			DestroyMessage(response);
			// clean buffer
			while (remain > 0) {
				status = SegmentationReceive(socket, &request, &request_len, &remain);
				if (status != 1)
					return status;
				free(request);
			}
			return 0;
		}
		else
		{
			total += sum;
			is_continue = remain != 0;
		}
	}
	// handle success result
	char* total_str = (char*)malloc(INT_MAX_LEN + 1);
	if (total_str == NULL) {
		printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
		status = 0;
	}
	else {
		_itoa_s(total, total_str, INT_MAX_LEN + 1, 10);
		MESSAGE response = CreateMessage(STATUS_OK_END, total_str);
		status = SegmentationSend(socket, response, strlen(response) + 1, NULL);
		free(total_str);
		DestroyMessage(response);
	}
	return status;
}

#pragma endregion

#pragma region Utilities

int ExtractCommand(int argc, char* argv[], int* oport)
{
	int is_ok = 1;
	if (argc < 2) {
		printf("[%s] %s\n", WARNING_FLAGS, _NOT_SPECIFY_PORT);
		is_ok = 0;
	}
	else {
		*oport = atoi(argv[1]);
		if (*oport == 0) {
			printf("[%s] %s\n", WARNING_FLAGS, _CONVERT_PORT_FAIL);
			is_ok = 0;
		}
	}

	if (!is_ok) {
		*oport = DEFAULT_PORT;
	}
	return is_ok;
}

IP CreateDefaultIP()
{
	IP addr;
	addr.s_addr = htonl(INADDR_ANY);
	return addr;
}

char* Clone(const char* source, int length, int start)
{
	char* _clone = (char*)malloc(length + start);

	if (_clone == NULL)
		printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
	else
		memcpy_s(_clone + start, length, source, length);
	return _clone;
}

MESSAGE CreateMessage(int status, const char* message)
{
	MESSAGE m = Clone(message, strlen(message) + 1, 1);
	if (status == STATUS_OK) {
		m[0] = STATUS_OK_CHAR;
	}
	else if (status == STATUS_OK_END) {
		m[0] = STATUS_OK_END_CHAR;
	}
	else if (status == STATUS_ERROR) {
		m[0] = STATUS_ERROR_CHAR;
	}
	else {
		m[0] = '\0';
	}
	return m;
}

void DestroyMessage(MESSAGE m)
{
	free(m);
	m = NULL;
}

#pragma endregion

