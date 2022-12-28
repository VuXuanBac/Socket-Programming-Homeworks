#include "SocketLibrary.h"

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
		return FAIL;
	}
	return SUCCESS;
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
		s = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	}
	else if (protocol == TCP) {
		s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
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
		return SUCCESS;
	int status = SUCCESS;
	if (mode == CLOSE_SAFELY) {
		if (shutdown(socket, flags) == SOCKET_ERROR) {
			status = FAIL;
#ifdef _ERROR_DEBUGGING
			printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SHUTDOWN_SOCKET_FAIL);
#endif // _ERROR_DEBUGGING

		}
	}
	if (closesocket(socket) == SOCKET_ERROR) {
		status = FAIL;
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _CLOSE_SOCKET_FAIL);
#endif // _ERROR_DEBUGGING
	}
	return status;
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
		return FAIL;
	}
	return SUCCESS;
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
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _TOO_MANY_SOCKETS);
		}
		else {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _LISTEN_SOCKET_FAIL);
		}
#endif // _ERROR_DEBUGGING
		return FAIL;
	}
	return SUCCESS;
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

		return FAIL;
	}
	return SUCCESS;
}

IP CreateDefaultIP()
{
	IP addr;
	addr.s_addr = htonl(INADDR_ANY);
	return addr;
}

int AttachEventForSocket(SOCKET socket, WSAEVENT socket_event, long listen_event)
{
	int ret = WSAEventSelect(socket, socket_event, listen_event);
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _ATTACH_EVENT_FAIL);
#endif // _ERROR_DEBUGGING
		return FAIL;
	}
	return SUCCESS;
}

int ListenEvents(WSAEVENT* events, int count, int is_alertable, int time_wait)
{
	int ret = WSAWaitForMultipleEvents(count, events, FALSE, time_wait, is_alertable);
	if (ret == WSA_WAIT_TIMEOUT) {
		return FATAL_ERROR;
	}
	else if (ret == WSA_WAIT_IO_COMPLETION) {
		return WAIT;
	}
	else if (ret == WSA_WAIT_FAILED) {
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _LISTEN_EVENTS_FAIL);
#endif // _ERROR_DEBUGGING
		return FATAL_ERROR;
	}
	return ret - WSA_WAIT_EVENT_0;
}

#pragma endregion

#pragma region Segment

int CreateSegment(const stream message, uint message_len, stream* osegment, uint* osegment_len)
{
	if (osegment == NULL || osegment_len == NULL)
		return INVALID_ARGUMENTS;
	if (message_len > MESSAGE_MAX_SIZE)
		return FAIL;

	*osegment_len = message_len + SEGMENT_HEADER_SIZE;
	uint be_len = ToNetworkByteOrder(message_len);
	memcpy_s(*osegment, SEGMENT_HEADER_SIZE, &be_len, SEGMENT_HEADER_SIZE);
	memcpy_s(*osegment + SEGMENT_HEADER_SIZE, message_len, message, message_len);

	return SUCCESS;
}

#pragma endregion


#pragma region Utilities

void SignalEvent(WSAEVENT socket_event)
{
	WSASetEvent(socket_event);
}

void UnSignalEvent(WSAEVENT socket_event)
{
	WSAResetEvent(socket_event);
}

int SetReceiveTimeout(SOCKET socket, int interval)
{
	int _interval = interval;
	int ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&_interval, sizeof(_interval));
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SET_TIMEOUT_FAIL);
#endif
		return FAIL;
	}
	return SUCCESS;
}

int SetSendBufferSize(SOCKET socket, uint size)
{
	uint _size = size;
	int ret = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&_size, sizeof(_size));
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SET_BUFFER_SIZE_FAIL);
#endif
		return FAIL;
	}
	return SUCCESS;
}

int SetReceiveBufferSize(SOCKET socket, uint size)
{
	uint _size = size;
	int ret = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&_size, sizeof(_size));
	if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SET_BUFFER_SIZE_FAIL);
#endif
		return FAIL;
	}
	return SUCCESS;
}

int TryParseIPString(const char* str, IP* oip)
{
	return inet_pton(AF_INET, str, oip) == 1;
}

ushort ToNetworkByteOrder(ushort value)
{
	return htons(value);
}

uint ToNetworkByteOrder(uint value)
{
	return htonl(value);
}

ushort ToHostByteOrder(ushort value)
{
	return ntohs(value);
}

