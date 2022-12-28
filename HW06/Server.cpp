#include "Server.h"

CLIENTINFO clients[MAX_CLIENTS];
int clients_count = 0;
int new_client_index;
CRITICAL_SECTION critical_section;

int main(int argc, char* argv[])
{
	if (!IsExist(DEFAULT_TEMP_FOLDER)) {
		CreateFolder(DEFAULT_TEMP_FOLDER);
	}
	if (WSInitialize()) {
		SOCKET listener = CreateSocket(TCP);
		//SetSendBufferSize(listener, 3 * 4096); // config for all connectors get from this listener
		//SetReceiveBufferSize(listener, 3 * 4096);

		ADDRESS socket_address = CreateSocketAddress(CreateDefaultIP(), DEFAULT_PORT);

		if (listener != INVALID_SOCKET) {

			if (BindSocket(listener, socket_address)) {
				if (SetListenState(listener)) {

#ifdef _ERROR_DEBUGGING
					printf("[%s] Listenning at port %d...\n", INFO_FLAGS, DEFAULT_PORT);
#endif // _ERROR_DEBUGGING

					WSAEVENT listener_event = WSACreateEvent();
					CreateThread(listener_event);
					InitializeCriticalSection(&critical_section);
					while (1) {
						SOCKET connector = GetConnectionSocket(listener);
						if (connector != INVALID_SOCKET) {

							EnterCriticalSection(&critical_section);
							new_client_index = AppendSocketToManager(connector);
							LeaveCriticalSection(&critical_section);

							if (new_client_index > -1) {
								// signal event to IO thread
								SignalEvent(listener_event);
							}
						}
					}
					DeleteCriticalSection(&critical_section);

				}
			}
		}
		CloseSocket(listener, CLOSE_SAFELY);
		WSCleanup();
	}
#ifdef _ERROR_DEBUGGING
	printf("[%s] Stopping...\n", INFO_FLAGS);
#endif // _ERROR_DEBUGGING
	return 0;
}

#pragma region Thread and Session

unsigned __stdcall RunOverlappedIO(void* arguments_wsaevent)
{
	WSAEVENT accepted_event = (WSAEVENT)arguments_wsaevent;
	while (1) {
		int ret = ListenEvents(&accepted_event, 1);
		if (ret == FATAL_ERROR) {
			return 0;
		}

		if (ret == 0) { // accepted socket signal
			WSAResetEvent(accepted_event);

			EnterCriticalSection(&critical_section);

			CLIENTINFO* client = clients + new_client_index;

			// invoke receive to initiate overlapped event
			if (ReceiveRequestHeader(client) == FATAL_ERROR) {
				RemoveClientFromManager(client);
			}
			LeaveCriticalSection(&critical_section);
		}
	}
	return 0;
}

HANDLE CreateThread(WSAEVENT listener_event)
{
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, RunOverlappedIO, (void*)listener_event, 0, NULL);
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

#pragma endregion

#pragma region Winsock Completion IO

int HandleIOResult(CLIENTINFO* client, int sockex_status)
{
	switch (sockex_status) {
	case SS_RECH: // receive segment header success -> continue receive message in this segment
		return ReceiveRequestContent(client);

	case SS_RECC: // receive message success -> handle request for the message
		return Request(client);

	case SS_SENA: // successful send ack for the receiving -> continue receive
		return ReceiveRequestHeader(client);

	case SS_RECA: // receive ack -> continue send
		return Respond(client);

	case SS_SEND: // send success -> receive ack for this sending
		return ReceiveAckSendStatus(client);

	case SS_FREE: // response success -> start new request.
		Reset(client);
		return ReceiveRequestHeader(client);

	default:
		return SUCCESS;
	}
}

int SendAckReceiveStatus(CLIENTINFO* client)
{
	UpdateStatus(&(client->socketex), SS_SENA);
	return SendACK(&(client->socketex));
}

int ReceiveRequestHeader(CLIENTINFO* client)
{
	UpdateStatus(&(client->socketex), SS_RECH);
	return ReceiveSegmentHeader(&(client->socketex)); // new session
}

int ReceiveRequestContent(CLIENTINFO* client)
{
	uint message_size_from_header = ToHostByteOrder(ToUnsignedInt(client->socketex.buffer.buf));
	UpdateStatus(&(client->socketex), SS_RECC);
	return ReceiveSegmentContent(&(client->socketex), message_size_from_header);
}

