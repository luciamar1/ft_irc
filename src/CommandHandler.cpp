#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include "CommandHandler.hpp"
#include <cstring>
#include <cctype> 
#include "CommandHandler.hpp"
#include <cstdlib>
#include <map>

CommandHandler::CommandHandler()
{

}
CommandHandler::~CommandHandler()
{

}

void CommandHandler::sendError(int client_fd, const std::string& code, const std::string& msg)
{
    std::string msg_error = "code error: " + code +" " + msg;
    send(client_fd, msg_error.c_str(), msg_error.length(), 0);
}

CommandHandler::ParsedMessage CommandHandler::parseMessage(const std::string& raw_message) {
    ParsedMessage result;
    
    // 1. Limitar tamaño máximo según RFC 1459 (512 bytes)
    if (raw_message.size() > 512) {
        result.command = "ERROR";
        result.trailing = "Message too long";
        return result;
    }
    
    // 2. Filtrar caracteres no ASCII y secuencias de escape
    std::string cleaned_message;
    for (size_t i = 0; i < raw_message.size(); ++i) {
        const char c = raw_message[i];
        // Permitir caracteres ASCII imprimibles (32-126) y \r, \n
        if ((c >= 32 && c <= 126) || c == '\r' || c == '\n' || c == '\t') {
            cleaned_message += c;
        }
        // Convertir secuencias de escape a espacios
        else if (c == 27) {  // ESC character (inicio de secuencia ANSI)
            while (i + 1 < raw_message.size() && 
                  (raw_message[i+1] == '[' || 
                   (raw_message[i+1] >= 'A' && raw_message[i+1] <= 'Z') || 
                   (raw_message[i+1] >= 'a' && raw_message[i+1] <= 'z'))) {
                i++;
            }
            cleaned_message += ' ';  // Reemplazar secuencia por espacio
        }
    }
    
    std::istringstream iss(cleaned_message);
    std::string token;
    
    // 3. Parsear comando principal
    if (!(iss >> result.command)) {
        result.command = "EMPTY";
        return result;
    }
    
    // 4. Parsear parámetros y trailing con límite máximo
    int param_count = 0;
    const int MAX_PARAMS = 10;
    
    while (iss >> token && param_count < MAX_PARAMS) {
        if (token[0] == ':') {
            // Capturar trailing completo
            result.trailing = token.substr(1);
            std::getline(iss, result.trailing, '\0');  // Leer hasta final
            
            // Eliminar \r y \n del final
            size_t end = result.trailing.find_last_not_of("\r\n");
            if (end != std::string::npos) {
                result.trailing = result.trailing.substr(0, end + 1);
            }
            break;
        }
        
        // Limitar tamaño de cada parámetro
        if (token.size() > 50) {
            token = token.substr(0, 50);
        }
        
        result.params.push_back(token);
        param_count++;
    }
    
    // 5. Limitar tamaño del trailing
    if (result.trailing.size() > 300) {
        result.trailing = result.trailing.substr(0, 300);
    }
    
    return result;
}


// CommandHandler::ParsedMessage CommandHandler::parseMessage(const std::string& raw_message) {
//     ParsedMessage result;
//     std::istringstream iss(raw_message);
//     std::string token;
    
//     iss >> result.command;
    
//     while (iss >> token) {
//         if (token[0] == ':') {
//             result.trailing = token.substr(1);
//             std::getline(iss, result.trailing);
//             // Limpiar retornos de carro
//             size_t end = result.trailing.find_last_not_of("\r\n");
//             if (end != std::string::npos) {
//                 result.trailing = result.trailing.substr(0, end + 1);
//             }
//             break;
//         }
//         result.params.push_back(token);
//     }
    
//     return result;
// }
// CommandHandler::ParsedMessage CommandHandler::parseMessage(const std::string& raw_message) 
// {
//     ParsedMessage result;
//     std::istringstream iss(raw_message);
//     std::string token;
    
//     iss >> result.command;  // Extraer comando principal
    
//     // Procesar parámetros y trailing
//     while (iss >> token) {
//         if (token[0] == ':') {  // Inicio del trailing
//             result.trailing = token.substr(1);
//             std::getline(iss, result.trailing);  // Capturar todo lo restante
//             break;
//         }
//         result.params.push_back(token);
//     }
    
//     // Limpiar trailing (eliminar \r\n)
//     if (!result.trailing.empty()) {
//         size_t end = result.trailing.find_last_not_of("\r\n");
//         if (end != std::string::npos) {
//             result.trailing = result.trailing.substr(0, end + 1);
//         }
//     }
    
//     return result;
// }

