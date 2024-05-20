#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <algorithm>

#include <winsock2.h>
#include <WS2tcpip.h>
#include <unordered_map>
#pragma comment(lib, "Ws2_32.lib")

std::vector<SOCKET> clients;
std::vector<std::string> clientNames;
std::vector<std::string> messages; 
std::unordered_map<SOCKET, std::string> clientLastMessage;
bool running = true;

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

void BroadcastMessage(const std::string& message) {
    for (size_t i = 0; i < clients.size(); ++i) {
        send(clients[i], message.c_str(), message.length(), 0);
    }
}

void BroadcastMessageButOne(const std::string& message, SOCKET IgnoredClient) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == IgnoredClient) return;

        send(clients[i], message.c_str(), message.length(), 0);
    }
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
            std::cout << "[" << GetNameBySocket(clientSock) << "] disconnected." << std::endl;

            BroadcastMessage("[" + GetNameBySocket(clientSock) + "] disconnected."); 
            clientNames.erase(clientNames.begin() + i);
            clients.erase(clients.begin() + i);

            if (clients.size() == 0)
            {
                ForceClose();
            }
            break;
        }
    }
}

void ClientHandler(SOCKET clientSock) {
    // Get client's IP address
    sockaddr_in clientAddr;
    int addrSize = sizeof(clientAddr);
    getpeername(clientSock, (sockaddr*)&clientAddr, &addrSize);
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

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

    // Send the welcome message to the client
    std::string welcomeMessage = "Welcome, " + name + "!\n";
    send(clientSock, welcomeMessage.c_str(), welcomeMessage.length(), 0);

    // Send the entire chat message history to the client
    for (std::string message : messages) {
        send(clientSock, message.c_str(), message.length(), 0);
    }

    std::cout << "[" << name << "] connected from " << clientIP << std::endl;

    //Send Connection notices to all clients but the one that has just join
    BroadcastMessageButOne("[" + name + "] connected", clientSock);

    while (running) {
        std::cout << "Waiting for a message..." << std::endl;
        char buffer[4096];
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            RemoveClient(clientSock);
            break;
        }

        std::string receivedMessage(buffer, bytesReceived);
        std::string message;

        if (receivedMessage.substr(0, 5) == "/PUT ") {
            // Store the message
            std::string toStore = receivedMessage.substr(5); // Message after /PUT
            clientLastMessage[clientSock] = toStore;
            message = "[Server] message saved";
            send(clientSock, message.c_str(), message.size(), 0);
            std::cout << "Message saved from client " << GetNameBySocket(clientSock) << std::endl;
        }
        else if (receivedMessage == "/GET") {
            // Retrieve the last stored message
            if (clientLastMessage.find(clientSock) != clientLastMessage.end()) {
                message = "[Server] " + clientLastMessage[clientSock];
            }
            else {
                message = "[Server] No message found";
            }
            send(clientSock, message.c_str(), message.size(), 0);
        }
        else if (receivedMessage == "/QUIT") {
            // Handle client disconnection
            RemoveClient(clientSock);
            break;
        }
        else if (receivedMessage[0] == '/') {
            // Handle invalid command
            message = "[Server] INVALID COMMAND";
            send(clientSock, message.c_str(), message.size(), 0);
        }
        else {
            message = "[" + GetNameBySocket(clientSock) + "] " + receivedMessage;
            std::cout << message << std::endl;
            BroadcastMessage(message);
        }
    }
}

void Server() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(15366); // Change to desired port
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        printf("Error in socket(). Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    int bound = bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (bound == SOCKET_ERROR) {
        printf("Error in bind(). Error code: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    std::cout << "Listening..." << std::endl;
    int status = listen(serverSock, 5);
    if (status == SOCKET_ERROR) {
        printf("Error in listen(). Error code: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);
    while (running) {
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

    std::thread serverThread(Server);

    serverThread.join();
    WSACleanup();
    return 0;
}