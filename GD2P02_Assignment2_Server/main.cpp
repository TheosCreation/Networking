/***
Bachelor of Software Engineering
Media Design School
Auckland
New Zealand
(c) 2024 Media Design School
File Name : main.cpp
Description : The entry point of the application
Author : Theo Morris
Mail : theo.morris@mds.ac.nz
**/

#include "Server.h"
#include <thread>

int main() {
    Server server(15366);
    if (!server.InitWSA()) {
        return 1;
    }

    server.Start();

    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.Stop();
    return 0;
}
