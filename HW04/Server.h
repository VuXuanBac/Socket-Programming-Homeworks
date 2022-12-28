#pragma once

#pragma region Header Declarations

#define _WINSOCK_DEPRECATED_NO_WARNINGS // ignore warning on deprecated WSAAsynsSelect()

#include "resource.h"
#include "framework.h"

#include <stdio.h>
#include <process.h>
#include <time.h>

#include "ApplicationCommon.h"

#pragma endregion

#pragma region Constant Definitions

#define WINDOW_CLASS_NAME L"VUXUANBAC"
#define MAIN_WINDOW_TITLE L"Encrypt/Decrypt File Server"

#define DEFAULT_TEMP_FOLDER ".//temp//"

#define WM_SOCKET 0xBA98 // message code for notifications from current application

#define MAX_CLIENTS 1024

#pragma endregion

#pragma region Type Definitions

typedef struct _socket_extend {

	SOCKET socket; // Managed socket

	int request_type; // RT_ENCRYPT || RT_DECRYPT

	uint key; // encryption|decryption key

	char* temp_file_path; // The path to the temp file.

} SOCKETEX;

#pragma endregion

#pragma region Function Declarations

#pragma region Window Desktop Common

/// <summary>
///  A window class defines a set of behaviors that several windows might have in common.
///  It's not a OOP Class, it is a data structure used internally by the operating system. 
///  Every window must be associated with a window class.
///  Window classes are registered with the system at run time.
///  *** Default class name: See WINDOW_CLASS_NAME
///  *** Default Window Proc (Handle Notification Messages): HandleNotification
/// </summary>
/// <param name="hInstance">The current process's handle</param>
/// <returns>Identifier for the Window Class or 0 if fail</returns>
ATOM RegisterWindowClass(HINSTANCE hInstance);

/// <summary>
/// 
/// </summary>
/// <param name="hInstance">The current process's handle</param>
/// <param name="nCmdShow">Command Parameter for ShowWindow() function: Minimize, Maximize</param>
/// <returns></returns>
HWND CreateWindowInstance(HINSTANCE hInstance, int nCmdShow);

/// <summary>
/// Handle Events/Notifications from User and System post/send to a Window Instance
/// *** WM_CLOSE	: User clicks Close Button/ Alt + F4. Handled before Window disappears from Screen
/// *** WM_DESTROY	: Appear after Window removed from Screen. Call OnDestroyWindow()
/// 
/// *** WM_SOCKET	: Current Application Message.
/// *** Others		: Use default action for these messages.
/// </summary>
/// <param name="hWnd">The window instance (Target of the notification message)</param>
/// <param name="message">The message code of the notification</param>
/// <param name="wParam">Additional Data for message (UINT_PTR)</param>
/// <param name="lParam">Additional Data for message (LONG_PTR)</param>
/// <returns>Response to a particular message</returns>
LRESULT CALLBACK HandleNotification(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/// <summary>
/// Create new Console Window attached on this process and Open output stream on it.
/// </summary>
void EnableConsole();

#pragma endregion

#pragma region Handle Requests

int HandleUploadRequest(SOCKETEX* socketex, char* arguments, uint arguments_length);

int HandleRequest(SOCKETEX* socketex);
#pragma endregion

#pragma region Thread and Session

char* ProcessData(int request_type, int key, FILE* tempfp, size_t* oresult_len, int* ocontinue);

unsigned __stdcall SendResult(void* arguments);

HANDLE CreateThread(SOCKETEX* socketex);
#pragma endregion

#pragma region Socket Extend

void FreeSocketExtend(int index);

int GetSocketExtendItem(SOCKET socket);

int AppendSocket(SOCKET socket);

#pragma endregion

#pragma region Notification Handlers
int GetStatusOnSocketEvent(WORD events, WORD errors);

int OnSocketChangeState(SOCKET socket, WORD events, WORD errors, HWND window);

/// <summary>
/// Call on HandleNotification when WM_DESTROY message raise.
/// </summary>
/// <return>0 always</return>
int OnDestroyWindow();

int RegisterSocketNotification(SOCKET socket, HWND window, long events);
#pragma endregion

int PrepareForCommunicationOnSocket(HWND window);
#pragma endregion