// Modificar handleClientMessage para usar el parser
std::string cleanInput(const std::string& input) 
{
    std::string cleaned;
    for (size_t i = 0; i < input.size(); ++i) {
        const char c = input[i];
        if (c >= 32 && c <= 126) { // Solo caracteres ASCII imprimibles
            cleaned += c;
        }
    }
    return cleaned;
}


void CommandHandler::handleClientMessage(int client_fd, const std::string& message, IRCServer& server) {
    std::string cleaned_msg = cleanInput(message);
    ParsedMessage parsed = parseMessage(cleaned_msg);

    if (parsed.command == "NICK") {
        handleNickCommand(client_fd, parsed.params.empty() ? "" : parsed.params[0], server);
    } 
    else if (parsed.command == "QUIT") {
        server.removeClient(client_fd);
    } 
    else if (parsed.command == "JOIN") {
        std::string channel = parsed.params.empty() ? "" : parsed.params[0];
        std::string password = parsed.params.size() > 1 ? parsed.params[1] : "";
        handleJoinCommand(client_fd, parsed.params.empty() ? "" : parsed.params[0], password, server);
    } 
    else if (parsed.command == "PRIVMSG") {
        if (parsed.params.empty() || parsed.trailing.empty()) {
            sendError(client_fd, "411", "PRIVMSG :Not enough parameters");
            return;
        }
        handlePrivMsgCommand(client_fd, parsed.params[0], parsed.trailing, server);
    } 
    else if (parsed.command == "PART") {
        handlePartCommand(client_fd, parsed.params.empty() ? "" : parsed.params[0], parsed.trailing, server);
    }
    else if (parsed.command == "MODE") {
        std::string target = parsed.params.empty() ? "" : parsed.params[0];
        std::string modes = parsed.params.size() > 1 ? parsed.params[1] : "";
        std::vector<std::string> args(parsed.params.begin() + 2, parsed.params.end());
        
        handleModeCommand(client_fd, target, modes, args, server);
    }
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





void CommandHandler::handleJoinCommand(int client_fd, const std::string& channel_name_raw,const std::string& password, IRCServer& server) 
{
    std::string channel_name = channel_name_raw;
    
    std::string err_msg = "CHANEL NAME == " + channel_name + "\n" ;
    send(client_fd, err_msg.c_str(), err_msg.size(), 0);

    // Limpiar el nombre del canal
    channel_name.erase(channel_name.find_last_not_of("\r\n") + 1);
    channel_name.erase(0, channel_name.find_first_not_of(" "));
    channel_name.erase(channel_name.find_last_not_of(" ") + 1);
    
    server.getChannelsBook().getChannel(channel_name)->getPassword().empty();
    sendError(client_fd, "llego", "??????????????");
    // 1. Verificar si el canal requiere contraseña
    if (!server.getChannelsBook().getChannel(channel_name)->getPassword().empty()) 
    {
        // 2. Comprobar contraseña proporcionada
        if (password != server.getChannelsBook().getChannel(channel_name)->getPassword()) 
        {
            std::string err_msg = ":server 475 " + server.getClientsBook().getClient(client_fd)->getNickname() + " " + channel_name + " :Cannot join channel (+k) - invalid password\r\n";
            send(client_fd, err_msg.c_str(), err_msg.size(), 0);
            return;
        }
    }
        
    // Validar nombre del canal
    // err_msg = "CHANEL NAME == " + channel_name + "\n";
    // send(client_fd, err_msg.c_str(), err_msg.size(), 0);
    if (channel_name.empty() || channel_name[0] != '#'/*|| channel_name.find(' ')*/ ) {
        std::string err_msg = channel_name + ":server 461 JOIN :Invalid channel name.\n Remember that the name mustn´t be empty and must start with '#'.\r\n";
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







void CommandHandler::handlePrivMsgCommand(int client_fd, const std::string& target, const std::string& message, IRCServer& server) 
{
    Client* sender = server.getClientsBook().getClient(client_fd);
    
    // 1. Validar estado del cliente
    if (!sender || sender->getStage() != CONNECTED) 
    {
        std::string err_msg = ":server 451 * :You have not registered\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    // 2. Validar parámetros
    if (target.empty()) {
        std::string err_msg = ":server 411 " + sender->getNickname() + " :No recipient given\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }
    
    if (message.empty()) {
        std::string err_msg = ":server 412 " + sender->getNickname() + " :No text to send\r\n";
        send(client_fd, err_msg.c_str(), err_msg.size(), 0);
        return;
    }

    // 3. Determinar tipo de destino (canal o usuario)
    if (target[0] == '#') {
        // Mensaje a canal
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

        // Construir y enviar mensaje
        std::string msg = ":" + sender->getNickname() + "!" + sender->getRealname() + 
                        " PRIVMSG " + target + " :" + message + "\r\n";
        channel->sendToAll(msg, sender);
    } 
    else {
        // Mensaje privado
        Client* receiver = server.getClientsBook().getClientByNick(target);
        
        if (!receiver) {
            std::string err_msg = ":server 401 " + sender->getNickname() + " " + target + " :No such nick/channel\r\n";
            send(client_fd, err_msg.c_str(), err_msg.size(), 0);
            return;
        }

        // Construir y enviar mensaje
        std::string msg = ":" + sender->getNickname() + "!" + sender->getRealname() + "@"  + 
                        " PRIVMSG " + target + " :" + message + "\r\n";
        send(receiver->getFd(), msg.c_str(), msg.size(), 0);
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

void CommandHandler::handleModeCommand(int client_fd, const std::string& target, const std::string& mode_str, const std::vector<std::string>& args,IRCServer& server) 
{
    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client || client->getStage() != CONNECTED) 
    {
        sendError(client_fd, "451", ":You have not registered");
        return;
    }

    // Si es un canal
    if (target[0] == '#') 
    {
        Channel* channel = server.getChannelsBook().getChannel(target);
        if (!channel) 
        {
            sendError(client_fd, "403", target + " :No such channel");
            return;
        }

        // Solo operadores pueden cambiar modos
        if (!channel->isOperator(client)) 
        {
            sendError(client_fd, "482", target + " :You're not channel operator");
            return;
        }

        // Consulta de modos (sin argumentos)
        if (mode_str.empty()) 
        {
            std::string mode_reply = ":server 324 " + client->getNickname() + " " + target + " " + channel->getModeString() + "\r\n";
            send(client_fd, mode_reply.c_str(), mode_reply.size(), 0);
            return;
        }

        // Procesar cambios de modo: +modo o -modo
        char sign = '+';
        size_t arg_index = 0;
        std::string changes;
        std::string mode_args;

        for (size_t i = 0; i < mode_str.size(); ++i) 
        {
            char c = mode_str[i];
            
            if (c == '+' || c == '-') 
            {
                sign = c;
                continue;
            }

            switch (c) 
            {
                case 'i': // Invite-only
                case 't': // Topic restriction
                    channel->setMode(c, sign == '+');
                    changes += sign;
                    changes += c;
                    break;
                    
                case 'k': // Password
                    if (sign == '+') 
                    {
                        if (arg_index >= args.size()) 
                        {
                            sendError(client_fd, "461", "MODE k :Not enough parameters");
                            return;
                        }
                        channel->setMode(c, true, args[arg_index]);
                        changes += sign;
                        changes += c;
                        mode_args += " " + args[arg_index];
                        arg_index++;
                    } 
                    else 
                    {
                        channel->setMode(c, false);
                        changes += sign;
                        changes += c;
                    }
                    break;

                case 'o': // Operator privilege
                    if (arg_index >= args.size()) 
                    {
                        sendError(client_fd, "461", "MODE o :Not enough parameters");
                        return;
                    }
                    
                    {
                        Client* target_client = server.getClientsBook().getClientByNick(args[arg_index]);
                        if (!target_client) 
                        {
                            sendError(client_fd, "401", args[arg_index] + " :No such nick");
                            return;
                        }

                        if (sign == '+') 
                        {
                            channel->addOperator(target_client);
                        } 
                        else 
                        {
                            channel->removeOperator(target_client);
                        }
                        changes += sign;
                        changes += c;
                        mode_args += " " + args[arg_index];
                        arg_index++;
                    }
                    break;

                case 'l': // User limit
                    if (sign == '+') 
                    {
                        if (arg_index >= args.size()) 
                        {
                            sendError(client_fd, "461", "MODE l :Not enough parameters");
                            return;
                        }
                        const std::string& limit_str = args[arg_index];
                        if (!std::isdigit(limit_str[0])) 
                        {
                            sendError(client_fd, "696", target + " l :Invalid limit format");
                            return;
                        }
                        channel->setMode(c, true, limit_str);
                        changes += sign;
                        changes += c;
                        mode_args += " " + limit_str;
                        arg_index++;
                    } 
                    else 
                    {
                        channel->setMode(c, false);
                        changes += sign;
                        changes += c;
                    }
                    break;

                    default:
                    sendError(client_fd, "472", std::string(1, c) + " :is unknown mode char");
                    break;
            }
        }

        // Notificar a todos en el canal
        if (!changes.empty()) 
        {
            std::string msg = ":" + client->getNickname() + "!" + client->getRealname() + "@localhost MODE " + target + " " + changes + mode_args + "\r\n";
            channel->sendToAll(msg, NULL);
        }
    }
        // Modos de usuario no implementados
        else 
        {
            sendError(client_fd, "502", ":User modes not supported");
        }
}