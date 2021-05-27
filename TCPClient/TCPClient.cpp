#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <algorithm>


constexpr int BUFLEN = 256;
constexpr auto PORT_ADDR = "1276";

char recvbuf[BUFLEN];
int recvbuflen = BUFLEN;

std::string AskLine() {
    std::cout << "Send message ('exit' to disconnect): " << std::endl;
    std::cout << ">> ";

    std::string send_msg;
    getline(std::cin, send_msg);
    return send_msg;
}

bool SendMsg(SOCKET ConnectSocket, std::string send_msg) {
    send_msg.erase(std::remove(send_msg.begin(), send_msg.end(), ';'), send_msg.end());
    send_msg += ";";
    auto str = send_msg.c_str();
    int index = 0;
    int left = strlen(str);

    while (left > 0) {
        auto sendResult = send(ConnectSocket, &str[index], left, 0);
        if (sendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return false;
        }

        index += sendResult;
        left -= sendResult;
    }

    return true;
}

bool WaitForMsg(SOCKET ConnectSocket) {
    std::string message;
    int iResult;

    while (true) {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            for (int i = 0; i < iResult; i++) {
                if (recvbuf[i] == ';') {
                    std::cout << "<< " << message << std::endl;
                    return true;
                }
                else {
                    message += recvbuf[i];
                }
            }
        }
        else if (iResult == 0) {
            printf("Client Disconnected.\n");
            closesocket(ConnectSocket);
            return false;
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            return false;
        }
    }
    return false;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = NULL;
    iResult = getaddrinfo(argv[1], PORT_ADDR, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ServerSocket = INVALID_SOCKET;
    struct addrinfo* ptr = NULL;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        ServerSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ServerSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        iResult = connect(ServerSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ServerSocket);
            ServerSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ServerSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    while (true) {
        auto send_msg = AskLine();

        if (!SendMsg(ServerSocket, send_msg))
            return 1;

        if (!WaitForMsg(ServerSocket))
            return 1;
    };

    iResult = shutdown(ServerSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }


    closesocket(ServerSocket);
    WSACleanup();

    return 0;
}
