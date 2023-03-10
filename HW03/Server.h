#pragma once

#pragma region Header Declarations
// define before header <Winsock2.h>
#define FD_SETSIZE 1024

#include <process.h>

#include "CommonHeader.h"

#pragma endregion

#pragma region Constants Definitions

#define MAX_CONNECTIONS SOMAXCONN
#define MAX_CLIENTS_PER_THREAD FD_SETSIZE

#define LINE_MAX_SIZE 1024
#define ACCOUNT_FILE_PATH "c://users//rsn//desktop//account.txt"

#define S_LOGIN_SUCC 10
#define S_ACCOUNT_LOCK 11
#define S_ACCOUNT_NOT_EXIST 12
#define S_ACCOUNT_LOGGEDIN 13
#define S_LOGGEDIN 14
#define S_POST_SUCC 20
#define S_NOT_LOGIN 21
#define S_LOGOUT_SUCC 30
#define S_UNREGCONIZE_COMMAND 99

#define SM_LOGIN_SUCC "Login successfully"
#define SM_ACCOUNT_LOCK "Account locked"
#define SM_ACCOUNT_NOT_EXIST "Account does not exist"
#define SM_ACCOUNT_LOGGEDIN "Another client is working on this account"
#define SM_LOGGEDIN "You already logged in"
#define SM_POST_SUCC "Post the article successfully"
#define SM_NOT_LOGIN "No permission because you are not logged in"
#define SM_LOGOUT_SUCC "Log out successfully"
#define SM_UNREGCONIZE_COMMAND "Unregconize command"

#define AS_FREE 0
#define AS_LOCK 1
#define AS_LOGGED_IN 2
#pragma endregion

#pragma region Type Definitions

#define SOCKETSET fd_set

/// <summary>
/// Account infos read from file
/// </summary>
typedef struct accountinfo {

	char* account; // User name

	int status; // Account status. See AS_ for some definitions of account status

	struct accountinfo* next; // Next account. Linked List

} ACCOUNTINFO;

/// <summary>
/// Manage sockets on a thread;
/// </summary>
typedef struct thread_sockets_manager {

	int free_index; // A index in "sockets" field is free. -1 if full

	SOCKET sockets[MAX_CLIENTS_PER_THREAD]; // Managed sockets
	//ACCOUNTINFO** accounts; // Account infos logged in on, arrays of pointers to ACCOUNTINFOs NODE.
	int accounts_status[MAX_CLIENTS_PER_THREAD]; // status of account running on the sockets

	SOCKETSET init_set; // Managed sockets. [Initialized set]

	SOCKETSET probe_set; // Managed sockets. [Probe set: The select result here]

	struct thread_sockets_manager* next; // Next thread's sockets manager. Linked list
} SOCKETSMANAGER;

#pragma endregion

#pragma region Function Declarations

/// <summary>
/// Set a socket for SOCKETSMANAGER.
/// </summary>
/// <param name="manager">The sockets manager</param>
/// <param name="socket">The socket want to add</param>
/// <returns>1 if add success. 0 if manager is NULL.</returns>
int SetSocket(SOCKETSMANAGER* manager, SOCKET socket);

/// <summary>
/// Find first SOCKETSMANAGER that have a room for new socket. If not found, create new and append to linked list
/// </summary>
/// <param name="ois_new">[Output] Is the SOCKETSMANAGER created</param>
/// <returns>Found or Created SOCKETSMANAGER</returns>
SOCKETSMANAGER* FindFirstFreeManagerOrCreateNew(int* ois_new);

/// <summary>
/// Create new SOCKETSMANAGER node and initialize all fields.
/// </summary>
/// <returns>Created SOCKETSMANAGER</returns>
SOCKETSMANAGER* CreateSocketsManager();

/// <summary>
/// Append an item to linked list. [Preserve following items]
/// </summary>
/// <param name="prev">The previous item. NULL if want to insert to head.</param>
/// <param name="current">The item want to append</param>
/// <returns>The appended item [current]</returns>
SOCKETSMANAGER* Append(SOCKETSMANAGER* prev, SOCKETSMANAGER* current);

