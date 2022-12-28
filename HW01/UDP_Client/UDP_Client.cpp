#include "UDP_Client.h"

int main(int argc, char* argv[]) 
{
    int server_port;
    IP server_ip;
    int is_ok = 1;
    if (ExtractCommand(argc, argv, &server_port, &server_ip) == 0) {
        printf("[%s] %s\n", WARNING_FLAGS, _CONVERT_ARGUMENTS_FAIL);
        printf("[%s] Do you want to use default address? (y/n): ", USER_INPUT_FLAGS);
        char c;
        scanf_s("%c", &c, 1);
        if (c == 'y' || c == 'Y') {
            server_port = DEFAULT_PORT;
            is_ok = TryParseIPString(DEFAULT_IP, &server_ip);
        }
        else
            is_ok = 0;
        scanf_s("%c", &c, 1); // consume '\n'
    }
    
    if (is_ok && WSInitialize()) {
        SOCKET socket = CreateSocket(UDP);
        if (socket != INVALID_SOCKET) {
            SetReceiveTimeout(socket, RECEIVE_TIMEOUT_INTERVAL);
            printf("[%s] Ready to communicate...\n", INFO_FLAGS);

            ADDRESS server = CreateSocketAddress(server_ip, server_port);
            char request[USER_INPUT_MAX_SIZE];

            while (1) {
                printf("[%s] Enter your request (domain name): ", USER_INPUT_FLAGS);
                gets_s(request, USER_INPUT_MAX_SIZE);
                if (strlen(request) == 0)
                    break;
                if (Send(socket, request, server)) {
                    HandleResponse(socket, server);
                }
            }
        }
        CloseSocket(socket, CLOSE_NORMAL);
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

#pragma endregion

#pragma region Send and Receive

int Receive(SOCKET receiver, char** omessage, ADDRESS* osender_addr)
{
    char buffer[APPLICATION_BUFF_MAX_SIZE];
    ADDRESS _sender_addr;
    int _sender_addr_len = sizeof(_sender_addr);

    int ret = recvfrom(receiver, buffer, APPLICATION_BUFF_MAX_SIZE, 0, (SOCKADDR*)&_sender_addr, &_sender_addr_len);

    int is_ok = 1;
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEMSGSIZE) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _MESSAGE_TOO_LARGE);
        }
        else {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _RECEIVE_FAIL);
            is_ok = 0;
        }
    }
    // Copy value to output variables
    if (is_ok == 1) {
        if (ret >= APPLICATION_BUFF_MAX_SIZE)
            ret = APPLICATION_BUFF_MAX_SIZE - 1;
        buffer[ret] = '\0'; // in case buffer dont have '\0' or lost byte.

        *osender_addr = _sender_addr;
        *omessage = (char*)malloc(strlen(buffer) + 1);

        if (*omessage == NULL)
        {
            printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
            is_ok = 0;
        }
        else
            strcpy_s(*omessage, strlen(buffer) + 1, buffer);
    }

    return is_ok;
}

int Send(SOCKET sender, const char* message, ADDRESS receiver, int* obyte_sent)
{
    int expect_send = (int)strlen(message) + 1;

    int ret = sendto(sender, message, expect_send, 0, (SOCKADDR*)&receiver, sizeof(receiver));

    int is_ok = 1;
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEMSGSIZE) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _MESSAGE_EXTREME_LARGE);
            is_ok = 0;
        }
        else if (err == WSAEHOSTUNREACH) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HOST_UNREACHABLE);
            is_ok = 0;
        }
        else {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _SEND_FAIL);
            is_ok = 0;
        }
        if (obyte_sent != NULL)
            *obyte_sent = 0;
    }
    else
    {
        if (obyte_sent != NULL)
            *obyte_sent = ret;
        if (ret < expect_send)
        {
            printf("[%s] %s\n", WARNING_FLAGS, _SEND_NOT_ALL);
        }
    }
    return is_ok;
}

#pragma endregion

#pragma region Handle Response

int PrintResponse(const MESSAGE message, const char* title)
{
    int has_next = 0;
    if (strlen(message) == 0) {
    }
    if (message[0] == STATUS_ERROR_CHAR) {
        printf("\t%s\n", message + 1);
    }
    else if (message[0] == STATUS_OK_CHAR || message[0] == STATUS_OK_END_CHAR) {
        if (title != NULL) {
            printf("%s", title);
        }
        printf("\t%s\n", message + 1);
        has_next = (message[0] == STATUS_OK_CHAR);
    }
    return has_next;
}

void HandleResponse(SOCKET socket, ADDRESS sender)
{
    char* response;
    char default_title[] = RESPONSE_TITLE;
    char* title = default_title;
    int is_continue = 1;
    while (is_continue) {
        if (Receive(socket, &response, &sender))
        {
            is_continue = PrintResponse(response, title);
            if (is_continue)
                title = NULL; // Only use title for the first response.
            free(response);
        }
        else
            break;
    }
}
#pragma endregion

#pragma region Utilities

int ExtractCommand(int argc, char* argv[], int* oport, IP* oip)
{
    int is_ok = 1;
    if (argc < 3) {
        *oport = 0;
        oip = NULL;
        is_ok = 0;
    }
    else {
        is_ok = TryParseIPString(argv[1], oip);
        *oport = atoi(argv[2]);
        is_ok &= (*oport != 0);
    }
    return is_ok;
}

int SetReceiveTimeout(SOCKET socket, int interval)
{
    int _interval = interval;
    int ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&_interval, sizeof(_interval));
    if (ret == SOCKET_ERROR) {
        printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SET_TIMEOUT_FAIL);
        return 0;
    }
    return 1;
}

int TryParseIPString(const char* str, IP* oip)
{
    return inet_pton(AF_INET, str, oip) == 1;
}

#pragma endregion

