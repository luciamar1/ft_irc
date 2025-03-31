
#include "CommandHandler.hpp"


CommandHandler::CommandHandler()
{

}
CommandHandler::~CommandHandler()
{

}

void CommandHandler::handleNickCommand(int client_fd, const std::string& new_nick, IRCServer &server ) 
{
   
    std::string trimmed_nick = new_nick;
    trimmed_nick.erase(trimmed_nick.find_last_not_of("\r\n") + 1);  // Eliminar saltos de línea

    if (trimmed_nick.empty()) {
        std::string error_msg = "ERROR: Nickname cannot be empty.\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
        return;
    }

    if(server.nickExist(trimmed_nick))
    {
            std::string error_msg = "ERROR: Nickname already in use.\n";
            send(client_fd, error_msg.c_str(), error_msg.length(), 0);
            return;
    }

    // Establecer el nuevo nickname
    server.setClientsInfo(client_fd, trimmed_nick);
    std::string success_msg = "Nickname successfully set to: " + trimmed_nick + "\n";
    send(client_fd, success_msg.c_str(), success_msg.length(), 0);
    std::cout << "Client with fd " << client_fd << " has changed their nickname to: " << trimmed_nick << "." << std::endl;
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
    for (std::map<int, std::string>::const_iterator it = server.getClientsInfo().begin(); it !=  server.getClientsInfo().end(); ++it) {
        if (it->first != sender_fd) {
            send(it->first, message.c_str(), message.size(), 0);
        }
    }
}