/// <summary>
/// Create a SOCKETSMANAGER node.
/// </summary>
/// <returns>An SOCKETSMANAGER node that set 0 for "first" field and initialize "set" field</returns>
ACCOUNTINFO* CreateAccountInfo(const char* username, int namelen, int status);

/// <summary>
/// Free memory use for SOCKETSMANAGER linked list.
/// </summary>
/// <param name="first">The head of the linked list</param>
void FreeSocketsManagerList(SOCKETSMANAGER* first);

/// <summary>
/// Append an item to linked list. [Preserve following items]
/// </summary>
/// <param name="prev">The previous item. NULL if want to insert to head.</param>
/// <param name="current">The item want to append</param>
/// <returns>The appended item [current]</returns>
ACCOUNTINFO* Append(ACCOUNTINFO* prev, ACCOUNTINFO* current);

/// <summary>
/// Create a ACCOUNTINFO node.
/// </summary>
/// <param name="username">The name is assigned to "account" field</param>
/// <param name="namelen">The length of usernam used to assign</param>
/// <param name="status">The status is assigned to "status" field</param>
/// <returns>An ACCCOUNTINFO node that use INVALID_SOCKET for "socket" field and NULL for "next" field.</returns>
ACCOUNTINFO* CreateAccountInfo(const char* username, int namelen, int status);

/// <summary>
/// Find first ACCOUNTINFO node that has "account" field is equal to [username] argument. [Case-insensitive searching]
/// </summary>
/// <param name="start">The start node that searching begins</param>
/// <param name="username">The searching keyword</param>
/// <returns>The first found node. NULL if have no node satisfies</returns>
ACCOUNTINFO* FindFirstAccountInfo(ACCOUNTINFO* start, const char* username);

/// <summary>
/// Free memory use for ACCOUNINFO linked list.
/// </summary>
/// <param name="first">The head of the linked list</param>
void FreeAccountList(ACCOUNTINFO* first);

/// <summary>
/// Load ACCOUNTINFO nodes from file. [Set ACCOUNT_FILE_PATH]
/// </summary>
/// <param name="file">The path to the file want to read.</param>
/// <returns>1 if success. 0 if have some errors on file operations.</returns>
int LoadAccountList(const char* file);

/// <summary>
/// Extract command and arguments from a request.
/// </summary>
/// <param name="request">The input request.</param>
/// <param name="oarguments">[Output] The command arguments</param>
/// <returns>The command code. See C_ for some command codes and CM_ for some commands text</returns>
int ExtractRequestCommand(const char* request, char** oarguments);

/// <summary>
/// Bind a socket to an address [IPv4, Port]
/// </summary>
/// <param name="socket">The socket want to bind</param>
/// <param name="addr">A socket address [IPv4, Port]</param>
/// <returns>1 is bind successfully. 0 otherwise</returns>
int BindSocket(SOCKET socket, ADDRESS addr);

/// <summary>
/// Set listen state for a socket.
/// </summary>
/// <param name="socket">The socket used for listen connections</param>
/// <param name="connection_numbers">Size of socket connections pending queue. Default SOMAXCONN (Set by underlying service provider)</param>
/// <returns>1 if has no errors. 0 otherwise</returns>
int SetListenState(SOCKET socket, int connection_numbers = MAX_CONNECTIONS);

/// <summary>
/// Extract and Accept the first connection from listener socket pending queue.
/// This function blocks the program if the pending queue is empty
/// </summary>
/// <param name="listener">The listener socket used to listen connections</param>
/// <param name="osender_address">The address of the process establishes the connection (by connect())</param>
/// <returns>A socket for the top connection on pending queue.</returns>
SOCKET GetConnectionSocket(SOCKET listener, ADDRESS* osender_address = NULL);

