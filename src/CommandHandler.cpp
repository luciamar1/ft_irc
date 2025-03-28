
#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include <vector>
#include <string>

void CommandHandler::handlePass(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.empty()) {
        const std::string errorMsg = "ERROR :Password required\r\n";
        send(client->getFd(), errorMsg.c_str(), errorMsg.size(), 0);
        server.removeClient(client->getFd()); // Ahora es accesible
        return;
    }

    if (args[0] != server.getPassword()) { // Acceso mediante getter
        const std::string errorMsg = "ERROR :Bad password\r\n";
        send(client->getFd(), errorMsg.c_str(), errorMsg.size(), 0);
        server.removeClient(client->getFd());
    }
}

// Implementaciones restantes de comandos...