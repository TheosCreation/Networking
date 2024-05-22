/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : MessageProcessor.h
Description : Provides functionality to process messages from clients
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#pragma once
#include <thread>
#include "Server.h"
#include <algorithm>

/**
 * @class MessageProcessor
 * @brief Provides functionality to process messages from clients.
 */
class MessageProcessor {
public:
    /**
     * @brief Processes a message received from a client.
     * @param message The message received from the client.
     * @param clientSock The socket associated with the client.
     * @param server A pointer to the server instance.
     * @param serverReply A reference to a string where the server's reply will be stored.
     * @return The processed message.
     */
    static std::string ProcessMessage(const std::string& message, SOCKET clientSock, Server* server, std::string& serverReply);

private:
    /**
     * @brief Capitalizes all characters in the message.
     * @param message The message to be capitalized.
     */
    static void Capitalize(std::string& message);

    /**
     * @brief Reverses the characters in the message.
     * @param message The message to be reversed.
     */
    static void Reverse(std::string& message);
};