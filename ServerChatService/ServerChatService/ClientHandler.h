#pragma once
#include <Windows.h>

DWORD WINAPI ClientHandler(LPVOID lpParam);

bool SendAll(SOCKET s, const char* data, int len);
  
bool SendPacket(SOCKET s, const char* data, int len);

bool RecvAll(SOCKET s, char* buf, int len);
    
bool RecvPacket(SOCKET s, char* buffer, int bufferSize, int& outSize);