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
            SetSendBufferSize(socket, 3 * 4096); // config for all connectors get from this listener
            SetReceiveBufferSize(socket, 3 * 4096);

            ADDRESS server = CreateSocketAddress(server_ip, server_port);

            int try_establish = 0;
            do {
                if (EstablishConnection(socket, server)) {
                    try_establish = 0;
#ifdef _ERROR_DEBUGGING
                    printf("[%s] Ready to communicate...\n", INFO_FLAGS);
#endif // _ERROR_DEBUGGING
                    PrintMenu();

                    int request_type, key;
                    char* file;
                    int status = SUCCESS;

                    while (status != FATAL_ERROR) {
                        if ((status = HandleInput(&request_type, &key, &file)) == SUCCESS) {
                            if ((status = SendRequest(socket, request_type, key, file)) == SUCCESS) {
                                    status = HandleResponse(socket, request_type, file);
                            }
                        }
                        free(file);
                    }
                }
                // Handle establish fail
                else {
                    // Let user wait and try again
                    printf("[%s] Try establish the connection again? (y/n): ", INPUT_FLAGS);
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

#pragma region Handle Request & Response

int SendRequest(SOCKET socket, int request_type, int key, const char* file)
{
    int status = SUCCESS;
    if (request_type == RT_ENCRYPT || request_type == RT_DECRYPT) {
        // The first step message: Request type (Encrypt/Decrypt) | Key
        status = SendEncryptDecryptMessage(socket, request_type, key);
        if (status == SUCCESS) {
            status = ReceiveACK(socket);
        }
        if (status == SUCCESS) {

            FILE* fp = OpenFile(file, FOM_READ);
            
            if (fp != NULL) {
                stream read;
                uint read_count;
                int read_status;
                while (1) {
                    read_status = ReadFromFile(fp, MESSAGE_PAYLOAD_MAX_SIZE, &read, &read_count);
                    if (read_status != FATAL_ERROR) { // SUCCESS or FAIL (EOF)
                        // The second step messages: The content of the file
                        status = SendDataMessage(socket, read, read_count);
                        if (status == SUCCESS) {
                            status = ReceiveACK(socket);
                        }
                    }
                    DestroyStream(read);

                    if (status == SUCCESS && read_status == FAIL) {
                        // The third step messages: The upload end message
                        status = SendDataMessage(socket, NULLSTR, 0);
                    }
                    
                    if (read_status != SUCCESS || status != SUCCESS)
                        break;
                }

                CloseFile(fp);
            }
            else {
                status = FAIL;
            }
        }
    }
    return status;
}

int HandleResponse(SOCKET socket, int request_type, const char* file)
{
    // Create result file
	uint file_len = strlen(file);
    char result_file[USER_INPUT_MAX_SIZE + FILE_EXTENSION_SIZE];
    memcpy_s(result_file, file_len, file, file_len);
	memcpy_s(result_file + file_len, FILE_EXTENSION_SIZE,
		request_type == RT_ENCRYPT ? ENCRYPT_FILE_EXTENSION : DECRYPT_FILE_EXTENSION, FILE_EXTENSION_SIZE);
    if (IsExist(result_file)) {
        printf("[%s] The file %s has already existed. Please restore the data on file before going further!\n", OUTPUT_FLAGS, result_file);
        char c;  scanf_s("%c", &c, 1);
        RemoveFile(result_file);
    }
    // receive
    stream payload;
    uint payload_len;
    int code;

    int _continue = 1, status;
	while (_continue) {
        _continue = 0;
        status = ReceiveMessage(socket, &code, &payload, &payload_len);
        if (status == SUCCESS) {
            if (code == MC_DATA) {
                if (payload_len > 0) {
                    _continue = 1;
                    FILE* fp = OpenFile(result_file, FOM_APPEND);
                    if (fp != NULL) {
                        WriteToFile(fp, payload_len, payload);
                        CloseFile(fp);
                    }
                    status = SendACK(socket);
                }
                else { // receive upload end message -> stop
                    printf("[%s] Handle request success. Check result file: %s\n", OUTPUT_FLAGS, result_file);
                }
            }
            else if (code == MC_ERROR) {
                printf("[%s] Fail to process request %s on file '%s'.\n", OUTPUT_FLAGS,
                    (request_type == RT_ENCRYPT ? "ENCRYPT" : "DECRYPT"), file);
            }
        }
        DestroyStream(payload);
	}
    return status;
}

#pragma endregion

#pragma region Handle I/O

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
    if (okey == NULL || ofile == NULL)
        return INVALID_ARGUMENTS;
    *ofile = NULL;
    char c;
    char file[USER_INPUT_MAX_SIZE];
    int key;

    printf("[%s] [%s] Enter the file path: ", INPUT_FLAGS, option_str);
    scanf_s("%c", &c, 1); //consume \n
    gets_s(file, USER_INPUT_MAX_SIZE);

    if (strlen(file) > 0) {

        if (IsExist(file)) {
            // Enter key
            do {
                printf("[%s] [%s] Enter the key: ", INPUT_FLAGS, option_str);
                scanf_s("%d", &key);
                if (key < 0) {
                    printf("[%s] The key can not be less than 0.\n", OUTPUT_FLAGS);
                }
            } while (key < 0);
            scanf_s("%c", &c, 1); //consume \n
        }
        else {
            printf("[%s] Can not open file '%s' to read.\n", OUTPUT_FLAGS, file);
            return FAIL;
        }
    }
    else {
        printf("[%s] The file path can not be null.\n", OUTPUT_FLAGS);
        return FAIL;
    }
    *okey = key;
    *ofile = Clone(file, strlen(file) + 1);
    return SUCCESS;
}

int HandleInput(int* orequest_type, int* okey, char** ofile)
{
    if (orequest_type == NULL || okey == NULL || ofile == NULL)
        return INVALID_ARGUMENTS;
    *ofile = NULL;

    char c;
    int status = SUCCESS;
    printf("[%s] Enter your choice: ", INPUT_FLAGS);
    scanf_s("%c", &c, 1);
    if (c == '1') {
        *orequest_type = RT_ENCRYPT;
        status = GetRequest("Encrypt", okey, ofile);
    }
    else if (c == '2') {
        *orequest_type = RT_DECRYPT;
        status = GetRequest("Decrypt", okey, ofile);
    }
    else {
        status = FATAL_ERROR;
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

