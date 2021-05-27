#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <vector>
#include <iostream>
#include <ws2tcpip.h>
#include <thread>
#include <map>

constexpr int BUFLEN = 256;
constexpr auto PORT_ADDR = "1276";

class Server
{
    SOCKET ListenSocket = INVALID_SOCKET;

    static bool stopping;
    std::vector<std::thread> accept_threads;

    static std::map<std::string, std::string> responses;
    static std::string DISCONNECT_MSG;

    static std::string ParseMessage(SOCKET id, std::string msg);

    static bool SendMsg(SOCKET ConnectSocket, std::string send_msg);
    static void HandleClient(SOCKET ClientSocket);

public:
    bool Start();
    bool Stop();
};