uint ToHostByteOrder(uint value)
{
	return ntohl(value);
}

#pragma endregion

#pragma region Send and Receive

int Send(SOCKET sender, int send_until_succ, uint bytes, const stream byte_stream, uint* obyte_sent)
{
	uint send_succ = 0;
	int ret;
	while (send_succ < bytes) {
		ret = send(sender, byte_stream + send_succ, bytes - send_succ, 0);
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

			return FATAL_ERROR;
		}
		else if (ret == 0) {
			return FATAL_ERROR;
		}

		send_succ += (uint)ret;

		if (!send_until_succ) {
			if (send_succ < bytes) {
				if (obyte_sent != NULL)
					*obyte_sent = send_succ;
				return FAIL;
			}
		}
	}
	if (obyte_sent != NULL)
		*obyte_sent = send_succ;
	return SUCCESS;
}

int Receive(SOCKET receiver, int recv_until_succ, uint bytes, stream* obyte_stream, uint* obyte_read)
{
	if (obyte_stream == NULL || bytes > MESSAGE_MAX_SIZE)
		return INVALID_ARGUMENTS;
	*obyte_stream = NULL;

	char buffer[MESSAGE_MAX_SIZE];
	int ret;
	uint read_succ = 0;
	while (read_succ < bytes) {
		ret = recv(receiver, buffer + read_succ, bytes - read_succ, 0);
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
			return FATAL_ERROR;
		}
		else if (ret == 0) { // 0 byte receive
			return FATAL_ERROR;
		}

		read_succ += (uint)ret;

		if (!recv_until_succ) {
			if (read_succ < bytes) {
				if (obyte_read != NULL)
					*obyte_read = read_succ;
				*obyte_stream = Clone(buffer, read_succ);
				return FAIL;
			}
		}
	}
	if (obyte_read != NULL)
		*obyte_read = read_succ;
	*obyte_stream = Clone(buffer, read_succ);
	return SUCCESS;
}

int SendSegment(SOCKET sender, int send_until_succ, const stream message, uint message_len, uint* obyte_sent)
{
	stream segment = CreateStream(SEGMENT_MAX_SIZE);
	uint segment_len;
	int ret = FAIL;
	if (CreateSegment(message, message_len, &segment, &segment_len) == SUCCESS) {
		uint byte_sent;
		ret = Send(sender, send_until_succ, segment_len, segment, &byte_sent);
		if(ret != FATAL_ERROR && obyte_sent != NULL)
			*obyte_sent = byte_sent;
	}
	DestroyStream(segment);
	return ret;
}

int ReceiveSegment(SOCKET receiver, int recv_until_succ, stream* omessage, uint* omessage_len, uint* obyte_read)
{
	if (omessage == NULL || omessage_len == NULL)
		return INVALID_ARGUMENTS;

	*omessage = NULL;

	stream read;
	uint read_count, byte_read = 0;
	uint len;
	
	// read segment content size
	int status = Receive(receiver, recv_until_succ, SEGMENT_HEADER_SIZE, &read, &read_count);
	byte_read += read_count;
	if (status == SUCCESS) {
		len = ToHostByteOrder(*(uint*)read);

		DestroyStream(read);

		// read segment content (message piece)
		status = Receive(receiver, recv_until_succ, len, &read, &read_count);
		byte_read += read_count;
		if (status != FATAL_ERROR) {
			*omessage_len = read_count;
			*omessage = Clone(read, read_count);
		}
	}
	DestroyStream(read);
	if (obyte_read != NULL)
		*obyte_read = byte_read;
	return status;
}

#pragma endregion

#pragma region Send and Receive Overlapped

int GetOverlappedResult(SOCKETEX* sockex, uint* obytes, uint* oflags)
{
	DWORD transfer_bytes, flags;
	BOOL result = WSAGetOverlappedResult(sockex->socket, &(sockex->overlapped), &transfer_bytes, FALSE, &flags);
	if (result == FALSE) {
#ifdef _ERROR_DEBUGGING
		int err = WSAGetLastError();
		if (err == WSA_INVALID_HANDLE) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _INVALID_EVENT);
		}
		else if (err == WSA_IO_INCOMPLETE) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _OVERLAPPED_IO_INCOMPLETE);
		}
		else if (err == WSA_INVALID_PARAMETER) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _INVALID_PARAMETER);
		}
		else if (err == WSAECONNRESET) {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
		}
		else {
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _GET_OVERLAPPED_RESULT_FAIL);
		}
