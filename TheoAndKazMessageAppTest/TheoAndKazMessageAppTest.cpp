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

void ReceiveMessages(SOCKET clientSock) {
    char buffer[256];
    while (true) {
        int rcv = recv(clientSock, buffer, sizeof(buffer), 0);
        if (rcv == SOCKET_ERROR) {
            printf("Error in recv(). Error code: %d\n", WSAGetLastError());
            continue;
        }
        if (rcv == 0) {
            printf("Server disconnected.\n");
            break;
        }
        buffer[rcv] = '\0';
        printf("%s\n", buffer);
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
    gets_s(name);
    send(clientSock, name, strlen(name), 0);

    std::thread recvThread(ReceiveMessages, clientSock);
    recvThread.detach(); // Detach the thread to run independently

    char message[256];
    while (true) {
        printf("Enter message: ");
        gets_s(message);
        if (strcmp(message, "exit") == 0)
            break;
        send(clientSock, message, strlen(message), 0);
    }

    closesocket(clientSock);
}

void ClientHandler(SOCKET clientSock) {
    char clientName[256];
    recv(clientSock, clientName, sizeof(clientName), 0);
    std::string name(clientName);
    clientNames.push_back(name);

    char buffer[256];
    while (true) {
        int rcv = recv(clientSock, buffer, sizeof(buffer), 0);
        if (rcv == SOCKET_ERROR) {
            printf("Error in recv(). Error code: %d\n", WSAGetLastError());
            continue;
        }
        if (rcv == 0) {
            printf("Client disconnected.\n");
            break;
        }
        buffer[rcv] = '\0';
        printf("[%s]: %s\n", clientName, buffer);
        // Send the message to all clients
        for (size_t i = 0; i < clients.size(); ++i) {
            if (clients[i] != clientSock) {
                send(clients[i], buffer, strlen(buffer), 0);
            }
        }
    }

    closesocket(clientSock);
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
        clients.push_back(clientSock);
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
        Client();
    }
    else {
        printf("Invalid choice.\n");
    }

    WSACleanup();
    return 0;
}