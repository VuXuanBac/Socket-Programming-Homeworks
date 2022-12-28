#include "Client.h"

int main(int argc, char* argv[])
{
    int server_port;
    IP server_ip;
    int is_ok = 1;
    // Handle command line
    if (ExtractCommand(argc, argv, &server_port, &server_ip) == 0) {
#ifdef _ERROR_DEBUGGING
        printf("[%s] %s\n", WARNING_FLAGS, _CONVERT_ARGUMENTS_FAIL);
#endif // _ERROR_DEBUGGING
        printf("[%s] Do you want to use default address? (y/n): ", INPUT_FLAGS);
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
        SOCKET socket = CreateSocket(TCP);
        if (socket != INVALID_SOCKET) {
            SetReceiveTimeout(socket, RECEIVE_TIMEOUT_INTERVAL);

            ADDRESS server = CreateSocketAddress(server_ip, server_port);

            int try_establish = 0;
            do {
                if (EstablishConnection(socket, server)) {
                    try_establish = 0;
#ifdef _ERROR_DEBUGGING
                    printf("[%s] Ready to communicate...\n", INFO_FLAGS);
#endif // _ERROR_DEBUGGING
                    PrintMenu();
                    int code, key; 
                    char* file;
                    int status = 1;
                    while (status != -1) {
                        if (HandleInput(&code, &key, &file)) {
                            status = Run(socket, code, key, file);
                            free(file);
                        }
                    }
                }
                // Handle establish fail
                else {
                    // Let user wait and try again
                    printf("[%s] Do you want to try establish the connection again? (y) or you can exit program (n). (y/n): ", INPUT_FLAGS);
                    char c;
                    scanf_s("%c", &c, 1);
                    try_establish = (c == 'y' || c == 'Y');
                    scanf_s("%c", &c, 1); // consume '\n'
                }
            } while (try_establish);

        }
        CloseSocket(socket, CLOSE_SAFELY, SD_BOTH);
        WSCleanup();
    }
#ifdef _ERROR_DEBUGGING
    printf("[%s] Stopping...\n", INFO_FLAGS);
#endif // _ERROR_DEBUGGING
    return 0;
}

#pragma region Handle Response



int Run(SOCKET socket, int request_type, int key, const char* file)
{
    int status = 1;
    if (request_type == RT_ENCRYPT || request_type == RT_DECRYPT) {
        MESSAGE request;
        request = CreateMessage(request_type == RT_ENCRYPT ? MC_ENCRYPT : MC_DECRYPT, key);
        status = ReliableSend(socket, request, strlen(request));
        free(request);
        if (status == 1) {
            FILE* fp = OpenFile(file, "r");
            char* read = (char*)malloc(FILE_BUFF_MAX_SIZE);
            size_t read_count;
            int read_status = 1;
            while (read_status == 1) {
                read_status = ReadFromFile(fp, FILE_BUFF_MAX_SIZE, &read, &read_count);
                if (read_status == -1)
                    break;
                request = CreateMessage(MC_UPLOAD, read, read_count);
                status = ReliableSend(socket, request, strlen(request));
                if (status != 1)
                    break;
                free(request);
            }
            if (read_status != -1 && status == 1) {
                request = CreateMessage(MC_UPLOAD, NULL, 0);
                status = ReliableSend(socket, request, strlen(request));
                free(request);
            }
            if (status == 1) {
                status = HandleResponse(socket, request_type, file);
            }
        }
    }
    return status;
}

int HandleResponse(SOCKET socket, int request_type, const char* file)
{
    size_t file_len = strlen(file) + 1;
    char* result_file = Clone(file, file_len);
    if (result_file != NULL) {
        strcat_s(result_file, file_len, request_type == RT_ENCRYPT ? ".enc" : ".dec");
        MESSAGE response = NULL;
        int status = SegmentationReceive(socket, &response);
        if (status == 1) {
            char* content;
            unsigned int length;
            int code = ExtractMessage(response, &content, &length);
            if (code == MC_UPLOAD && length > 0) {
                FILE* fp = OpenFile(result_file, "a");
                WriteToFile(fp, length, content);
                CloseFile(fp);
            }
        }
        DestroyMessage(response);
        return status;
    }
    return 0;
}

#pragma endregion

#pragma region Utilities

void PrintMenu()
{
    printf("\t############### COMMANDS ###############\n");
    printf("\t#    1. Encrypt File (Shift Cipher)    #\n");
    printf("\t#    2. Decrypt File (Shift Cipher)    #\n");
    printf("\t#    Other. Exit program               #\n");
    printf("\t########################################\n");
}

int GetRequest(const char* option_str, int* okey, char** ofile)
{
    char c;
    char file[USER_INPUT_MAX_SIZE];
    int key;

    printf("[%s] [%s] Enter the file path: ", INPUT_FLAGS, option_str);
    scanf_s("%c", &c, 1); //consume \n
    gets_s(file, USER_INPUT_MAX_SIZE);

    int request_len = (int)strlen(file);
    if (request_len > 0) {

        if (IsExist(file)) {
            // Enter key
            scanf_s("%c", &c, 1); //consume \n
            do {
                printf("[%s] [%s] Enter the key: ", INPUT_FLAGS, option_str);
                scanf_s("%d", &key, sizeof(key));
                if (key < 0) {
                    printf("[%s] The key can not be less than 0.\n", OUTPUT_FLAGS);
                }
            } while (key < 0);
        }
        else {
            printf("[%s] Can not open file '%s' to read.\n", OUTPUT_FLAGS, file);
            return 0;
        }
    }
    else {
        printf("[%s] The file path can not be null.\n", OUTPUT_FLAGS);
        return 0;
    }
    *okey = key;
    *ofile = Clone(file, (int)strlen(file) + 1);
    return 1;
}

int HandleInput(int* ocode, int* okey, char** ofile)
{
    if (ocode == NULL || okey == NULL || ofile == NULL)
        return 0;
    *ofile = NULL;

    char c;
    int status = 1;
    printf("[%s] Enter your choice: ", INPUT_FLAGS);
    scanf_s("%c", &c, 1);
    if (c == '1') {
        *ocode = RT_ENCRYPT;
        status = GetRequest("Encrypt", okey, ofile);
    }
    else if (c == '2') {
        *ocode = RT_DECRYPT;
        status = GetRequest("Decrypt", okey, ofile);
    }
    else {
        status = 0;
    }
    return status;
}

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

#pragma endregion

