#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>


#define SERVICE_NAME L"ServerChatService"


extern bool g_ServerRunning;
bool StartMyService();
void StopMyService();
void StartTcpServer();
void StopTcpServer();


