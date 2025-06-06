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

    if(server.getClientsBook().nickExists(_nick))
    {
            std::string error_msg = "ERROR: Nickname already in use.\n";
            send(client_fd, error_msg.c_str(), error_msg.length(), 0);
            return;
    }

    if (!server.getClientsBook().fdExists(client_fd) ) 
    {
        std::cout << "NO EXISTE FD" << std::endl;
        std::cerr << "ERROR: No client found with fd " << client_fd << std::endl;
        return;
    }
    Client* client = server.getClientsBook().getClient(client_fd);
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
    } 
    else if (command == "QUIT") {
        server.removeClient(client_fd);  // Desconectar al cliente
    } 
    else if (command == "JOIN") {
        handleJoinCommand(client_fd, params, server);  // Unirse a un canal
    }  
    else if (command == "PRIVMSG") {
        handlePrivMsgCommand(client_fd, params, server);
    }
    else if (command == "PART") 
    {
        std::string channel = params.substr(0, params.find(' '));
        std::string reason = params.find(':') != std::string::npos ? params.substr(params.find(':') + 1) : "";
        handlePartCommand(client_fd, channel, reason, server);
    }

  
}




void CommandHandler::sendToAllClients(const std::string& message, int sender_fd, IRCServer &server) 
{
   
    
    const std::map<int, Client*>& clients = server.getClientsBook().getmap();
    for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        Client* client = it->second;

        // 🔒 Verificamos que esté conectado y que no sea el emisor
        if (client && client->getFd() != sender_fd && client->getStage() == CONNECTED) {
            send(client->getFd(), message.c_str(), message.length(), 0);
        }
    }
}



