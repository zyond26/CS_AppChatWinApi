#pragma once
#include "sqlite3.h"
#include <mutex>
#include "Struct.h"
#include "vector"


class Database {
private:
    static sqlite3* db;
    static std::recursive_mutex dbMutex;  

public:
    static bool Init();
    static bool RegisterUser(const wchar_t* username, const wchar_t* email, const wchar_t* password);
    static bool CheckLogin(const wchar_t* username, const wchar_t* password);
    static bool SaveMessage(int senderId, int receiverId, const wchar_t* message, time_t timestamp);
    static bool GetMessageHistory(int user1, int user2, MessageHistory* history, int maxCount, int* actualCount);

    static int GetUserId(const wchar_t* username);
    static bool GetUsername(int userId, wchar_t* username, size_t size);

    static bool UserExists(const wchar_t* username);

    //static void DebugMessages();

    static bool GetAllUsers(UserInfo* users, int maxCount, int* actualCount);

    static bool GetFullHistory(MessageHistory* list, int maxCount, int* actual);



    static void Close();

private:
    static bool Exec16(const wchar_t* sql);  
};