int ReceiveAckSendStatus(CLIENTINFO* client)
{
	UpdateStatus(&(client->socketex), SS_RECA);
	return ReceiveACK(&(client->socketex));
}

void CALLBACK RoutineCallback(DWORD error, DWORD transfered_bytes, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	CLIENTINFO* client = GetClientInfo(overlapped);
	SOCKETEX* sockex = &(client->socketex);

	int operation_status = SUCCESS;

	if (error != 0) {
		printf("[%s:%d] Have some errors on Completion Routine\n", WARNING_FLAGS, error);
		operation_status = FATAL_ERROR;
	}
	//printf("%d\n", ++_count);

	if (transfered_bytes == 0) {
		operation_status = FATAL_ERROR;
	}
	else if (transfered_bytes < sockex->buffer.len) {
		int status = SUCCESS;
		switch (sockex->status) {
			case SS_RECH:
				//printf("[%s] Receive segment header fail at client %d: %d/%d\n", WARNING_FLAGS,
				//sockex->socket, transfered_bytes, sockex->buffer.len);
			case SS_RECC:
				//printf("[%s] Receive segment content fail at client %d: %d/%d\n", WARNING_FLAGS,
					//sockex->socket, transfered_bytes, sockex->buffer.len);
				status = ContinueReceive(sockex, sockex->buffer.len - transfered_bytes, transfered_bytes);
				if (status != FATAL_ERROR)
					return;
				break;
			case SS_SEND:
				//printf("[%s] Send segment fail at client %d: %d/%d\n", WARNING_FLAGS,
				//	sockex->socket, transfered_bytes, sockex->buffer.len);
				status = ContinueSend(sockex, sockex->buffer.len - transfered_bytes, transfered_bytes);
				if (status != FATAL_ERROR)
					return;
				break;
		}
		if (status == FATAL_ERROR)
			operation_status = FATAL_ERROR;
	}

	if (operation_status == SUCCESS) {
		operation_status = HandleIOResult(client, sockex->status);
	}

	if (operation_status == FATAL_ERROR) {
		EnterCriticalSection(&critical_section);
		RemoveClientFromManager(client);
		LeaveCriticalSection(&critical_section);
	}
}

#pragma endregion

#pragma region Client Manager

CLIENTINFO CreateClientInfo(SOCKET socket)
{
	CLIENTINFO c; {
		c.key = 0;
		c.socketex = CreateSocketExtend(socket, SEGMENT_MAX_SIZE, RoutineCallback);
		c.request_type = RT_INVALID;
		c.temp_file_path = NULL;
		c.temp_file_position = 0;
	}
	return c;
}

void Reset(CLIENTINFO* client)
{
	// SOCKETEX
	Reset(&(client->socketex));
	// CLIENTINFO
	client->key = 0;
	client->request_type = RT_INVALID;
	free(client->temp_file_path);
	client->temp_file_path = NULL;
	client->temp_file_position = 0;
}

int AppendSocketToManager(SOCKET socket)
{
	if (clients_count < MAX_CLIENTS) {
		WSAEventSelect(socket, NULL, 0); // unset event for accepted socket.

		clients[clients_count] = CreateClientInfo(socket);
		clients_count++;

		return clients_count - 1;
	}
	else {
#ifdef _ERROR_DEBUGGING
		printf("[%s] Too much clients served!\n", WARNING_FLAGS);
#endif // _ERROR_DEBUGGING
		CloseSocket(socket, CLOSE_SAFELY);
		return -1;
	}
}

void RemoveClientFromManager(CLIENTINFO* client)
{
#ifdef _ERROR_DEBUGGING
	printf("[%s] Remove client %d\n", INFO_FLAGS, client->socketex.socket);
#endif // _ERROR_DEBUGGING

	DestroySocketExtend(&(client->socketex));

	RemoveFile(client->temp_file_path);
	free(client->temp_file_path);
}

CLIENTINFO* GetClientInfo(OVERLAPPED* socketex_overlapped)
{
	return (CLIENTINFO*)socketex_overlapped;
}

#pragma endregion

#pragma region Handle Respond

