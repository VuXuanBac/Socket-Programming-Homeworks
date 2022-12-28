#include "UDP_Server.h"

int main(int argc, char* argv[])
{
    int running_port;
    ExtractCommand(argc, argv, &running_port);

    if (WSInitialize()) {
        SOCKET socket = CreateSocket(UDP);
        ADDRESS socket_address = CreateSocketAddress(CreateDefaultIP(), running_port);
        if (socket != INVALID_SOCKET) {
            if (BindSocket(socket, socket_address)) {
                printf("[%s] Ready to communicate at port %d...\n", INFO_FLAGS, running_port);
                char* request;
                ADDRESS client;
                while (1) {
                    if (Receive(socket, &request, &client)) {
                        HandleDomainNameRequest(request, socket, client);
                    }
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

#pragma region Handle Request

int TranslateDomainName(const char* name, ADDRINFO** oinfos)
{
    ADDRINFO hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // just use IPv4

    int ret = getaddrinfo(name, NULL, &hints, oinfos);
    if (ret != 0) {
        if (ret != WSAHOST_NOT_FOUND) // other from common error
            printf("[%s:%d] %s: %s\n", WARNING_FLAGS, ret, _TRANSLATE_DOMAIN_FAIL, name);
    }

    return ret == 0;
}

void HandleDomainNameRequest(const char* name, SOCKET sender, ADDRESS receiver)
{
    ADDRINFO* results;
    int is_ok = TranslateDomainName(name, &results);

    if (!is_ok || results == NULL) {
        MESSAGE response = CreateMessage(STATUS_ERROR, ERROR_MESSAGE);
        Send(sender, response, receiver);
        DestroyMessage(response);
    }
    else {
        ADDRINFO* node = results;
        while (node != NULL) {
            char* address = GetIPString(*(ADDRESS*)(node->ai_addr));
            if (address != NULL) {
                int status = (node->ai_next == NULL ? STATUS_OK_END : STATUS_OK);
                MESSAGE response = CreateMessage(status, address);
                Send(sender, response, receiver);

                free(address);
                DestroyMessage(response);
            }
            node = node->ai_next;
        }
    }
    freeaddrinfo(results);
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

int TryParseIPString(const char* str, IP* oip)
{
    return inet_pton(AF_INET, str, oip) == 1;
}

char* GetIPString(ADDRESS addr)
{
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(addr.sin_family, &addr.sin_addr, ip_str, sizeof(ip_str)) != NULL) {
        char* result = (char*)malloc(sizeof(ip_str));
        if (result == NULL) {
            printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
            return NULL;
        }
        strcpy_s(result, sizeof(ip_str), ip_str);
        return result;
    }
    return NULL;
}

IP CreateDefaultIP()
{
    IP addr;
    addr.s_addr = htonl(INADDR_ANY);
    return addr;
}

MESSAGE CreateMessage(int status, const char* message)
{
    MESSAGE m = (MESSAGE)malloc(MESSAGE_MAX_SIZE);
    if (m == NULL) {
        printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
        return NULL;
    }
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
    strcpy_s(m + 1, strlen(message) + 1, message);
    return m;
}

void DestroyMessage(MESSAGE m)
{
    free(m);
    m = NULL;
}
#pragma endregion