void CommandHandler::handleJoinCommand(int client_fd, const std::string& channel_name_raw, IRCServer& server) {
    std::string channel_name = channel_name_raw;
    
    // Limpiar el nombre del canal
    channel_name.erase(channel_name.find_last_not_of("\r\n") + 1);
    channel_name.erase(0, channel_name.find_first_not_of(" "));
    channel_name.erase(channel_name.find_last_not_of(" ") + 1);

    // Validar nombre del canal
    if (channel_name.empty() || channel_name[0] != '#') {
        std::string err_msg = ":server 461 JOIN :Invalid channel name.\n Remember that the name mustn´t be empty and must start with '#'.\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client) {
        std::cerr << "ERROR: Client not found for fd " << client_fd << std::endl;
        return;
    }

    // Crear canal si no existe
    if (!server.getChannelsBook().addChannel(channel_name)) {
        std::string err_msg = ":server 403 " + client->getNickname() + " " + channel_name + " :Channel creation failed\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    Channel* channel = server.getChannelsBook().getChannel(channel_name);

    // Verificar si ya está en el canal
    if (channel->hasClient(client)) {
        std::string msg = ":server 443 " + client->getNickname() + " " + channel_name + " :You're already in that channel\r\n";
        send(client_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    // Primer usuario = operador
    if (channel->getClients().empty()) {
        channel->addOperator(client);
    }

    // Unirse al canal
    channel->addClient(client);
    client->joinChannel(channel_name);

    // Construir mensaje JOIN (username placeholder: ~nickname)
    std::string join_msg = ":" + client->getNickname() + "!~" + client->getNickname() + "@localhost JOIN :" + channel_name + "\r\n";
    channel->sendToAll(join_msg, NULL);

    // Enviar tema (332) o aviso de no tema (331)
    if (!channel->getTopic().empty()) {
        std::string topic_msg = ":server 332 " + client->getNickname() + " " + channel_name + " :" + channel->getTopic() + "\r\n";
        send(client_fd, topic_msg.c_str(), topic_msg.size(), 0);
    } else {
        std::string notopic_msg = ":server 331 " + client->getNickname() + " " + channel_name + " :No topic is set\r\n";
        send(client_fd, notopic_msg.c_str(), notopic_msg.size(), 0);
    }

    // Construir lista de usuarios (operadores primero)
    std::string operators;
    std::string regular_users;
    const std::set<Client*>& members = channel->getClients();
    for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
        Client* member = *it;
        if (channel->isOperator(member)) {
            operators += "@" + member->getNickname() + " ";
        } else {
            regular_users += member->getNickname() + " ";
        }
    }

    std::string users = operators + regular_users;
    if (!users.empty()) users.erase(users.size() - 1); // Eliminar último espacio

    // Enviar lista de nombres (353) y fin (366)
    std::string names_msg = ":server 353 " + client->getNickname() + " = " + channel_name + " :" + users + "\r\n";
    std::string end_msg = ":server 366 " + client->getNickname() + " " + channel_name + " :End of /NAMES list\r\n";
    send(client_fd, names_msg.c_str(), names_msg.size(), 0);
    send(client_fd, end_msg.c_str(), end_msg.size(), 0);
}

// void CommandHandler::handleJoinCommand(int client_fd, const std::string& channel_name_raw, IRCServer& server)
// {
//     std::string channel_name = channel_name_raw;
//     channel_name.erase(channel_name.find_last_not_of("\r\n") + 1);
//     channel_name.erase(0, channel_name.find_first_not_of(" "));
//     channel_name.erase(channel_name.find_last_not_of(" ") + 1);

//     if (channel_name.empty() || channel_name[0] != '#') {
//         std::string err_msg = "ERROR: Invalid channel name. Must start with '#'.\n";
//         send(client_fd, err_msg.c_str(), err_msg.length(), 0);
//         return;
//     }

//     Client* client = server.getClientsBook().getClient(client_fd);
//     if (!client) {
//         std::cerr << "ERROR: Client not found for fd " << client_fd << std::endl;
//         return;
//     }
//     //AÑADIMOS EL CANAL EN EL CASO DE QUE TODAVIA NO ESTE EN BOOK
//     if(server.getChannelsBook().addChannel(channel_name) == false)
//     {
//         return ;
//     }

//     Channel* channel = server.getChannelsBook().getChannel(channel_name);


//     if (channel->getClients().empty()) 
//     {
//         channel->addOperator(client);  // Primer usuario es operador
//         channel->addClient(client);
//         client->joinChannel(channel_name);
//     }

//     else if (!channel->hasClient(client)) 
//     {
//         channel->addClient(client);
//         client->joinChannel(channel_name);

//         std::string join_msg = ":" + client->getNickname() + " JOIN " + channel_name + "\r\n";
//         channel->sendToAll(join_msg, client);

//         if (!channel->getTopic().empty()) {
//             std::string topic_msg = "332 " + client->getNickname() + " " + channel_name + " :" + channel->getTopic() + "\r\n";
//             send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);
//         }

//         // Enviar lista de usuarios en el canal
//         std::string users;
//         const std::set<Client*>& members = channel->getClients();
//         for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
//             users += "\n" + (*it)->getNickname();
//         }
//         std::string names_reply = "353 " + client->getNickname() + " = " + channel_name + " :" + users + "\r\n";
//         std::string end_reply = "366 " + client->getNickname() + " " + channel_name + " :End of /NAMES list.\r\n";

//         send(client_fd, names_reply.c_str(), names_reply.length(), 0);
//         send(client_fd, end_reply.c_str(), end_reply.length(), 0);
//     } 
//     else 
//     {
//         std::string msg = "You're already in that channel.\n";
//         send(client_fd, msg.c_str(), msg.length(), 0);
//     }
// }


void CommandHandler::handlePrivMsgCommand(int client_fd, const std::string& params, IRCServer& server) {
    Client* sender = server.getClientsBook().getClient(client_fd);
    if (!sender || sender->getStage() != CONNECTED) {
        std::string err_msg = ":server 451 * :You have not registered\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    // Parsear target y mensaje
    size_t space_pos = params.find(' ');
    if (space_pos == std::string::npos) {
        std::string err_msg = ":server 411 " + sender->getNickname() + " :No recipient given (PRIVMSG)\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    std::string target = params.substr(0, space_pos);
    std::string message = params.substr(space_pos + 1);

    // Validar mensaje
    if (message.empty() /*|| message[0] != ':'*/) {
        std::string err_msg = ":server 412 " + sender->getNickname() + " :No text to send\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }
    //message = message.substr(1); // Quitar el ':'

    // Caso 1: Mensaje a un canal
    if (target[0] == '#') {
        Channel* channel = server.getChannelsBook().getChannel(target);
        if (!channel) {
            std::string err_msg = ":server 403 " + sender->getNickname() + " " + target + " :No such channel\r\n";
            send(client_fd, err_msg.c_str(), err_msg.size(), 0);
            return;
        }
        if (!channel->hasClient(sender)) {
            std::string err_msg = ":server 404 " + sender->getNickname() + " " + target + " :Cannot send to channel\r\n";
            send(client_fd, err_msg.c_str(), err_msg.size(), 0);
            return;
        }
        // Enviar a todos en el canal
        std::string privmsg = ":" + sender->getNickname() + "!~" + sender->getRealname() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
        channel->sendToAll(privmsg, sender);
    } 
    // Caso 2: Mensaje a un usuario
    else {
        Client* receiver = server.getClientsBook().getClientByNick(target);
        if (!receiver) {
            std::string err_msg = ":server 401 " + sender->getNickname() + " " + target + " :No such nick/channel\r\n";
            send(client_fd, err_msg.c_str(), err_msg.size(), 0);
            return;
        }
        // Enviar al usuario
        std::string privmsg = ":" + sender->getNickname() + "!~" + sender->getRealname() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
        send(receiver->getFd(), privmsg.c_str(), privmsg.size(), 0);
    }
}



void CommandHandler::handlePartCommand(int client_fd, const std::string& channel_name, const std::string& reason, IRCServer& server) {
    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client || client->getStage() != CONNECTED) {
        std::string err_msg = ":server 451 * :You have not registered\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    // Validar formato del canal
    if (channel_name.empty() || channel_name[0] != '#') {
        std::string err_msg = ":server 403 " + client->getNickname() + " " + channel_name + " :No such channel\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    Channel* channel = server.getChannelsBook().getChannel(channel_name);
    if (!channel) {
        std::string err_msg = ":server 403 " + client->getNickname() + " " + channel_name + " :No such channel\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    // Verificar si el cliente está en el canal
    if (!channel->hasClient(client)) {
        std::string err_msg = ":server 442 " + client->getNickname() + " " + channel_name + " :You're not on that channel\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    // Notificar a los miembros del canal
    std::string part_msg = ":" + client->getNickname() + "!~" + client->getRealname() + "@localhost PART " + channel_name;
    if (!reason.empty()) part_msg += " :" + reason;
    part_msg += "\r\n";
    
    channel->sendToAll(part_msg, NULL); // Enviar a todos, incluido el remitente

    // Eliminar al cliente del canal
    channel->removeClient(client);
    client->leaveChannel(channel_name);

    // Si el canal queda vacío, eliminarlo
    if (channel->getClients().empty()) {
        server.getChannelsBook().removeChannel(channel_name);
    }
}