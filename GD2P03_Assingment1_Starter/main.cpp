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

//general
bool running = true;
const size_t MAX_LINES = 20;
const size_t MAX_WIDTH = 20;

//client
sf::Text clientText;
std::vector<std::string> messagesOnClient;

//server
std::vector<SOCKET> clients;
std::vector<std::string> clientNames;
sf::Text serverText;
std::vector<std::string> messagesOnServer;

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

void WrapText(sf::Text& text, float maxWidth) {
    sf::String wrappedText;
    sf::String currentLine;
    sf::String word;
    for (const auto& character : text.getString()) {
        if (character == ' ' || character == '\n') {
            if (text.getFont()->getGlyph(word[0], text.getCharacterSize(), false).advance * word.getSize() + text.getFont()->getGlyph(currentLine[0], text.getCharacterSize(), false).advance * currentLine.getSize() > maxWidth) {
                wrappedText += currentLine + '\n';
                currentLine = word + ' ';
            }
            else {
                currentLine += word + ' ';
            }
            word.clear();
            if (character == '\n') {
                wrappedText += currentLine + '\n';
                currentLine.clear();
            }
        }
        else {
            word += character;
        }
    }
    if (!currentLine.isEmpty()) {
        wrappedText += currentLine;
    }
    text.setString(wrappedText);
}

void UpdateClientText() {
    sf::String updatedText;
    for (const auto& msg : messagesOnClient) {
        updatedText += msg + "\n";
    }
    clientText.setString(updatedText);

    // Wrap text
    WrapText(clientText, MAX_WIDTH);

    // If the number of lines is larger than max lines, trim the first line
    size_t lineCount = std::count(clientText.getString().begin(), clientText.getString().end(), '\n');
    while (lineCount > MAX_LINES) {
        size_t newlinePos = clientText.getString().find('\n');
        if (newlinePos != sf::String::InvalidPos) {
            clientText.setString(clientText.getString().substring(newlinePos + 1));
            lineCount--;
        }
        else {
            break; // Should not occur, but safety check
        }
    }
}

void UpdateServerText() {
    sf::String updatedText;
    for (const auto& msg : messagesOnServer) {
        updatedText += msg + "\n";
    }
    serverText.setString(updatedText);

    // Wrap text
    WrapText(serverText, MAX_WIDTH);

    // If the number of lines is larger than max lines, trim the first line
    size_t lineCount = std::count(serverText.getString().begin(), serverText.getString().end(), '\n');
    while (lineCount > MAX_LINES) {
        size_t newlinePos = serverText.getString().find('\n');
        if (newlinePos != sf::String::InvalidPos) {
            serverText.setString(serverText.getString().substring(newlinePos + 1));
            lineCount--;
        }
        else {
            break; // Should not occur, but safety check
        }
    }
}

void RemoveClient(SOCKET clientSock) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == clientSock) {
            closesocket(clientSock);
            messagesOnServer.push_back("[" + GetNameBySocket(clientSock) + "] disconnected.");
            clientNames.erase(clientNames.begin() + i);
            clients.erase(clients.begin() + i);
            break;
        }
    }
    UpdateServerText();
}

void BroadcastMessage(const std::string& message) {
    for (size_t i = 0; i < clients.size(); ++i) {
        send(clients[i], message.c_str(), message.length(), 0);
    }
}

void ReceiveMessages(SOCKET clientSock) {
    char buffer[4096];
    while (true) {
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0); // Reduce buffer size by 1 to leave space for the null terminator
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // Add null terminator to properly terminate the string
            std::string message(buffer);
            messagesOnClient.push_back(message);
            UpdateClientText();
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
    send(clientSock, history.c_str(), history.length(), 0);

    messagesOnServer.push_back("[" + name + "] connected.");
    UpdateServerText();

    while (true) {
        char buffer[4096];
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            RemoveClient(clientSock);
            break;
        }

        //when the server receives a message it will send it back to all clients connected
        std::string message = "[" + GetNameBySocket(clientSock) + "]: " + std::string(buffer, bytesReceived);
        messagesOnServer.push_back(message);
        UpdateServerText();

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