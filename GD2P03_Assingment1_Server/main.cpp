#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

std::vector<SOCKET> clients;
std::vector<std::string> clientNames;
std::unordered_map<SOCKET, std::string> clientLastMessage;
bool running = true; 
SOCKET serverSock;

bool InitWSA() {
    WORD wVersionRequested = MAKEWORD(2, 2); // Winsock version 2.2
    WSADATA wsaData;

    int result = WSAStartup(wVersionRequested, &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartUp failed: " << result << std::endl;
        return false;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        std::cerr << "Error: Winsock version 2.2 not available" << std::endl;
        WSACleanup();
        return false;
    }
    return true;
}

void BroadcastMessage(const std::string& message) {
    for (SOCKET client : clients) {
        send(client, message.c_str(), message.length(), 0);
    }
}

void RemoveClient(SOCKET clientSock) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == clientSock) {
            closesocket(clientSock);
            std::cout << "[" << clientNames[i] << "] disconnected." << std::endl;

            BroadcastMessage("[" + clientNames[i] + "] disconnected.");
            clients.erase(clients.begin() + i);
            clientNames.erase(clientNames.begin() + i);

            if (clients.size() <= 0)
            {
                // Close the server
                running = false;
                closesocket(serverSock);
                std::cout << "No clients left. Server is shutting down." << std::endl;
            }
            break;
        }
    }
}

void ClientHandler(SOCKET clientSock) {
    // Get the client's IP address
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(clientSock, (sockaddr*)&clientAddr, &addrLen);
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

    char clientName[256];
    auto bytesReceived = recv(clientSock, clientName, sizeof(clientName) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }
    clientName[bytesReceived] = '\0';
    std::string name(clientName);

    clientNames.push_back(name);
    clients.push_back(clientSock);

    std::string welcomeMessage = "Welcome, " + name + "!\n";
    send(clientSock, welcomeMessage.c_str(), welcomeMessage.length(), 0);

    // Add the IP address they connected from
    std::cout << "[" << name << "] connected from IP: " << clientIP << std::endl;
    BroadcastMessage("[" + name + "] connected.");

    while (running) {
        std::cout << "Waiting for a message..." << std::endl;
        char buffer[4096];
        auto bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            RemoveClient(clientSock);
            break;
        }

        std::string receivedMessage(buffer, bytesReceived); std::string message;

        if (receivedMessage.substr(0, 5) == "/PUT ") {
            // Store the message
            std::string toStore = receivedMessage.substr(5); // Message after /PUT
            clientLastMessage[clientSock] = toStore;
            message = "[Server] message saved";
            send(clientSock, message.c_str(), message.size(), 0);
            std::cout << "Message saved from client " << name << std::endl;
        }
        else if (receivedMessage == "/GET") {
            // Retrieve the last stored message
            if (clientLastMessage.find(clientSock) != clientLastMessage.end()) {
                message = "[Server] " + clientLastMessage[clientSock];
            }
            else {
                message = "[Server] No message found";
            }
            std::cout << "Saved Message sent to " << name << std::endl;
            send(clientSock, message.c_str(), message.size(), 0);
        }
        else if (receivedMessage.substr(0, 12) == "/CAPITALIZE ") {
            // Capitalize the provided message
            std::string toCapitalize = receivedMessage.substr(12); // Message after /CAPITALIZE
            std::transform(toCapitalize.begin(), toCapitalize.end(), toCapitalize.begin(), ::toupper);
            message = "[Server] " + toCapitalize;
            send(clientSock, message.c_str(), message.size(), 0);
            std::cout << "Message capitalized and sent to client " << name << std::endl;
        }
        else if (receivedMessage.substr(0, 9) == "/REVERSE ") {
            // Reverse the provided message
            std::string toReverse = receivedMessage.substr(9); // Message after /REVERSE
            std::reverse(toReverse.begin(), toReverse.end());
            message = "[Server] " + toReverse;
            send(clientSock, message.c_str(), message.size(), 0);
            std::cout << "Message reversed and sent to client " << name << std::endl;
        }
        else if (receivedMessage == "/QUIT") {
            // Handle client disconnection
            RemoveClient(clientSock);
            break;
        }
        else if (receivedMessage[0] == '/') {
            // Handle invalid command
            message = "[Server] INVALID COMMAND";
            std::cout << "Client sent invalid command: " << name << std::endl;
            send(clientSock, message.c_str(), message.size(), 0);
        }
        else {
            message = "[" + name + "] " + receivedMessage;
            std::cout << message << std::endl;
            BroadcastMessage(message);
        }
    }
}

void Server() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(15366);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Error in socket(). Error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    int bound = bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (bound == SOCKET_ERROR) {
        std::cerr << "Error in bind(). Error code: " << WSAGetLastError() << std::endl;
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    std::cout << "Listening..." << std::endl;
    int status = listen(serverSock, 5);
    if (status == SOCKET_ERROR) {
        std::cerr << "Error in listen(). Error code: " << WSAGetLastError() << std::endl;
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);
    while (running) {
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrlen);
        if (clientSock == INVALID_SOCKET) {
            if (!running) {
                break; // Server is shutting down
            }
            int error = WSAGetLastError();
            if (error == WSAEINTR) {
                std::cout << "Server shutdown initiated. Accept call interrupted." << std::endl;
                break;
            }
            std::cerr << "Error in accept(). Error code: " << error << std::endl;
            closesocket(serverSock);
            WSACleanup();
            return;
        }
        std::thread clientThread(ClientHandler, clientSock);
        clientThread.detach();
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
