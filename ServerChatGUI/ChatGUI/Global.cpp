#include "pch.h"
#include "Global.h"
#include "ClientSocket.h"

ClientSocket g_socket;
CString g_username;
CString g_currentUsername;

HWND g_hwndMain = nullptr;
ChatScreen* g_currentChatScreen = nullptr;
RegisterScreen* g_currentRegisterScreen = nullptr;

