#include "Server.h"

SOCKET listener;
SOCKETEX SocketsManager[MAX_CLIENTS];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    EnableConsole();

    RegisterWindowClass(hInstance);

    HWND window = CreateWindowInstance(hInstance, nCmdShow);

    if (window != NULL) {

        if (PrepareForCommunicationOnSocket(window)) {

            if (IsExist(DEFAULT_TEMP_FOLDER) == 0) {
                CreateFolder(DEFAULT_TEMP_FOLDER);
            }

            MSG msg;
            // GetMessage: Extract (Remove) first message from window queue. [Blocking]
            // False when msg is WM_QUIT: ( Can created programmingly by calling: PostQuitMessage(0) )
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg); // Translate Keyboard Inputs to Character Message
                DispatchMessage(&msg); // Invoke Window Procedure inside it (HandleNotification)
            }

            return (int)msg.wParam;
        }
    }
    return FALSE;
}

int PrepareForCommunicationOnSocket(HWND window)
{
    if (WSInitialize()) {

        listener = CreateSocket(TCP);
        ADDRESS socket_address = CreateSocketAddress(CreateDefaultIP(), DEFAULT_PORT);

        if (listener != INVALID_SOCKET) {
            if (RegisterSocketNotification(listener, window, FD_ACCEPT | FD_READ | FD_CLOSE)) {
                if (BindSocket(listener, socket_address)) {
                    if (SetListenState(listener)) {
#ifdef _ERROR_DEBUGGING
                        printf("[%s] Listenning at port %d...\n", INFO_FLAGS, DEFAULT_PORT);
#endif // _ERROR_DEBUGGING
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

#pragma region Window Desktop Common

ATOM RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = HandleNotification;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

HWND CreateWindowInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND window = CreateWindow(WINDOW_CLASS_NAME, MAIN_WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    return window;
}

LRESULT CALLBACK HandleNotification(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SOCKET:
        return OnSocketChangeState((SOCKET)wParam, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam), hWnd);
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        return OnDestroyWindow();
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void EnableConsole()
{
    // Each process can have at most 1 console. This function create new one for current process
    // Initialize standard input/output/error handle for created console
    AllocConsole();
    // Use HANDLE oConsole = GetStdHandle(STD_OUTPUT_HANDLE) is similar
    // STD_OUTPUT_HANDLE (int) = "CONOUT$"
    // and Use WriteConsole with this handle to print.
    FILE* outStream;
    freopen_s(&outStream, "CONOUT$", "w", stdout);
}

#pragma endregion

#pragma region Handle Requests

int HandleUploadRequest(SOCKETEX* socketex, char* arguments, uint arguments_length)
{
    if (socketex->temp_file_path == NULL) {
        time_t current = time(NULL);
        socketex->temp_file_path = Clone(DEFAULT_TEMP_FOLDER, strlen(DEFAULT_TEMP_FOLDER));
        char* current_str = (char*)malloc(20);
        ctime_s(current_str, 20, &current);
        strcat_s(socketex->temp_file_path, strlen(socketex->temp_file_path), current_str);
    }

    if (arguments_length != 0) { 
        FILE* tempfp = OpenFile(socketex->temp_file_path, "a");
        WriteToFile(tempfp, arguments_length, arguments);
        CloseFile(tempfp);
    }
    else { // upload end -> send result
        CreateThread(socketex);
    }
    return 1;
}

int HandleRequest(SOCKETEX* socketex)
{
    char* request, * arguments;
    uint arguments_length;
    int status = 1;
    status = SegmentationReceive(socketex->socket, &request);
    if (status != 1) {
        free(request);
        return status;
    }

    // Handle request
    int command = ExtractMessage(request, &arguments, &arguments_length);
    if (command == MC_ENCRYPT) {
        socketex->request_type = RT_ENCRYPT;
        socketex->key = ToUnsignedInt(arguments);
    }
    else if (command == MC_DECRYPT) {
        socketex->request_type = RT_DECRYPT;
        socketex->key = ToUnsignedInt(arguments);
    }
    else if (command == MC_UPLOAD) {
        status = HandleUploadRequest(socketex, arguments, arguments_length);
    }
    else {
        MESSAGE response = CreateMessage(MC_ERROR, NULL, 0);
        SegmentationSend(socketex->socket, response, (int)strlen(response) + 1, NULL);
    }
    free(request);

    return status;
}

#pragma endregion

#pragma region Thread and Session

char* ProcessData(int request_type, int key, FILE* tempfp, size_t* oresult_len, int* ocontinue)
{
    char* buffer = (char*)malloc(FILE_BUFF_MAX_SIZE);
    char* result = NULL;
    size_t read_count;
    int ret = ReadFromFile(tempfp, FILE_BUFF_MAX_SIZE, &buffer, &read_count);

    if (ocontinue != NULL)
        *ocontinue = 1;

    if (ret == 0) {
        if (ocontinue != NULL)
            *ocontinue = 0;
        return NULL;
    }
        
    if (request_type == RT_ENCRYPT) {
        result = EncryptShiftCipher(key, buffer, read_count);
    }
    else if (request_type == RT_DECRYPT) {
        result = DecryptShiftCipher(key, buffer, read_count);
    }
    free(buffer);
    if (oresult_len != NULL)
        *oresult_len = read_count;
    return result;
}


unsigned __stdcall SendResult(void* arguments)
{
    SOCKETEX* socketex = (SOCKETEX*)arguments;
    MESSAGE response;
    {
        int _continue = 1;
        char* result;
        size_t result_len;
        int status;
        int sent_byte;

        FILE* tempfp = OpenFile(socketex->temp_file_path, "r");
        if (tempfp != NULL) {
            while (_continue) {
                result = ProcessData(socketex->request_type, socketex->key, tempfp, &result_len, &_continue);
                response = CreateMessage(MC_UPLOAD, result, strlen(result));
                free(result);
                status = ReliableSend(socketex->socket, response, (int)strlen(response));
                DestroyMessage(response);
                if (status != 1)
                    return -1;
            }
            response = CreateMessage(MC_UPLOAD, NULL, 0);
            status = ReliableSend(socketex->socket, response, (int)strlen(response));
            DestroyMessage(response);
            if (status != 1)
                return -1;
            CloseFile(tempfp);
        }
    }
    return 0; // terminate thread
}

HANDLE CreateThread(SOCKETEX* socketex)
{
    HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, SendResult, (void*)socketex, 0, NULL);
    if (thread == 0) { // has error
#ifdef _ERROR_DEBUGGING
        if (errno == EAGAIN) {
            printf("[%s] %s\n", WARNING_FLAGS, _TOO_MANY_THREADS);
        }
        else if (errno == EACCES) {
            printf("[%s] %s\n", WARNING_FLAGS, _INSUFFICIENT_RESOURCES);
        }
#endif // _ERROR_DEBUGGING
    }
    return thread;
}

#pragma endregion

#pragma region Socket Extend

void FreeSocketExtend(int index)
{
    CloseSocket(SocketsManager[index].socket, CLOSE_SAFELY);
    SocketsManager[index].socket = INVALID_SOCKET;

    free(SocketsManager[index].temp_file_path);
}

int GetSocketExtendItem(SOCKET socket)
{
    int index = socket % MAX_CLIENTS;
    if (SocketsManager[index].socket == socket)
        return index;

    int i = index + 1;
    while (i != index) {
        if (SocketsManager[i].socket == socket)
            return i;
        ++i;
        i = (i < MAX_CLIENTS) ? i : i - MAX_CLIENTS;
    }
    return -1;
}

int AppendSocket(SOCKET socket)
{
    int index = socket % MAX_CLIENTS;
    if (SocketsManager[index].socket != INVALID_SOCKET) { // is set = true
        int i = index + 1;
        while (i != index) {
            if (SocketsManager[i].socket == INVALID_SOCKET)
                break;
            ++i;
            i = (i < MAX_CLIENTS) ? i : i - MAX_CLIENTS;
        }
        if (i == index) { // not found <=> full
#ifdef _ERROR_DEBUGGING
            printf("[%s] %s\n", WARNING_FLAGS, _TOO_MANY_CLIENTS);
#endif // _ERROR_DEBUGGING
            return 0;
        }
        index = i;
    }

    SocketsManager[index].socket = socket;
    SocketsManager[index].request_type = RT_INVALID;
    SocketsManager[index].key = 0;
    SocketsManager[index].temp_file_path = NULL;
    return 1;
}

#pragma endregion

#pragma region Notification Handlers

int GetStatusOnSocketEvent(WORD events, WORD errors)
{
    if (events == FD_CLOSE) {
#ifdef _ERROR_DEBUGGING
        if (errors != 0) { // WSAENETDOWN || WSAECONNRESET || WSAECONNABORTED (Ctrl + C / Close Window)
            printf("[%s:%d] %s\n", WARNING_FLAGS, errors, _CONNECTION_DROP);
        }
#endif
        return FD_CLOSE;
    }
    else if (events == FD_ACCEPT) {

        if (errors != 0) { // WSAENETDOWN
#ifdef _ERROR_DEBUGGING
            printf("[%s:%d] %s\n", ERROR_FLAGS, errors, _CONNECTION_DROP);
#endif
            return FD_CLOSE;
        }
        return FD_ACCEPT;
    }
    else if (events == FD_READ) {
        if (errors != 0) { // WSAENETDOWN
#ifdef _ERROR_DEBUGGING
            printf("[%s:%d] %s\n", ERROR_FLAGS, errors, _CONNECTION_DROP);
#endif
            return FD_CLOSE;
        }
        return FD_READ;
    }
    return 0;
}

int OnSocketChangeState(SOCKET socket, WORD events, WORD errors, HWND window)
{
    int status = GetStatusOnSocketEvent(events, errors);
    if (status == FD_CLOSE) {
        CloseSocket(socket, CLOSE_SAFELY);
    }
    else if (status == FD_ACCEPT) {
        SOCKET connector = GetConnectionSocket(socket); // socket = listener
        if (connector != INVALID_SOCKET) {
            RegisterSocketNotification(connector, window, FD_READ | FD_CLOSE);
            AppendSocket(connector);
        }
    }
    else if (status == FD_READ) {
        int index = GetSocketExtendItem(socket);
        if (index > -1) {
            int ret = HandleRequest(SocketsManager + index);
            if (ret == -1) {
                FreeSocketExtend(index);
            }
        }
    }
    return 0;
}

int OnDestroyWindow()
{
    CloseSocket(listener, CLOSE_SAFELY);
    WSCleanup();    
#ifdef _ERROR_DEBUGGING
    printf("[%s] Stopping...\n", INFO_FLAGS);
#endif // _ERROR_DEBUGGING
    PostQuitMessage(0);
    return 0;
}

int RegisterSocketNotification(SOCKET socket, HWND window, long events)
{

    int ret = WSAAsyncSelect(socket, window, WM_SOCKET, events);
    if (ret == SOCKET_ERROR) {
#ifdef _ERROR_DEBUGGING
        printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _ATTACH_NOTIFICATION_FAIL);
#endif // _ERROR_DEBUGGING
        return 0;
    }
    return 1;
}
#pragma endregion


