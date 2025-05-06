#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include "CommandHandler.hpp"
#include <cstring>
#include "CommandHandler.hpp"
#include <cstdlib>
#include <map>

CommandHandler::CommandHandler()
{

}
CommandHandler::~CommandHandler()
{

}

void CommandHandler::handleNickCommand(int client_fd, const std::string& new_nick, IRCServer &server ) 
{
    std::string _nick(new_nick);

   std::cout << "handle nick command" << std::endl;
   _nick.erase(_nick.find_last_not_of("\r\n") + 1);  
   _nick.erase(0, _nick.find_first_not_of(" ")); 
   _nick.erase(_nick.find_last_not_of(" ") + 1); 

    if (_nick.empty()) 
    {
        std::string error_msg = "ERROR: Nickname cannot be empty.\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
        return;
    }

    if(server.getBook().nickExists(_nick))
    {
            std::string error_msg = "ERROR: Nickname already in use.\n";
            send(client_fd, error_msg.c_str(), error_msg.length(), 0);
            return;
    }

    if (!server.getBook().fdExists(client_fd) ) 
    {
        std::cout << "NO EXISTE FD" << std::endl;
        std::cerr << "ERROR: No client found with fd " << client_fd << std::endl;
        return;
    }
    Client* client = server.getBook().getClient(client_fd);
    if (!client) 
    {
        std::cerr << "ERROR: Client pointer is NULL for fd " << client_fd << std::endl;
        return;
    }
    client->setNickname(_nick);
    // Establecer el nuevo nickname
    std::string success_msg = "Nickname successfully set to: " + _nick + "\n";
    send(client_fd, success_msg.c_str(), success_msg.length(), 0);
    std::cout << "Client with fd " << client_fd << " has changed their nickname to: " << _nick << "." << std::endl;
}

// Manejar los mensajes enviados por los clientes
void CommandHandler::handleClientMessage(int client_fd, const std::string& message, IRCServer &server ) 
{
    std::istringstream iss(message);
    std::string command, params;

    iss >> command;
    std::getline(iss, params);

    if (!params.empty() && params[0] == ' ') {
        params = params.substr(1);  // Eliminar espacios extra al inicio de los parámetros
    }

    // Manejo de comandos específicos
    if (command == "NICK") {
        handleNickCommand(client_fd, params, server);  // Cambiar el nickname
    } else if (command == "QUIT") {
        server.removeClient(client_fd);  // Desconectar al cliente
    } 
    // else {
    //     std::string error_msg = "ERROR: Unknown command\n";
    //     send(client_fd, error_msg.c_str(), error_msg.length(), 0);  // Comando desconocido
    //     std::cout << "Unknown command received from client " << client_fd << ": " << command << std::endl;
    // }
}

void CommandHandler::sendToAllClients(const std::string& message, int sender_fd, IRCServer &server) 
{
    // std::map<int, Client *> map = server.getBook().getmap();
    // std::map<int, Client *>::iterator it = map.begin();
    // while(it != map.end())
    // {
    //     if (it->first != sender_fd) 
    //         send(it->first, message.c_str(), message.size(), 0);
    //     it ++;
    // }    
    
    const std::map<int, Client*>& clients = server.getBook().getmap();
    for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        Client* client = it->second;

        // 🔒 Verificamos que esté conectado y que no sea el emisor
        if (client && client->getFd() != sender_fd && client->getStage() == CONNECTED) {
            send(client->getFd(), message.c_str(), message.length(), 0);
        }
    }
}