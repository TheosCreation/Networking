#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <algorithm>s
#include <SFML/Graphics.hpp>

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

std::vector<SOCKET> clients;
std::vector<std::string> clientNames;
sf::Text serverText;
sf::Text clientText;
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
            serverText.setString("[" + GetNameBySocket(clientSock) + "] disconnected.\n" + serverText.getString());
            clientNames.erase(clientNames.begin() + i);
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

void BroadcastMessage(const std::string& message) {
    for (size_t i = 0; i < clients.size(); ++i) {
        send(clients[i], message.c_str(), message.length(), 0);
    }
}

std::string BuildChatHistory() {
    std::string history;
    sf::String serverTextString = serverText.getString();
    size_t startPos = 0;
    size_t endPos = serverTextString.find('\n');

    while (endPos != sf::String::InvalidPos) {
        std::string line = serverTextString.substring(startPos, endPos - startPos);

        // Check if the line contains client connection or disconnection messages
        if (line.empty() || line.find("[") != std::string::npos) {
            startPos = endPos + 1; // Move to the next line
            endPos = serverTextString.find('\n', startPos);
            continue; // Skip these lines
        }

        // Append the line to the chat history if it's not empty
        if (!line.empty()) {
            history += line + '\n';
        }

        startPos = endPos + 1; // Move to the next line
        endPos = serverTextString.find('\n', startPos);
    }

    return history;
}

void ReceiveMessages(SOCKET clientSock) {
    char buffer[4096];
    while (true) {
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0); // Reduce buffer size by 1 to leave space for the null terminator
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // Add null terminator to properly terminate the string
            std::string message(buffer);
            clientText.setString(clientText.getString() + message + "\n");
        }
        else if (bytesReceived == 0) {
            printf("Disconnected from server.\n");
            break;
        }
        else {
            printf("Error in recv(). Error code: %d\n", WSAGetLastError());
            break;
        }
    }
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

    // Send the welcome message to the client
    std::string welcomeMessage = "Welcome, " + name + "!\n";
    send(clientSock, welcomeMessage.c_str(), welcomeMessage.length(), 0);

    // Send the entire chat history to the client
    std::string history = serverText.getString();
    // Send the current chunk
    send(clientSock, history.c_str(), history.length(), 0);

    serverText.setString("[" + name + "] connected.\n" + serverText.getString());

    while (true) {
        char buffer[4096];
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            RemoveClient(clientSock);
            break;
        }

        std::string message = "[" + GetNameBySocket(clientSock) + "]: " + std::string(buffer, bytesReceived);
        serverText.setString(message + "\n" + serverText.getString());

        BroadcastMessage(message);
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
    sf::Font font;
    if (!font.loadFromFile("Fonts/OpenSans-Regular.ttf")) {
        std::cerr << "Failed to load font." << std::endl;
        return 1;
    }

    char choice;
    printf("Enter 's' for server or 'c' for client: ");
    std::cin >> choice;
    
    // we can split this program into two exe one for server and one for client
    if (choice == 's') {
        serverText.setCharacterSize(16);
        serverText.setFont(font);
        serverText.setFillColor(sf::Color::White);
        serverText.setPosition(10, 10);

        sf::RenderWindow serverWindow(sf::VideoMode(800, 600), "Server Chat");

        std::thread serverThread(Server);
        // Launch server window
        while (serverWindow.isOpen()) {
            sf::Event event;
            while (serverWindow.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    serverWindow.close();
                }
            }
            serverWindow.clear();

            serverWindow.draw(serverText);

            serverWindow.display();
        }
        serverThread.join();
    }
    else if (choice == 'c') {
        SOCKET clientSock;

        clientSock = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSock == INVALID_SOCKET) {
            printf("Error in socket(), Error code %d\n", WSAGetLastError());
            return -1;
        }

        sockaddr_in recvAddr;
        recvAddr.sin_family = AF_INET;
        recvAddr.sin_port = htons(15366); // Change to server port
        InetPton(AF_INET, L"127.0.0.1", &recvAddr.sin_addr.S_un.S_addr);

        int status = connect(clientSock, (sockaddr*)&recvAddr, sizeof(recvAddr));
        if (status == SOCKET_ERROR) {
            printf("Error in connect(), Error code %d\n", WSAGetLastError());
            closesocket(clientSock);
            return -1;
        }
        std::thread receiveThread(ReceiveMessages, clientSock);

        clientText.setCharacterSize(16);
        clientText.setFont(font);
        clientText.setFillColor(sf::Color::White);
        clientText.setPosition(10, 30);
        // Launch client window
        sf::RenderWindow clientWindow(sf::VideoMode(800, 600), "Client Chat");

        // Text input field
        sf::Text inputText("", font, 16);
        inputText.setFillColor(sf::Color::White);
        inputText.setPosition(10, 550);
        std::string inputBuffer;

        while (clientWindow.isOpen()) {
            sf::Event event;
            while (clientWindow.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    clientWindow.close();
                }
                // Handle text input
                if (event.type == sf::Event::TextEntered) {
                    if (event.text.unicode == '\b') { // Backspace
                        if (!inputBuffer.empty()) {
                            inputBuffer.pop_back();
                            inputText.setString(inputBuffer);
                        }
                    }
                    else if (event.text.unicode < 128) { // Printable characters
                        inputBuffer += static_cast<char>(event.text.unicode);
                        inputText.setString(inputBuffer);
                    }
                }
                // Handle Enter key press
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Enter) {
                        // Send message to server
                        if (!inputBuffer.empty()) {
                            // Send the message to the server
                            send(clientSock, inputBuffer.c_str(), inputBuffer.length(), 0);
                            // Clear the input buffer and text field
                            inputBuffer.clear();
                            inputText.setString("");
                        }
                    }
                }
            }
            clientWindow.clear();
        
            // Draw text input field
            clientWindow.draw(inputText);

            clientWindow.draw(clientText);
        
            clientWindow.display();
        }


        closesocket(clientSock);
        receiveThread.join();
    }
    else {
        printf("Invalid choice.\n");
    }

    WSACleanup();
    return 0;
}