/// <summary>
/// Check if a socket set have any sockets can receive a message.
/// </summary>
/// <param name="sockets">The checked sockets</param>
/// <returns>The sockets that can receive</returns>
SOCKETSET* GetCanReadSockets(SOCKETSET* sockets);

/// <summary>
/// Create and Begin new thread for communicating on multiple socket managed by a SOCKETMANAGER
/// </summary>
/// <param name="manager">The sockets manager</param>
/// <returns>The thread handle</returns>
HANDLE CreateThreadForSocketsManager(SOCKETSMANAGER* manager);

/// <summary>
/// Callback method running on another thread that created by CreateThreadForSocketsManager()
/// </summary>
/// <param name="arguments">A pointer to sockets manager. [Cast directly]</param>
/// <returns>0. [The thread is also terminated]</returns>
unsigned __stdcall Run(void* arguments);

/// <summary>
/// Processing the post request
/// </summary>
/// <param name="socket">The connected socket identify the client</param>
/// <param name="arguments">The arguments for post request [The article want to post to server]</param>
/// <param name="ioaccount_status">[Input/Output] The status of account working on the socket.</param>
/// <returns>The response message for client</returns>
MESSAGE HandlePostRequest(SOCKET socket, const char* arguments, int* ioaccount_status);

/// <summary>
/// Processing the login request
/// </summary>
/// <param name="socket">The connected socket identify the client</param>
/// <param name="arguments">The arguments for login request [The username want to login]</param>
/// <param name="ioaccount_status">[Input/Output] The status of account working on the socket.</param>
/// <returns>The response message for client</returns>
MESSAGE HandleLoginRequest(SOCKET socket, const char* arguments, int* ioaccount_status);

/// <summary>
/// Processing the logout request
/// </summary>
/// <param name="socket">The connected socket identify the client</param>
/// <param name="ioaccount_status">[Input/Output] The status of account working on the socket.</param>
/// <returns>The response message for client</returns>
MESSAGE HandleLogoutRequest(SOCKET socket, int* ioaccount_status);

/// <summary>
/// Handle request: Read requests from buffer, Processing requests and Send response back.
/// </summary>
/// <param name="socket">The connected socket to the remote process</param>
/// <returns>1 if have no errors. 0 if request cant be processed completely. 
/// -1 if have errors and the socket cant be used anymore (lost connection to remote process)</returns>
int HandleRequest(SOCKET socket, int* ioaccount_status);

/// <summary>
/// Extract port number from command-line arguments.
/// If has error, use default port number [predefined, See: DEFAULT_PORT]
/// </summary>
/// <param name="argc">Number of Arguments [From main()]</param>
/// <param name="argv">Arguments value [From main()]</param>
/// <param name="oport">[Output] The extracted port number</param>
/// <returns>1 if extract successfully. 0 otherwise</returns>
int ExtractCommand(int argc, char* argv[], int* oport);

/// <summary>
/// Create a INADDR_ANY IP Address
/// </summary>
/// <returns>The INADDR_ANY IP</returns>
IP CreateDefaultIP();

/// <summary>
/// Create a Message object that contains a status.
/// The first STATUS_LENGTH byte of Message is the status code: See S_ for some status code
/// </summary>
/// <param name="status">The status code</param>
/// <param name="message">The response message. Use NULL if have no additional message</param>
/// <returns>A command message. NULL if memory allocation fail</returns>
MESSAGE CreateMessage(int status, const char* message);

/// <summary>
/// Free memory for Message object
/// </summary>
/// <param name="m">The message want to free</param>
void DestroyMessage(MESSAGE m);

/// <summary>
/// Compare two string [case-insensitive]
/// </summary>
/// <param name="first">The first string</param>
/// <param name="second">The second string</param>
/// <param name="length">The compare length. The actual compare length will always be not exceed the length of input strings</param>
/// <returns>0 if equal. 1 if [first] is greater [the [second] go first alphabetically], -1 if [second] is greater</returns>
int ICompare(const char* first, const char* second, int length = 0);
#pragma endregion