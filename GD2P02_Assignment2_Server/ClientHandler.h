/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : ClientHandler.h
Description : Handles communication with a connected client
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#pragma once
#include "Server.h"

/**
 * @class ClientHandler
 * @brief Handles communication with a connected client.
 */
class ClientHandler {
public: 
    /**
     * @brief Constructs a ClientHandler object.
     * @param clientSock The socket associated with the client.
     * @param server A pointer to the server instance.
     */
    ClientHandler(SOCKET clientSock, Server* server);

    /**
     * @brief Handles communication with the client.
     */
    void HandleClient();

private:
    SOCKET clientSock; // The socket associated with the client.
    Server* server; // A pointer to the server instance.
};