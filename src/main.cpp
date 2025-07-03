#include "Server.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
     if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    // Verificar que el puerto sea numérico
    for (char* p = argv[1]; *p; p++) {
        if (!std::isdigit(*p)) {
            std::cerr << "Invalid port: must be numeric" << std::endl;
            return 1;
        }
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number. Port must be between 1 and 65535." << std::endl;
        return 1;
    }

    std::string password = argv[2];
    if (password.empty()) {
        std::cerr << "Password cannot be empty." << std::endl;
        return 1;
    }

    IRCServer server(port, password);
    server.run();
    return 0;
}
