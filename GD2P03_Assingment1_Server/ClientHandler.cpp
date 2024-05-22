/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : ClientHandler.cpp
Description : Handles communication with a connected client
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#include "ClientHandler.h"
#include "MessageProcessor.h"

ClientHandler::ClientHandler(SOCKET clientSock, Server* server) : clientSock(clientSock), server(server) 
{
}

void ClientHandler::HandleClient() {
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

    server->AddClient(name, clientSock);

    std::string welcomeMessage = "Welcome, " + name + "!\n";
    send(clientSock, welcomeMessage.c_str(), (int)welcomeMessage.length(), 0);

    std::cout << "[" << name << "] connected from IP: " << clientIP << std::endl;
    server->BroadcastMessage("[" + name + "] connected.");

    std::string clientTextName = "[" + name + "] ";
    while (server->isRunning()) {
        // While the app is running waits for a message 
        std::cout << "Waiting for a message..." << std::endl;

        char buffer[4096];
        auto bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            server->RemoveClient(clientSock);
            break;
        }

        std::string receivedMessage(buffer, bytesReceived);
        std::string serverCommand = "";
        std::string response = MessageProcessor::ProcessMessage(receivedMessage, clientSock, server, serverCommand);
        if (!response.empty()) {
            if (!serverCommand.empty())
            {
                std::cout << serverCommand << name << std::endl;
                std::string serverReplyMessage = "[Server] " + response;
                send(clientSock, serverReplyMessage.c_str(), (int)serverReplyMessage.length(), 0);
            }
            else
            {
                server->BroadcastMessage(clientTextName + response);
                std::cout << "Message from " + name + ", " + clientIP + ": " + response << std::endl;
            }
        }
    }
}