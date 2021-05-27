#include "Server.h"

bool Server::stopping = false;

std::map<std::string, std::string> Server::responses = {
    {"Hello World!", "Hi to you too!"},
    {"hello", "hello to you too!"}
};

std::string Server::DISCONNECT_MSG = "exit";;

std::string Server::ParseMessage(SOCKET id, std::string msg) {
    std::cout << "[Server]" << " << " << std::hex << "[Client 0x" << id << "] \"" << msg << "\"" << std::endl;

    auto itr = responses.find(msg);
    std::string ret = (itr != responses.end()) ? itr->second : std::string("Default response");

    std::cout << "[Server]" << " >> " << std::hex << "[Client 0x" << id << "] \"" << ret << "\"" << std::endl;
    return ret;
}

bool Server::SendMsg(SOCKET ConnectSocket, std::string send_msg) {
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

void Server::HandleClient(SOCKET ClientSocket) {

    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        return;
    }

    int iResult;
    char recvbuf[BUFLEN];
    int recvbuflen = BUFLEN;

    std::string message;
    while (!stopping) {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0); // iresult = bytes rcvd
        if (iResult > 0) {
            for (int i = 0; i < iResult; i++) {
                char c = recvbuf[i];

                if (c == ';') {
                    if (message == DISCONNECT_MSG) {
                        printf("Client Disconnected.\n");
                        closesocket(ClientSocket);
                        return;
                    }
                    auto response = ParseMessage(ClientSocket, message);

                    if (!SendMsg(ClientSocket, response)) {
                        printf("Failed to send message to client.\n");
                        return;
                    }

                    message.clear();
                }
                else {
                    message += c;
                }
            }
        }
        else if (iResult == 0) {
            printf("Client Disconnected.\n");
            closesocket(ClientSocket);
            return;
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            return;
        }
    };
}

bool Server::Start() {
    WSADATA wsaData;
    int iResult;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return false;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, PORT_ADDR, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return false;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }

    while (!(GetAsyncKeyState(VK_END) & 0x01)) {
        auto client_socket = accept(ListenSocket, NULL, NULL);
        if (client_socket < 0) continue;
        accept_threads.push_back(std::thread(HandleClient, client_socket));
    }

    return true;
}

bool Server::Stop() {
    stopping = true;
    for (auto& thread : accept_threads) {
        thread.join();
    }

    closesocket(ListenSocket);
    WSACleanup();
    return true;
}