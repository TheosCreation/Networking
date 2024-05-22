/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : Server.cpp
Description : Manages client connections, broadcasts messages, and handles the server's lifecycle
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#include "Server.h"
#include "ClientHandler.h"

Server::Server(int port) : port(port), running(false), serverSock(INVALID_SOCKET) {}

Server::~Server() {
    Stop();
}

bool Server::InitWSA() {
    WORD wVersionRequested = MAKEWORD(2, 2);
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

void Server::Start() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
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

    running = true;
    std::thread acceptThread(&Server::AcceptClients, this);
    acceptThread.detach();
}

void Server::Stop() {
    running = false;
    if (serverSock != INVALID_SOCKET) {
        closesocket(serverSock);
    }
    WSACleanup();
}

void Server::BroadcastMessage(std::string message)
{
    for (SOCKET client : clients) {
        send(client, message.c_str(), (int)message.length(), 0);
    }
}

void Server::AddClient(std::string clientName, SOCKET clientSock)
{
    clientNames.push_back(clientName);
    clients.push_back(clientSock);
}

std::string Server::GetNameFromSocket(SOCKET clientSock)
{
    auto it = std::find(clients.begin(), clients.end(), clientSock);
    if (it != clients.end()) {
        int index = (int)std::distance(clients.begin(), it);
        return clientNames[index];
    }
    return "";
}

bool Server::isRunning()
{
    return running;
}

void Server::AcceptClients() {
    sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);
    while (running) {
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrlen);
        if (clientSock == INVALID_SOCKET) {
            if (!running) break;
            std::cerr << "Error in accept(). Error code: " << WSAGetLastError() << std::endl;
            continue;
        }
        ClientHandler* handler = new ClientHandler(clientSock, this);
        std::thread(&ClientHandler::HandleClient, handler).detach();
    }
}

void Server::RemoveClient(SOCKET clientSock) {
    auto it = std::find(clients.begin(), clients.end(), clientSock);
    if (it != clients.end()) {
        int index = (int)std::distance(clients.begin(), it);
        // close the clients socket
        closesocket(clientSock);

        // Send disconection message to the server and all clients connected
        std::cout << "[" << clientNames[index] << "] disconnected." << std::endl;
        BroadcastMessage("[" + clientNames[index] + "] disconnected.");

        // remove all the clients data
        clients.erase(it);
        clientNames.erase(clientNames.begin() + index);
        clientLastMessage.erase(clientSock);

        // if there is no clients left stop the server
        if (clients.empty()) {
            Stop();
            std::cout << "No clients left. Server is shutting down." << std::endl;
        }
    }
}