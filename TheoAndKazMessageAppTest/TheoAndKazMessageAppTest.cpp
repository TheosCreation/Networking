#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

std::vector<SOCKET> clients;
std::vector<std::string> clientNames;

bool InitWSA() {
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2); //winsock version 2.2

    int result = WSAStartup(wVersionRequested, &wsaData);
    if (result != 0) {
        printf("WSAStartUp failed %d\n", result);
        return false;
    }

    //check version exists. ints we check against should match above
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("Error: Version is not available\n");
        WSACleanup();
        return false;
    }
    return true;
}

std::string GetNameBySocket(SOCKET clientSock) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == clientSock) {
            return clientNames[i];
        }
    }
    return "Unknown";
}

void RemoveClient(SOCKET clientSock) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == clientSock) {
            closesocket(clientSock);
            printf("[%s] disconnected.\n", clientNames[i].c_str());
            clientNames.erase(clientNames.begin() + i);
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

void BroadcastMessage(SOCKET senderSock, const char* buffer, int bytesReceived) {
    std::string senderName = GetNameBySocket(senderSock);
    std::string message = "[" + senderName + "]: " + std::string(buffer, bytesReceived);

    for (size_t i = 0; i < clients.size(); ++i) {
        // Skip sending the message back to the sender
        if (clients[i] == senderSock) {
            continue;
        }

        // Send the formatted message to each connected client
        send(clients[i], message.c_str(), message.length(), 0);
    }
}

void ReceiveMessages(SOCKET clientSock) {
    char buffer[4096];
    while (true) {
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            printf("[%s] disconnected.\n", GetNameBySocket(clientSock).c_str());
            RemoveClient(clientSock);
            break;
        }
        printf("[%s]: %s\n", GetNameBySocket(clientSock).c_str(), buffer);
        // Broadcast the message to all other clients
        BroadcastMessage(clientSock, buffer, bytesReceived);
    }
}

void Client() {
    SOCKET clientSock;

    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == INVALID_SOCKET) {
        printf("Error in socket(), Error code %d\n", WSAGetLastError());
        return;
    }

    sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(15366); // Change to server port
    InetPton(AF_INET, L"127.0.0.1", &recvAddr.sin_addr.S_un.S_addr);

    int status = connect(clientSock, (sockaddr*)&recvAddr, sizeof(recvAddr));
    if (status == SOCKET_ERROR) {
        printf("Error in connect(), Error code %d\n", WSAGetLastError());
        closesocket(clientSock);
        return;
    }

    // Connected to server
    printf("Connected to server.\n");

    char name[256];
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0'; // Remove newline character
    send(clientSock, name, strlen(name), 0);

    std::string clientName = "[" + std::string(name) + "]: ";
    char message[256];
    while (true) {
        printf("Enter message: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0'; // Remove newline character
        if (strcmp(message, "exit") == 0)
            break;
        std::string fullMessage = clientName + message;
        send(clientSock, fullMessage.c_str(), fullMessage.length(), 0);
    }

    closesocket(clientSock);
}

void ClientHandler(SOCKET clientSock) {
    char clientName[256];
    int bytesReceived = recv(clientSock, clientName, sizeof(clientName) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }
    clientName[bytesReceived] = '\0'; // Add null terminator to properly terminate the string
    std::string name(clientName);
    clientNames.push_back(name);
    clients.push_back(clientSock);
    printf("[%s] connected.\n", name.c_str());
    ReceiveMessages(clientSock);
}

void Server() {
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        printf("Error in socket(). Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(15366); // Change to desired port
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int bound = bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (bound == SOCKET_ERROR) {
        printf("Error in bind(). Error code: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    printf("Listening...\n");
    int status = listen(serverSock, 5);
    if (status == SOCKET_ERROR) {
        printf("Error in listen(). Error code: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    printf("Accepting...\n");
    sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);
    while (true) {
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrlen);
        if (clientSock == INVALID_SOCKET) {
            printf("Error in accept(). Error code: %d\n", WSAGetLastError());
            closesocket(serverSock);
            WSACleanup();
            return;
        }
        std::thread clientThread(ClientHandler, clientSock);
        clientThread.detach(); // Detach the thread to run independently
    }

    closesocket(serverSock);
    WSACleanup();
}

int main() {
    if (!InitWSA()) {
        return 1;
    }

    char choice;
    printf("Enter 's' for server or 'c' for client: ");
    std::cin >> choice;

    if (choice == 's') {
        Server();
    }
    else if (choice == 'c') {
        getchar(); // Clear the newline character left in the input buffer
        Client();
    }
    else {
        printf("Invalid choice.\n");
    }

    WSACleanup();
    return 0;
}