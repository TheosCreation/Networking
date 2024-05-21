#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <condition_variable>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

bool running = true;
std::string loopMessage = "Enter Message: ";

bool InitWSA() {
    WORD wVersionRequested = MAKEWORD(2, 2); // Winsock version 2.2
    WSADATA wsaData;

    int result = WSAStartup(wVersionRequested, &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        std::cerr << "Error: Winsock version 2.2 not available" << std::endl;
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

int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

int calculateLines(const std::string& input) {
    return (input.length() / getConsoleWidth()) + 1;
}

void ProcessInput(SOCKET clientSock) {
    std::string inputBuffer;

    while (true) {
        std::getline(std::cin, inputBuffer);

        {
            if (!running) break;
        }

        if (inputBuffer.empty()) continue;

        //calculated how many lines the user inputted
        int linesToDelete = calculateLines(inputBuffer + loopMessage);
        //delete the current line and the input buffer and message the user inputted
        deleteLines(linesToDelete);

        auto bytesSent = send(clientSock, inputBuffer.c_str(), inputBuffer.length(), 0);
        if (bytesSent == -1) {
            running = false;
            std::cerr << "Failed to send message to the server." << std::endl;
            break;
        }
    }
}

void ReceiveMessages(SOCKET clientSock) {
    char buffer[4096];
    while (true) {
        auto bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            //clear current line
            std::cout << "\r" << std::string(getConsoleWidth(), ' ') << "\r" << buffer << std::endl;
            std::cout << loopMessage;
        }
        else if (bytesReceived == 0) {
            running = false;
            std::cout << "Disconnected from server." << std::endl;
            break;
        }
        else {
            running = false;
            std::cerr << "Error in recv(). Error code: " << WSAGetLastError() << std::endl;
            break;
        }
    }
}

void Client() {
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "Error in socket(), Error code " << WSAGetLastError() << std::endl;
        return;
    }

    std::string connectionChoice;
    std::cout << "Enter Server IP, or '1' to connect to 127.0.0.1: ";
    std::getline(std::cin, connectionChoice);

    std::string Name;
    std::cout << "Enter Name: ";
    std::getline(std::cin, Name);

    sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(15366); // Server port
    std::wstring serverIP = L"127.0.0.1";
    if (connectionChoice != "1") {
        serverIP = std::wstring(connectionChoice.begin(), connectionChoice.end());
    }
    InetPton(AF_INET, serverIP.c_str(), &recvAddr.sin_addr.S_un.S_addr);

    int status = connect(clientSock, (sockaddr*)&recvAddr, sizeof(recvAddr));
    if (status == SOCKET_ERROR) {
        std::cerr << "Error in connect(), Error code " << WSAGetLastError() << std::endl;
        closesocket(clientSock);
        return;
    }

    auto bytesSent = send(clientSock, Name.c_str(), Name.length(), 0);
    if (bytesSent == -1) {
        std::cerr << "Failed to send message to the server." << std::endl;
        closesocket(clientSock);
        return;
    }

    std::thread receiveMessageThread(ReceiveMessages, clientSock);
    std::cout << loopMessage;
    ProcessInput(clientSock);

    receiveMessageThread.join();
    closesocket(clientSock);
}

int main() {
    if (!InitWSA()) {
        return 1;
    }

    Client();

    WSACleanup();
    return 0;
}