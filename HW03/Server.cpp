#include "Server.h"

ACCOUNTINFO* gAccounts = NULL;
SOCKETSMANAGER* gSocketsManager = NULL;
CRITICAL_SECTION gAccountCriticalSection;
CRITICAL_SECTION gSocketsManagerCriticalSection;

int main(int argc, char* argv[])
{
	int running_port;
	ExtractCommand(argc, argv, &running_port);
	if (WSInitialize()) {
		SOCKET listener = CreateSocket(TCP);

		ADDRESS socket_address = CreateSocketAddress(CreateDefaultIP(), running_port);

		if (listener != INVALID_SOCKET) {
			if (BindSocket(listener, socket_address)) {
				if (SetListenState(listener)) {

					printf("[%s] Listenning at port %d...\n", INFO_FLAGS, running_port);

					if (LoadAccountList(ACCOUNT_FILE_PATH)) {

						InitializeCriticalSection(&gAccountCriticalSection);
						InitializeCriticalSection(&gSocketsManagerCriticalSection);
						while (1) {
							SOCKET connector = GetConnectionSocket(listener);
							if (connector != INVALID_SOCKET) {
								int is_new;

								EnterCriticalSection(&gSocketsManagerCriticalSection);
								SOCKETSMANAGER* manager = FindFirstFreeManagerOrCreateNew(&is_new);
								if (is_new) { // not found
									CreateThreadForSocketsManager(manager);
								}
								SetSocket(manager, connector);
								LeaveCriticalSection(&gSocketsManagerCriticalSection);
							}
						}
						DeleteCriticalSection(&gSocketsManagerCriticalSection);
						DeleteCriticalSection(&gAccountCriticalSection);

						FreeAccountList(gAccounts);
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

#pragma region Thread and Session

HANDLE CreateThreadForSocketsManager(SOCKETSMANAGER* manager)
{
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, Run, (void*)manager, 0, NULL);
	if (thread == 0) { // has error
		if (errno == EAGAIN) {
			printf("[%s] %s\n", WARNING_FLAGS, _TOO_MANY_THREADS);
		}
		else if (errno == EACCES) {
			printf("[%s] %s\n", WARNING_FLAGS, _INSUFFICIENT_RESOURCES);
		}
	}
	return thread;
}

SOCKETSET* GetCanReadSockets(SOCKETSET* sockets)
{
	int ret = select(0, sockets, NULL, NULL, NULL);
	if (ret == SOCKET_ERROR) {
		printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _CHECK_SOCKETSET_FAIL);
	}
	return sockets;
}

unsigned __stdcall Run(void* arguments)
{
	SOCKETSMANAGER* manager = (SOCKETSMANAGER*)arguments; // a pointer to a node in gSocketsManager.
	while (1) {
		//EnterCriticalSection(&gSocketsManagerCriticalSection);
		int ret = select(0, &(manager->probe_set), NULL, NULL, NULL);
		if (ret == SOCKET_ERROR) {
			printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _CHECK_SOCKETSET_FAIL);
		}
		SOCKETSET* sset = &(manager->probe_set);
		//LeaveCriticalSection(&gSocketsManagerCriticalSection);
		int can_read_sockets_count = sset->fd_count;
		if (can_read_sockets_count > 0) {
			for (int i = 0; i < MAX_CLIENTS_PER_THREAD; ++i) {

				EnterCriticalSection(&gSocketsManagerCriticalSection);
				SOCKET connector = manager->sockets[i];
				int connector_account_status = manager->accounts_status[i];
				LeaveCriticalSection(&gSocketsManagerCriticalSection);

				if (FD_ISSET(connector, sset)) {
					int status = HandleRequest(connector, &connector_account_status);

					if (status == -1) { // have fatal error
						CloseSocket(connector, CLOSE_SAFELY);

						EnterCriticalSection(&gSocketsManagerCriticalSection);
						FD_CLR(connector, &(manager->init_set));
						FD_CLR(connector, &(manager->probe_set));
						manager->sockets[i] = INVALID_SOCKET;
						manager->accounts_status[i] = AS_FREE;
						LeaveCriticalSection(&gSocketsManagerCriticalSection);
					}
					else {

						EnterCriticalSection(&gSocketsManagerCriticalSection);
						manager->accounts_status[i] = connector_account_status;
						LeaveCriticalSection(&gSocketsManagerCriticalSection);
					}
				}
				if (--can_read_sockets_count < 0)
					break; // have no more can read sockets
			}
		}
	}
	return 0; // terminate thread
}

#pragma endregion

#pragma region Handle Request

MESSAGE HandlePostRequest(SOCKET socket, const char* arguments, int* ioaccount_status)
{
	if (*ioaccount_status == AS_FREE) {
		return CreateMessage(S_NOT_LOGIN, SM_NOT_LOGIN);
	}

	return CreateMessage(S_POST_SUCC, SM_POST_SUCC);
}

MESSAGE HandleLoginRequest(SOCKET socket, const char* arguments, int* ioaccount_status)
{
	if (*ioaccount_status == AS_LOGGED_IN) {
		return CreateMessage(S_LOGGEDIN, SM_LOGGEDIN);
	}

	EnterCriticalSection(&gAccountCriticalSection);
	ACCOUNTINFO* acc = FindFirstAccountInfo(gAccounts, arguments);
	int status = acc == NULL ? -1 : acc->status;
	LeaveCriticalSection(&gAccountCriticalSection);

	if (status == -1) { // not found
		return CreateMessage(S_ACCOUNT_NOT_EXIST, SM_ACCOUNT_NOT_EXIST);
	}
	//else if (status == AS_LOGGED_IN) {
	//	return CreateMessage(S_ACCOUNT_LOGGEDIN, SM_ACCOUNT_LOGGEDIN);
	//}
	else if (status == AS_LOCK) {
		return CreateMessage(S_ACCOUNT_LOCK, SM_ACCOUNT_LOCK);
	}
	else { // AS_FREE
		*ioaccount_status = AS_LOGGED_IN;
	}
	return CreateMessage(S_LOGIN_SUCC, SM_LOGIN_SUCC);
}

MESSAGE HandleLogoutRequest(SOCKET socket, int* ioaccount_status)
{
	if (*ioaccount_status == AS_FREE) {
		return CreateMessage(S_NOT_LOGIN, SM_NOT_LOGIN);
	}

	*ioaccount_status = AS_FREE;
	return CreateMessage(S_LOGOUT_SUCC, SM_LOGOUT_SUCC);
}

int HandleRequest(SOCKET socket, int* ioaccount_status)
{
	char* request, *arguments;
	int status = 1;
	status = SegmentationReceive(socket, &request);
	if (status != 1) {
		free(request);
		return status;
	}

	MESSAGE response = NULL;
	// Handle request
	int command = ExtractRequestCommand(request, &arguments);
	if (command == C_POST) {
		response = HandlePostRequest(socket, arguments, ioaccount_status);
	}
	else if (command == C_LOGIN) {
		response = HandleLoginRequest(socket, arguments, ioaccount_status);
	}
	else if (command == C_LOGOUT) {
		response = HandleLogoutRequest(socket, ioaccount_status);
	}
	else {
		response = CreateMessage(S_UNREGCONIZE_COMMAND, SM_UNREGCONIZE_COMMAND);
	}
	free(request);

	// Send response
	status = SegmentationSend(socket, response, (int)strlen(response) + 1, NULL);
	DestroyMessage(response);
	return status;
}

int ExtractRequestCommand(const char* request, char** oarguments)
{
	char* space_pos = (char*)memchr(request, ' ', strlen(request));
	if (space_pos == NULL) {
		if (ICompare(request, CM_LOGOUT, (int)max(strlen(CM_LOGOUT), strlen(request))) == 0)
			return C_LOGOUT;
		return 0;
	}

	*oarguments = space_pos + 1;
	// match command
	if (ICompare(request, CM_POST,
		(int)max((int)strlen(CM_POST), (space_pos - request)))
		== 0)
		return C_POST;
	else if (ICompare(request, CM_LOGIN,
		(int)max((int)strlen(CM_LOGIN), (space_pos - request)))
		== 0)
		return C_LOGIN;
	else if (ICompare(request, CM_LOGOUT,
		(int)max((int)strlen(CM_LOGOUT), (space_pos - request)))
		== 0)
		return C_LOGOUT;

	return 0;
}

#pragma endregion

#pragma region Linked Lists

SOCKETSMANAGER* FindFirstFreeManagerOrCreateNew(int* ois_new)
{
	SOCKETSMANAGER* cur = gSocketsManager;
	SOCKETSMANAGER* prev = NULL;
	if (ois_new != NULL)
		*ois_new = 1;
	while (cur != NULL) {
		// if found on a Socket Manager
		if (cur->free_index >= 0 && cur->free_index < MAX_CLIENTS_PER_THREAD) {
			if (ois_new != NULL)
				*ois_new = 0;
			return cur;
		}
		prev = cur;
		cur = cur->next;
	}
	return Append(prev, CreateSocketsManager());
}

int SetSocket(SOCKETSMANAGER* manager, SOCKET socket)
{
	if (manager != NULL) {
		manager->sockets[manager->free_index] = socket;
		FD_SET(socket, &(manager->init_set));
		FD_SET(socket, &(manager->probe_set));
		// change free item index.
		int next = (manager->free_index + 1) % MAX_CLIENTS_PER_THREAD;
		if (manager->sockets[next] == INVALID_SOCKET) { // next index is free
			manager->free_index = next;
		}
		else { // linear search
			manager->free_index = -1;
			for (int i = 0; i < MAX_CLIENTS_PER_THREAD; ++i) {
				if (manager->sockets[i] == INVALID_SOCKET) { // i index is free
					manager->free_index = i;
					break;
				}
			}
		}
		return 0;
	}
	return 1;
}

SOCKETSMANAGER* CreateSocketsManager()
{
	SOCKETSMANAGER* mgr = (SOCKETSMANAGER*)malloc(sizeof(SOCKETSMANAGER));
	if (mgr != NULL) {
		FD_ZERO(&(mgr->init_set));
		FD_ZERO(&(mgr->probe_set));
		mgr->free_index = 0;
		mgr->next = NULL;
		memset(mgr->accounts_status, AS_FREE, sizeof(mgr->accounts_status));
		memset(mgr->sockets, (int)INVALID_SOCKET, sizeof(mgr->sockets));
	}
	return mgr;
}

SOCKETSMANAGER* Append(SOCKETSMANAGER* prev, SOCKETSMANAGER* current)
{
	if (prev != NULL) {
		current->next = prev->next;
		prev->next = current;
	}
	else {
		current->next = gSocketsManager;
		gSocketsManager = current;
	}
	return current;
}

void FreeSocketsManagerList(SOCKETSMANAGER* first)
{
	SOCKETSMANAGER* cur = first;
	SOCKETSMANAGER* prev = NULL;
	while (cur != NULL) {
		prev = cur;
		cur = cur->next;
		free(prev);
	}
}

ACCOUNTINFO* Append(ACCOUNTINFO* prev, ACCOUNTINFO* current)
{
	if (prev != NULL) {
		current->next = prev->next;
		prev->next = current;
	}
	else {
		current->next = gAccounts;
		gAccounts = current;
	}
	return current;
}

ACCOUNTINFO* CreateAccountInfo(const char* username, int namelen, int status)
{
	ACCOUNTINFO* acc = (ACCOUNTINFO*)malloc(sizeof(ACCOUNTINFO));
	if (acc != NULL) {
		acc->account = Clone(username, namelen);
		acc->account[namelen] = '\0';
		//acc->socket = INVALID_SOCKET;
		acc->status = status;
		acc->next = NULL;
	}
	return acc;
}

ACCOUNTINFO* FindFirstAccountInfo(ACCOUNTINFO* start, const char* username)
{
	ACCOUNTINFO* cur = start;
	while (cur != NULL) {
		if (ICompare(username, cur->account) == 0)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

void FreeAccountList(ACCOUNTINFO* first)
{
	ACCOUNTINFO* cur = first;
	ACCOUNTINFO* prev = NULL;
	while (cur != NULL) {
		prev = cur;
		cur = cur->next;
		free(prev->account);
		free(prev);
	}
}

#pragma endregion

#pragma region I/O Operations

int LoadAccountList(const char* file)
{
	FILE* fp;
	fopen_s(&fp, file, "r");
	if (fp == NULL) {
		printf("[%s] Fail to open account file: '%s'\n", ERROR_FLAGS, file);
		return 0;
	}
	char line[LINE_MAX_SIZE];
	int status;
	ACCOUNTINFO* cur = NULL;
	while (fgets(line, LINE_MAX_SIZE, fp)) {
		char* space_pos = (char*)memchr(line, ' ', strlen(line));
		if (space_pos == NULL) {
			if (strlen(line) > 1) {
				status = AS_LOCK;
				cur = Append(cur, CreateAccountInfo(line, (int)strlen(line), status));
			}
		}
		else {
			status = atoi(space_pos + 1);
			cur = Append(cur, CreateAccountInfo(line, (int)(space_pos - line), status));
		}
	}
	fclose(fp);
	return 1;
}

#pragma endregion

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

int SetListenState(SOCKET socket, int connection_numbers)
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

int Send(SOCKET sender, int bytes, const char* byte_stream)
{
	int ret = send(sender, byte_stream, bytes, 0);
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
		if (bsend + SEGMENT_HEADER_SIZE > APPLICATION_BUFF_MAX_SIZE) {
			bsend = APPLICATION_BUFF_MAX_SIZE - SEGMENT_HEADER_SIZE;
		}
		bremain = message_len - start_byte - bsend;

		int bsend_bigendian = htons(bsend); // uniform with many architectures.
		int bremain_bigendian = htons(bremain);

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

int Receive(SOCKET receiver, int bytes, char** obyte_stream)
{
	if (bytes > APPLICATION_BUFF_MAX_SIZE)
	{
		printf("[%s] %s\n", WARNING_FLAGS, _TOO_MUCH_BYTES);
		bytes = APPLICATION_BUFF_MAX_SIZE;
	}
	char buffer[APPLICATION_BUFF_MAX_SIZE];

	int ret = recv(receiver, buffer, bytes, 0);
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
	else if (ret < bytes) {
		printf("[%s] %s\n", WARNING_FLAGS, _RECEIVE_UNEXPECTED_MESSAGE);
		*obyte_stream = NULL;
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
		current = ntohs(*(unsigned short*)read);
		remain = ntohs(*(unsigned short*)(read + SEGMENT_HEADER_CURRENT_SIZE));

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
				printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
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
	char* _clone = (char*)malloc((size_t)length + start);

	if (_clone == NULL)
		printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
	else
		memcpy_s(_clone + start, length, source, length);
	return _clone;
}

MESSAGE CreateMessage(int status, const char* message)
{
	MESSAGE m;
	if (message == NULL) {
		m = Clone("", 1, STATUS_LENGTH);
	}
	else {
		m = Clone(message, (int)strlen(message) + 1, STATUS_LENGTH);
	}
	if (m != NULL) {
		// accept 2 digits value for status
		m[0] = status / 10 + '0';
		m[1] = status % 10 + '0';
	}
	return m;
}

void DestroyMessage(MESSAGE m)
{
	free(m);
}

int ICompare(const char* first, const char* second, int length)
{
	int flen = (int)strlen(first) + 1;
	int slen = (int)strlen(second) + 1;
	int len = length <= 0 ? flen : length;
	if (len > flen) len = flen;
	if (len > slen) len = slen;

	for (int i = 0; i < len; ++i) {
		char fi = *(first + i);
		char si = *(second + i);
		if (fi != si) { // 'a' & 'A'    'A' & 'a'      'a' & '!'     'A' & '!'     '!' & '?'
			if (fi >= 'a' && fi <= 'z') {
				if (fi - 'a' + 'A' != si)  // 'a' & '!'
					return fi > si ? 1 : -1;
			}
			else if (fi >= 'A' && fi <= 'Z') {
				if (fi - 'A' + 'a' != si)  // 'A' & '!'
					return fi > si ? 1 : -1;
			}
			else
				return fi > si ? 1 : -1; // '!' & '?'
		}
	}
	return 0;
}

#pragma endregion