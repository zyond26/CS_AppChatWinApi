#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <ctime>

#include "ClientHandler.h"
#include "Struct.h"
#include "Database.h"
#include "UserManager.h"

bool SendAll(SOCKET s, const char* data, int len)
{
    int total = 0;
    while (total < len)
    {
        int ret = send(s, data + total, len - total, 0);
        if (ret <= 0) return false;
        total += ret;
    }
    return true;
}

bool SendPacket(SOCKET s, const char* data, int len)
{
    if (!SendAll(s, (char*)&len, 4))
        return false;
    return SendAll(s, data, len);
}

bool RecvAll(SOCKET s, char* buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int ret = recv(s, buf + total, len - total, 0);
        if (ret <= 0) return false;
        total += ret;
    }
    return true;
}

bool RecvPacket(SOCKET s, char* buffer, int maxSize, int& outSize)
{
    int len = 0;

    if (!RecvAll(s, (char*)&len, 4))
        return false;

    if (len <= 0 || len > maxSize)
        return false;

    if (!RecvAll(s, buffer, len))
        return false;

    outSize = len;
    return true;
}

DWORD WINAPI ClientHandler(LPVOID lpParam)
{
    SOCKET client = (SOCKET)lpParam;
    char buffer[4096];
    std::wstring currentUsername;

    OutputDebugStringA("ClientHandler: New client connected\n");

    while (true)
    {
        int bytesReceived = 0;
        if (!RecvPacket(client, buffer, sizeof(buffer), bytesReceived))
        {
            OutputDebugStringA("ClientHandler: RecvPacket failed -> disconnect\n");
            break;
        }

        if (bytesReceived < sizeof(PacketType))
            continue;

        PacketType type = *(PacketType*)buffer;

        switch (type)
        {
        case PACKET_LOGIN:
        {
            PacketLogin* p = (PacketLogin*)buffer;
            PacketLoginResult res{};
            res.type = PACKET_LOGIN_RESULT;

            if (!Database::CheckLogin(p->username, p->password))
            {
                res.success = false;
                wcscpy_s(res.msg, L"Sai tài khoản hoặc mật khẩu!");
                SendPacket(client, (char*)&res, sizeof(res));
                break;
            }

            if (!UserManager::AddUser(p->username, client))
            {
                res.success = false;
                wcscpy_s(res.msg, L"Tài khoản đang đăng nhập ở nơi khác!");
                SendPacket(client, (char*)&res, sizeof(res));
                break;
            }

            currentUsername = p->username;
            res.success = true;
            wcscpy_s(res.msg, L"Đăng nhập thành công!");
            SendPacket(client, (char*)&res, sizeof(res));

            {
                auto online = UserManager::GetOnlineUsernames();

                PacketUserList pkt{};
                pkt.type = PACKET_USER_LIST;
                pkt.count = (int)online.size();

                for (int i = 0; i < pkt.count; i++)
                {
                    wcsncpy_s(pkt.users[i].username, online[i].c_str(), MAX_NAME);
                    pkt.users[i].userId = Database::GetUserId(online[i].c_str());
                    pkt.users[i].online = true;
                }

                int size = sizeof(PacketType) + sizeof(int) + pkt.count * sizeof(UserInfo);
                SendPacket(client, (char*)&pkt, size);
            }

            {
                PacketUserStatus st{};
                st.type = PACKET_USER_STATUS;
                st.userId = Database::GetUserId(currentUsername.c_str());
                wcscpy_s(st.username, currentUsername.c_str());
                st.isOnline = true;

                UserManager::Broadcast((char*)&st, sizeof(st), currentUsername);
            }
            break;
        }

        case PACKET_REGISTER:
        {
            PacketRegister* p = (PacketRegister*)buffer;
            PacketRegisterResult res{};
            res.type = PACKET_REGISTER_RESULT;

            if (Database::RegisterUser(p->username, p->email, p->password))
            {
                res.success = true;
                wcscpy_s(res.msg, L"Đăng ký thành công!");
            }
            else
            {
                res.success = false;
                wcscpy_s(res.msg, L"Tên đăng nhập đã tồn tại!");
            }

            SendPacket(client, (char*)&res, sizeof(res));
            break;
        }

        case PACKET_MESSAGE:
        {
            PacketMessage* p = (PacketMessage*)buffer;

            int senderId = Database::GetUserId(currentUsername.c_str());
            int receiverId = Database::GetUserId(p->receiver);

            if (receiverId <= 0)
            {
                PacketMessageResult r{};
                r.type = PACKET_MESSAGE_RESULT;
                r.success = false;
                wcscpy_s(r.msg, L"Người nhận không tồn tại!");
                SendPacket(client, (char*)&r, sizeof(r));
                break;
            }

            Database::SaveMessage(senderId, receiverId, p->message, p->timestamp);

            SendPacket(client, (char*)p, sizeof(PacketMessage));

            SOCKET recvSocket = UserManager::GetUserSocket(p->receiver);
            if (recvSocket != INVALID_SOCKET)
                SendPacket(recvSocket, (char*)p, sizeof(PacketMessage));

            PacketMessageResult ok{};
            ok.type = PACKET_MESSAGE_RESULT;
            ok.success = true;
            wcscpy_s(ok.msg, L"Tin nhắn đã gửi.");
            SendPacket(client, (char*)&ok, sizeof(ok));

            break;
        }

        case PACKET_REQUEST_HISTORY:
        {
            PacketRequestHistory* req = (PacketRequestHistory*)buffer;
            int userA = Database::GetUserId(currentUsername.c_str());
            int userB = req->targetId;

            MessageHistory his[200];
            int count = 0;

            if (Database::GetMessageHistory(userA, userB, his, 200, &count))
            {
                PacketMessageHistory pkt{};
                pkt.type = PACKET_MESSAGE_HISTORY;
                pkt.count = count;

                for (int i = 0; i < count; i++)
                    pkt.messages[i] = his[i];

                int size = sizeof(PacketType) + sizeof(int) + count * sizeof(MessageHistory);
                SendPacket(client, (char*)&pkt, size);
            }
            else
            {
                PacketMessageHistory pkt{};
                pkt.type = PACKET_MESSAGE_HISTORY;
                pkt.count = 0;
                SendPacket(client, (char*)&pkt, sizeof(PacketType) + sizeof(int));
            }
            break;
        }

        case PACKET_USER_LIST:
        {
            UserInfo users[MAX_CLIENTS];
            int count = 0;

            Database::GetAllUsers(users, MAX_CLIENTS, &count);

            PacketUserList pkt{};
            pkt.type = PACKET_USER_LIST;
            pkt.count = count;
            memcpy(pkt.users, users, count * sizeof(UserInfo));

            int size = sizeof(PacketType) + sizeof(int) + count * sizeof(UserInfo);
            SendPacket(client, (char*)&pkt, size);
            break;
        }

        case PACKET_LOGOUT:
        {
            UserManager::RemoveUser(currentUsername);

            PacketUserStatus st{};
            st.type = PACKET_USER_STATUS;
            st.isOnline = false;
            wcscpy_s(st.username, currentUsername.c_str());

            UserManager::Broadcast((char*)&st, sizeof(st), currentUsername);

            closesocket(client);
            return 0;
        }
        } 
    }

    if (!currentUsername.empty())
    {
        UserManager::RemoveUser(currentUsername);

        PacketUserStatus st{};
        st.type = PACKET_USER_STATUS;
        st.isOnline = false;
        wcscpy_s(st.username, currentUsername.c_str());

        UserManager::Broadcast((char*)&st, sizeof(st), currentUsername);
    }
    closesocket(client);
    return 0;
}
