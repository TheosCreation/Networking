/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : Server.h
Description : Manages client connections, broadcasts messages, and handles the server's lifecycle
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <winsock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

/**
 * @class Server
 * @brief Manages client connections, broadcasts messages, and handles the server's lifecycle.
 */
class Server {
public:
    /**
     * @brief Constructs a Server object with the specified port.
     * @param port The port number on which the server will listen for incoming connections.
     */
    Server(int port);

    /**
     * @brief Destructor for the Server class. Cleans up resources.
     */
    ~Server();

    /**
     * @brief Initializes the Windows Sockets API.
     * @return True if initialization is successful, false otherwise.
     */
    bool InitWSA();

    /**
     * @brief Starts the server, allowing it to accept client connections.
     */
    void Start();

    /**
     * @brief Stops the server and disconnects all clients.
     */
    void Stop();

    /**
     * @brief Broadcasts a message to all connected clients.
     * @param message The message to be broadcasted.
     */
    void BroadcastMessage(std::string message);

    /**
     * @brief Adds a client to the server.
     * @param clientName The name of the client.
     * @param clientSock The socket associated with the client.
     */
    void AddClient(std::string clientName, SOCKET clientSock);

    /**
     * @brief Retrieves the name of a client based on their socket.
     * @param clientSock The socket associated with the client.
     * @return The name of the client.
     */
    std::string GetNameFromSocket(SOCKET clientSock);

    /**
     * @brief Removes a client from the server.
     * @param clientSock The socket associated with the client to be removed.
     */
    void RemoveClient(SOCKET clientSock);

    /**
     * @brief Checks if the server is currently running.
     * @return True if the server is running, false otherwise.
     */
    bool isRunning();

    /// Stores the last message received from each client.
    std::unordered_map<SOCKET, std::string> clientLastMessage;

private:
    /**
     * @brief Accepts incoming client connections.
     */
    void AcceptClients();

    SOCKET serverSock; // The server socket used to listen for incoming connections.
    std::vector<SOCKET> clients; // A list of client sockets connected to the server.
    std::vector<std::string> clientNames; // A list of client names corresponding to the connected clients.
    bool running; // Indicates whether the server is currently running.
    int port; // The port number on which the server listens for incoming connections.
};