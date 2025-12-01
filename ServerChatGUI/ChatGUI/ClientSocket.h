
#pragma once
#include <winsock2.h>
#include <thread>

class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();

    bool Connect(const char* serverIp, int port);
    bool IsConnect();
    void Disconnect();

    bool Send(const void* data, int size);

    void SetCallback(void(*cb)(const char*, int));

private:
    SOCKET sock;
    bool isConnected;
    bool running;
    std::thread recvThread;
    void(*callback)(const char*, int);

    bool SendAll(const char* data, int size);
    bool SendPacket(const void* data, int size);
    bool RecvAll(char* buf, int len);
    void StartRecvThread();
};