#endif // _ERROR_DEBUGGING
		if (err == WSA_INVALID_HANDLE || err == WSA_IO_INCOMPLETE || err == WSA_INVALID_PARAMETER)
			return FAIL;
		return FATAL_ERROR;
	}
	if (transfer_bytes == 0)
		return FATAL_ERROR;
	if (obytes != NULL)
		*obytes = transfer_bytes;
	if (oflags != NULL)
		*oflags = flags;
	return SUCCESS;
}

int Send(SOCKETEX* sender)
{
	int ret = WSASend(sender->socket, &(sender->buffer), 1, NULL, NULL, &(sender->overlapped), sender->callback);
	if (ret == 0) { // WSASend return immediately
		return SUCCESS;
	}
	else if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err == WSA_IO_PENDING) {
			return WAIT;
		}
#ifdef _ERROR_DEBUGGING
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

		return FATAL_ERROR;
	}
	return SUCCESS;
}

int ContinueSend(SOCKETEX* sender, uint bytes, uint sent_success)
{
	if (bytes > MESSAGE_MAX_SIZE)
		return FAIL;
	PrepareBuffer(sender, bytes, sent_success);
	return Send(sender);
}

int SendSegment(SOCKETEX* sender, const stream message, uint message_len)
{
	uint segment_size;
	if (CreateSegment(message, message_len, &(sender->data), &segment_size) == SUCCESS) {
		PrepareBuffer(sender, segment_size);
		return Send(sender);
	}
	return FAIL;
}

int Receive(SOCKETEX* receiver)
{
	DWORD byte_recv, flags = 0;
	int ret = WSARecv(receiver->socket, &(receiver->buffer), 1, &byte_recv, &flags, &(receiver->overlapped), receiver->callback);
	if (ret == 0) { // WSARecv return immediately
		return SUCCESS;
	}
	else if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err == WSA_IO_PENDING) {
			return WAIT;
		}
#ifdef _ERROR_DEBUGGING
		if (err == WSAECONNABORTED || err == WSAECONNRESET)
			printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
		else
			printf("[%s:%d] %s\n", WARNING_FLAGS, err, _RECEIVE_FAIL);
#endif // _ERROR_DEBUGGING

		return FATAL_ERROR;
	}
	return SUCCESS;
}

int ContinueReceive(SOCKETEX* receiver, uint bytes, uint receive_success)
{
	if (bytes > MESSAGE_MAX_SIZE)
		return FAIL;
	PrepareBuffer(receiver, bytes, receive_success);
	return Receive(receiver);
}

int ReceiveSegmentHeader(SOCKETEX* receiver)
{
	PrepareBuffer(receiver, SEGMENT_HEADER_SIZE);
	return Receive(receiver);
}

int ReceiveSegmentContent(SOCKETEX* receiver, uint bytes)
{
	if (bytes > MESSAGE_MAX_SIZE) 
		return FAIL;
	PrepareBuffer(receiver, bytes);
	return Receive(receiver);
}

#pragma endregion

#pragma region Socket Extend

SOCKETEX CreateSocketExtend(SOCKET socket, uint buffer_size, OCRCALLBACK callback)
{
	SOCKETEX s; {
		s.socket = socket;
		s.callback = callback;
		memset(&(s.overlapped), 0, sizeof(s.overlapped));
		//s.overlapped.hEvent = socket_event;
		s.data = CreateStream(buffer_size);
		s.buffer.buf = s.data;
		s.buffer.len = 0;
		s.status = SS_FREE;
	}
	return s;
}

void Reset(SOCKETEX* sockex)
{
	sockex->status = SS_FREE;
	memset(&(sockex->overlapped), 0, sizeof(sockex->overlapped));
}

void PrepareBuffer(SOCKETEX* sockex, uint bytes, uint start_byte_in_data)
{
	sockex->buffer.len = bytes;
	sockex->buffer.buf = sockex->data + start_byte_in_data;
	if (start_byte_in_data == 0)
		sockex->expected_transfer = bytes;
}

void UpdateStatus(SOCKETEX* sockex, int status)
{
	sockex->status = status;
}

void DestroySocketExtend(SOCKETEX* sockex)
{
	free(sockex->data);
	CloseSocket(sockex->socket, CLOSE_SAFELY);
	sockex->socket = (SOCKET)0;
}
#pragma endregion
