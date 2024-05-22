/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : MessageProcessor.cpp
Description : Provides functionality to process messages from clients
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#include "MessageProcessor.h"

std::string MessageProcessor::ProcessMessage(const std::string& message, SOCKET clientSock, Server* server, std::string& serverReply) {
    std::string response;
    if (message.substr(0, 5) == "/PUT ") {
        std::string toStore = message.substr(5);
        server->clientLastMessage[clientSock] = toStore;
        response = "message saved";
        serverReply = "Message saved from client: ";
    }
    else if (message == "/GET") {
        if (server->clientLastMessage.find(clientSock) != server->clientLastMessage.end()) {
            response = server->clientLastMessage[clientSock];
        }
        else {
            response = "No message found";
        }
        serverReply = "Saved Message sent to client: ";
    }
    else if (message.substr(0, 12) == "/CAPITALIZE ") {
        std::string toCapitalize = message.substr(12);
        Capitalize(toCapitalize);
        response = toCapitalize;
    }
    else if (message.substr(0, 9) == "/REVERSE ") {
        std::string toReverse = message.substr(9);
        Reverse(toReverse);
        response = toReverse; 
    }
    else if (message == "/QUIT") {
        server->RemoveClient(clientSock);
    }
    else if (message[0] == '/') {
        response = "INVALID COMMAND";
        serverReply = "Invalid command sent from client: ";
    }
    else {
        response = message;
    }
    return response;
}

void MessageProcessor::Capitalize(std::string& message) {
    std::transform(message.begin(), message.end(), message.begin(), ::toupper);
}

void MessageProcessor::Reverse(std::string& message) {
    std::reverse(message.begin(), message.end());
}