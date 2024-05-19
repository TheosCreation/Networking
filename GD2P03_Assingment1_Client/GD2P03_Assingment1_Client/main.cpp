#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <algorithm>

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

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

void ReceiveMessages(SOCKET clientSock) {
    char buffer[4096];
    while (true) {
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0); // Reduce buffer size by 1 to leave space for the null terminator
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // Add null terminator to properly terminate the string
            std::string message(buffer);
            std::cout << message << std::endl;
        }
        else if (bytesReceived == 0) {
            std::cout << "Disconnected from server." << std::endl;
            break;
        }
        else {
            std::cout << "Error in recv(). Error code: " << WSAGetLastError() << std::endl;
            break;
        }
    }
}

void Client()
{
    SOCKET clientSock;

    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == INVALID_SOCKET) {
        printf("Error in socket(), Error code %d\n", WSAGetLastError());
        return;
    }

    std::string Name;
    std::cout << "Enter Name: ";
    std::getline(std::cin, Name);

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

    // Send the name of the client to the server
    size_t bytesSent = send(clientSock, Name.c_str(), Name.length(), 0);
    if (bytesSent == -1) {
        std::cerr << "Failed to send message to the server." << std::endl;
        // Handle the error appropriately
    }

    std::string inputBuffer;

    std::thread receiveMessageThread(ReceiveMessages, clientSock);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //we wait to receive the welcome message before continuing
    while (running) {
        std::cout << "Enter Message: ";
        std::getline(std::cin, inputBuffer); // Read input from the user

        std::cout << "\x1b[1A << \x1b[2K"; // Move cursor up one line and clear the line

        // Send the message to the server
        size_t bytesSent = send(clientSock, inputBuffer.c_str(), inputBuffer.length(), 0);
        if (bytesSent == -1) {
            std::cerr << "Failed to send message to the server." << std::endl;
            // Handle the error appropriately, possibly setting running to false
            running = false;
        }
    }

    closesocket(clientSock);
    receiveMessageThread.join();
}

int main() {
    if (!InitWSA()) {
        return 1;
    }

    Client();

    WSACleanup();
    return 0;
}