int ProcessData(int request_type, int key, FILE* tempfp, stream* oresult, uint* oresult_len)
{
	if (oresult == NULL || oresult_len == NULL)
		return INVALID_ARGUMENTS;
	*oresult = NULL;

	stream buffer = NULL;
	uint read_count;

	int ret = ReadFromFile(tempfp, MESSAGE_PAYLOAD_MAX_SIZE, &buffer, &read_count);

	if (ret != FATAL_ERROR) {
		stream result = NULL;
		if (request_type == RT_ENCRYPT) {
			result = EncryptShiftCipher(key, buffer, read_count);
		}
		else if (request_type == RT_DECRYPT) {
			result = DecryptShiftCipher(key, buffer, read_count);
		}
		*oresult_len = read_count;
		*oresult = result;
	}
	DestroyStream(buffer);
	return ret;
}

int Respond(CLIENTINFO* client)
{
	if (client->temp_file_position == UEOF) { // eof -> send Data End Message
		RemoveFile(client->temp_file_path);
#ifdef _ERROR_DEBUGGING
		printf("[%s] Success respond result to client %d\n", INFO_FLAGS, client->socketex.socket);
#endif
		UpdateStatus(&(client->socketex), SS_FREE);
		return SendDataMessage(&(client->socketex), NULLSTR, 0);
	}

	FILE* tempfile = OpenFile(client->temp_file_path, FOM_READ);
	if (tempfile == NULL)
		return FAIL;

	int status = FAIL;
	stream message_content;
	uint message_content_len;

	MoveFilePointer(tempfile, SEEK_SET, client->temp_file_position);
	int read_status = ProcessData(client->request_type, client->key, tempfile, &message_content, &message_content_len);

	if (read_status != FATAL_ERROR) {
		UpdateStatus(&(client->socketex), SS_SEND);
		status = SendDataMessage(&(client->socketex), message_content, message_content_len);
		if (status == SUCCESS || status == WAIT) {

			client->temp_file_position += message_content_len;

			if (read_status == FAIL) { // end of file
				client->temp_file_position = UEOF;
			}
		}
	}

	DestroyStream(message_content);
	CloseFile(tempfile);
	return status;
}

#pragma endregion

#pragma region Handle Request

int HandleDataRequest(CLIENTINFO* client, const stream payload, uint payload_length)
{
	if (client->temp_file_path == NULL) {
		// create temp file to store data: random name . All temp file is in DEFAULT_TEMP_FOLDER
		client->temp_file_path = CreateUniquePath(DEFAULT_TEMP_FOLDER, strlen(DEFAULT_TEMP_FOLDER));
	}

	if (payload_length != 0) {
		FILE* tempfp = OpenFile(client->temp_file_path, FOM_APPEND);
		if (tempfp != NULL) {
			WriteToFile(tempfp, payload_length, payload);
			CloseFile(tempfp);
		}
		return SUCCESS;
	}
	else { // Data End -> send result
#ifdef _ERROR_DEBUGGING
		printf("[%s] Success receive all file from client %d\n", INFO_FLAGS, client->socketex.socket);
#endif
		UpdateStatus(&(client->socketex), SS_SEND);
		return Respond(client);
	}
}

int HandleEncryptDecryptRequest(CLIENTINFO* client, int request_type, const stream payload)
{
	client->request_type = request_type;
	client->key = ToHostByteOrder(ToUnsignedInt(payload));
	return SUCCESS;
}

int Request(CLIENTINFO* client)
{
	stream payload;
	uint payload_len;
	int status;

	int command = ExtractMessage(client->socketex.data, client->socketex.expected_transfer, &payload, &payload_len);
	if (command == MC_ENCRYPT) {
		status = HandleEncryptDecryptRequest(client, RT_ENCRYPT, payload);
	}
	else if (command == MC_DECRYPT) {
		status = HandleEncryptDecryptRequest(client, RT_DECRYPT, payload);
	}
	else if (command == MC_DATA) {
		status = HandleDataRequest(client, payload, payload_len);
	}
	else {
#ifdef _ERROR_DEBUGGING
		printf("[%s] Receive message with invalid code from client %p\n", WARNING_FLAGS, client);
#endif // _ERROR_DEBUGGING
		status = FAIL;
	}
	DestroyStream(payload);
	if (status == SUCCESS) {
		if (client->socketex.status != SS_SEND) { // not receive Data End Message
			status = SendAckReceiveStatus(client);
		}
	}
	return status;
}

#pragma endregion
