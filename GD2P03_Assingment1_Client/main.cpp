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
    WORD wVersionRequested = MAKEWORD(2, 2); // Winsock version 2.2
    WSADATA wsaData;

    int result = WSAStartup(wVersionRequested, &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return false;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("Error: Winsock version 2.2 not available\n");
        WSACleanup();
        return false;
    }
    return true;
}

void deleteLines(int count) {
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            std::cout << "\x1b[1A" << "\x1b[2K";
        }
        std::cout << "\r";
    }
}

void ProcessInput(SOCKET clientSock) {
    std::string inputBuffer;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "Enter Message: ";

    std::getline(std::cin, inputBuffer);
    deleteLines(1);

    size_t bytesSent = send(clientSock, inputBuffer.c_str(), inputBuffer.length(), 0);
    if (bytesSent == -1) {
        std::cerr << "Failed to send message to the server." << std::endl;
        running = false;
    }
}

void ReceiveMessages(SOCKET clientSock) {
    char buffer[4096];
    while (true) {
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << buffer << std::endl;
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

void Client() {
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == INVALID_SOCKET) {
        printf("Error in socket(), Error code %d\n", WSAGetLastError());
        return;
    }

    std::string connectionChoice;
    std::cout << "Enther Server IP, or '1' to connect to 127.0.0.1: ";
    std::getline(std::cin, connectionChoice);
    deleteLines(1);
    
    std::string Name;
    std::cout << "Enter Name: ";
    std::getline(std::cin, Name);
    deleteLines(1);

    sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(15366); // Server port
    std::wstring serverIP = L"127.0.0.1";
    if (connectionChoice == "1")
    {

        InetPton(AF_INET, serverIP.c_str(), &recvAddr.sin_addr.S_un.S_addr);
    }
    else
    {
        serverIP = std::wstring(connectionChoice.begin(), connectionChoice.end());
        InetPton(AF_INET, serverIP.c_str(), &recvAddr.sin_addr.S_un.S_addr);
    }

    int status = connect(clientSock, (sockaddr*)&recvAddr, sizeof(recvAddr));
    if (status == SOCKET_ERROR) {
        printf("Error in connect(), Error code %d\n", WSAGetLastError());
        closesocket(clientSock);
        return;
    }

    size_t bytesSent = send(clientSock, Name.c_str(), Name.length(), 0);
    if (bytesSent == -1) {
        std::cerr << "Failed to send message to the server." << std::endl;
    }

    std::thread receiveMessageThread(ReceiveMessages, clientSock);
    while (running) {
        ProcessInput(clientSock);
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