#include "CommandHandler.hpp"
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>  
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

std::string intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void CommandHandler::sendError(int client_fd, const std::string& code, const std::string& msg, IRCServer& server)
{
    std::string msg_error = "code error: " + code +" " + msg;
    safeSend(client_fd, msg_error, server); 
}

void CommandHandler::safeSend(int client_fd, const std::string& message, IRCServer& server) {
    Client* client = server.getClientsBook().getClient(client_fd);
    if (client) {
        client->getOutputBuffer() += message;
    }
}



void CommandHandler::handleModeCommand(int client_fd, const std::string& target, 
                                      const std::string& mode_str, 
                                      const std::vector<std::string>& args,
                                      IRCServer& server) {
    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client || client->getStage() != CONNECTED) {
        sendError(client_fd, "451", ":You have not registered", server);
        return;
    }

    // Manejo para canales
    if (!target.empty() && target[0] == '#') {
        Channel* channel = server.getChannelsBook().getChannel(target);
        if (!channel) {
            sendError(client_fd, "403", target + " :No such channel", server);
            return;
        }

        // Verificar privilegios de operador
        if (!channel->isOperator(client)) {
            sendError(client_fd, "482", target + " :You're not channel operator", server);
            return;
        }

        // Consulta de modos (sin cambios)
        if (mode_str.empty()) {
            std::string reply = ":server 324 " + client->getNickname() + " " + target + " " + channel->getModeString() + "\r\n";
            safeSend(client_fd, reply, server);
            return;
        }
        

        // Procesar secuencia de modos
        char current_sign = '+';
        size_t arg_index = 0;
        std::string changes;
        std::string mode_args;
        bool valid_mode = true;

        for (size_t i = 0; i < mode_str.size() && valid_mode; ++i) 
        {
            char mode_char = mode_str[i];
            
            // Manejar cambio de signo (+/-)
            if (mode_char == '+' || mode_char == '-') {
                current_sign = mode_char;
                continue;
            }

            // Procesar cada modo específico
            switch (mode_char) 
            {
                case 'i': // Modo invite-only
                case 't': // Modo topic restringido
                    channel->setMode(mode_char, current_sign == '+');
                    changes += current_sign;
                    changes += mode_char;
                    break;
                    
                case 'k': // Contraseña del canal
                    if (current_sign == '+') {
                        if (arg_index >= args.size()) {
                            sendError(client_fd, "461", "MODE k :Not enough parameters", server);
                            valid_mode = false;
                            break;
                        }
                        channel->setMode(mode_char, true, args[arg_index]);
                        mode_args += " " + args[arg_index++];
                    } else {
                        channel->setMode(mode_char, false);
                    }
                    changes += current_sign;
                    changes += mode_char;
                    break;

                case 'o': // Privilegios de operador
                {
                    if (arg_index >= args.size()) {
                        sendError(client_fd, "461", "MODE o :Not enough parameters", server);
                        valid_mode = false;
                        break;
                    }
                    
                    Client* target_client = server.getClientsBook().getClientByNick(args[arg_index]);
                    if (!target_client) {
                        sendError(client_fd, "401", args[arg_index] + " :No such nick", server);
                        valid_mode = false;
                        break;
                    }

                    if (current_sign == '+') {
                        channel->addOperator(target_client);
                    } else {
                        channel->removeOperator(target_client);
                    }
                    changes += current_sign;
                    changes += mode_char;
                    mode_args += " " + args[arg_index++];
                    break;
                }

                case 'l': // Límite de usuarios
                    if (current_sign == '+') {
                        if (arg_index >= args.size()) {
                            sendError(client_fd, "461", "MODE l :Not enough parameters", server);
                            valid_mode = false;
                            break;
                        }
                        const std::string& limit = args[arg_index];
                        if (!std::isdigit(limit[0])) {
                            sendError(client_fd, "696", target + " l :Invalid limit format", server);
                            valid_mode = false;
                            break;
                        }
                        channel->setMode(mode_char, true, limit);
                        mode_args += " " + limit;
                        arg_index++;
                    } else {
                        channel->setMode(mode_char, false);
                    }
                    changes += current_sign;
                    changes += mode_char;
                    break;

                default: // Modo desconocido
                    sendError(client_fd, "472", std::string(1, mode_char) + " :is unknown mode char", server);
                    valid_mode = false;
                    break;
            }
        }
 // ... (resto del código sin cambios) ...

        // Notificar cambios si fueron válidos
        if (valid_mode && !changes.empty()) {
            std::string msg = ":" + client->getNickname() + "!" + client->getRealname() + 
                            "@localhost MODE " + target + " " + changes + mode_args + "\r\n";
            channel->sendToAll(msg, NULL);
        }
    } else {
        sendError(client_fd, "502", ":User modes not supported", server);
    }
}

void CommandHandler::handleTopicCommand(int client_fd, const std::string& channel_name, 
                                       const std::string& new_topic, IRCServer& server) {
    // 1. Verificar cliente autenticado
    Client* client_ptr = server.getClientsBook().getClient(client_fd);
    if (!client_ptr || client_ptr->getStage() != CONNECTED) {
        sendError(client_fd, "451", ":You have not registered", server);
        return;
    }

    // 2. Validar parámetros mínimos
    if (channel_name.empty()) {
        sendError(client_fd, "461", "TOPIC :Not enough parameters", server);
        return;
    }

    // 3. Verificar que el canal existe
    Channel* channel = server.getChannelsBook().getChannel(channel_name);
    if (!channel) {
        sendError(client_fd, "403", channel_name + " :No such channel", server);
        return;
    }

    // 4. Verificar que el cliente está en el canal
    if (!channel->hasClient(client_ptr)) {
        sendError(client_fd, "442", channel_name + " :You're not on that channel", server);
        return;
    }

    // 5. Si no se especifica nuevo tema: mostrar tema actual
    if (new_topic.empty()) {
        if (channel->getTopic().empty()) {
            std::string reply = ":server 331 " + client_ptr->getNickname() + " " + channel_name + " :No topic is set\r\n";
            safeSend(client_fd, reply, server);
        } else {
            std::string reply = ":server 332 " + client_ptr->getNickname() + " " + channel_name + " :" + channel->getTopic() + "\r\n";
            safeSend(client_fd, reply, server);
        }
        return;
    }

    // 6. Verificar permisos para cambiar el tema
    if (channel->isTopicRestricted() && !channel->isOperator(client_ptr)) {
        sendError(client_fd, "482", channel_name + " :You must be channel operator to change the topic", server);
        return;
    }

    // 7. Actualizar el tema
    channel->setTopic(new_topic);

    // 8. Notificar a todos en el canal
    std::string msg = ":" + client_ptr->getNickname() + "!" + client_ptr->getRealname() + 
                     "@localhost TOPIC " + channel_name + " :" + new_topic + "\r\n";
    channel->sendToAll(msg, NULL);

    // 9. Confirmación al cliente que cambió el tema
    std::string rpl = ":server 333 " + client_ptr->getNickname() + " " + channel_name + 
                     " " + client_ptr->getNickname() + " " + intToString(time(NULL)) + "\r\n";
    safeSend(client_fd, rpl, server);
}

void CommandHandler::handleInviteCommand(int client_fd, const std::string& nick, 
                                        const std::string& channel_name, IRCServer& server) {
    // 1. Verificar cliente autenticado
    Client* inviter = server.getClientsBook().getClient(client_fd);
    if (!inviter || inviter->getStage() != CONNECTED) {
        sendError(client_fd, "451", ":You have not registered", server);
        return;
    }

    // 2. Validar parámetros
    if (nick.empty() || channel_name.empty()) {
        sendError(client_fd, "461", "INVITE :Not enough parameters", server);
        return;
    }

    // 3. Verificar que el canal existe
    Channel* channel = server.getChannelsBook().getChannel(channel_name);
    if (!channel) {
        sendError(client_fd, "403", channel_name + " :No such channel", server);
        return;
    }

    // 4. Verificar que el inviter está en el canal
    if (!channel->hasClient(inviter)) {
        sendError(client_fd, "442", channel_name + " :You're not on that channel", server);
        return;
    }

    // 5. Verificar permisos
    if (channel->isInviteOnly() && !channel->isOperator(inviter)) {
        sendError(client_fd, "482", channel_name + " :You must be channel operator", server);
        return;
    }

    // 6. Verificar que el usuario objetivo existe
    Client* target = server.getClientsBook().getClientByNick(nick);
    if (!target) {
        sendError(client_fd, "401", nick + " :No such nick", server);
        return;
    }

    // 7. Verificar que no está ya en el canal
    if (channel->hasClient(target)) {
        sendError(client_fd, "443", nick + " " + channel_name + " :is already on channel", server);
        return;
    }

    // 8. Invitar al cliente
    channel->inviteClient(target);

    // 9. Mensaje al invitado
    std::string msg = ":" + inviter->getNickname() + "!" + inviter->getRealname() + 
                     "@localhost INVITE " + nick + " :" + channel_name + "\r\n";
    safeSend(target->getFd(), msg, server);
    
    // 10. Mensaje de confirmación al inviter
    std::string rpl = ":server 341 " + inviter->getNickname() + " " + nick + " " + channel_name + "\r\n";
    safeSend(client_fd, rpl, server);
    
    // 11. Mensaje a operadores del canal
    if (channel->isInviteOnly()) {
        std::string op_msg = ":" + inviter->getNickname() + "!" + inviter->getRealname() + 
                            "@localhost INVITE " + nick + " " + channel_name + "\r\n";
        channel->sendToAll(op_msg, inviter);
    }
}

void CommandHandler::handleKickCommand(int client_fd, const std::string& channel_name, 
                                      const std::string& user, const std::string& reason, 
                                      IRCServer& server) {
    // 1. Verificar cliente autenticado
    Client* issuer = server.getClientsBook().getClient(client_fd);
    if (!issuer || issuer->getStage() != CONNECTED) {
        sendError(client_fd, "451", ":You have not registered", server);
        return;
    }

    // 2. Validar parámetros mínimos
    if (channel_name.empty() || user.empty()) {
        sendError(client_fd, "461", "KICK :Not enough parameters", server);
        return;
    }

    // 3. Verificar que el canal existe
    Channel* channel = server.getChannelsBook().getChannel(channel_name);
    if (!channel) {
        sendError(client_fd, "403", channel_name + " :No such channel", server);
        return;
    }

    // 4. Verificar que el solicitante es operador
    if (!channel->isOperator(issuer)) {
        sendError(client_fd, "482", channel_name + " :You're not channel operator", server);
        return;
    }

    // 5. Verificar que el usuario objetivo existe y está en el canal
    Client* target = server.getClientsBook().getClientByNick(user);
    if (!target) {
        sendError(client_fd, "401", user + " :No such nick", server);
        return;
    }
    if (!channel->hasClient(target)) {
        sendError(client_fd, "441", user + " " + channel_name + " :They aren't on that channel", server);
        return;
    }

    // 6. Construir mensaje de expulsión
    std::string kick_msg = ":" + issuer->getNickname() + "!" + issuer->getRealname() +
                          "@localhost KICK " + channel_name + " " + user;
    if (!reason.empty()) kick_msg += " :" + reason;
    kick_msg += "\r\n";

    // 7. Enviar notificación a todos en el canal
    channel->sendToAll(kick_msg, NULL);

    // 8. Eliminar al usuario del canal
    channel->removeClient(target);
    target->leaveChannel(channel_name);

    // 9. Si el usuario era operador, quitar privilegios
    if (channel->isOperator(target)) {
        channel->removeOperator(target);
    }

    // 11. Eliminar canal si queda vacío
    if (channel->getClients().empty()) {
        server.getChannelsBook().removeChannel(channel_name);
    }
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
            // CORRECCIÓN: Capturar trailing completo incluyendo espacios
            result.trailing = token.substr(1);  // Quitar el ':' inicial
            
            // Leer el resto de la línea incluyendo espacios
            std::string remainder;
            std::getline(iss, remainder);
            
            // Añadir el resto si hay más contenido
            if (!remainder.empty()) {
                // Eliminar espacio inicial si existe
                if (!remainder.empty() && remainder[0] == ' ') {
                    remainder = remainder.substr(1);
                }
                result.trailing += " " + remainder;
            }
            
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
    if (parsed.command == "PING") {
        std::string param = parsed.params.empty() ? "server" : parsed.params[0];
        if (parsed.params.empty() && !parsed.trailing.empty()) {
            param = parsed.trailing;
        }
        safeSend(client_fd, "PONG :" + param + "\r\n", server);
        return;
    }
    
    // Ignorar PONG (respuesta del cliente)
    if (parsed.command == "PONG") {
        return;
    }
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
            sendError(client_fd, "411", "PRIVMSG :Not enough parameters", server);
            return;
        }
        handlePrivMsgCommand(client_fd, parsed.params[0], parsed.trailing, server);
    } 
    else if (parsed.command == "PART") {
        handlePartCommand(client_fd, parsed.params.empty() ? "" : parsed.params[0], parsed.trailing, server);
    }
    else if (parsed.command == "MODE") {
        // Verificación robusta de parámetros
        if (parsed.params.empty()) {
            sendError(client_fd, "461", "MODE :Not enough parameters", server);
            return;
        }
        
        const std::string& target = parsed.params[0];
        std::string modes = "";
        std::vector<std::string> args;
        
        // Manejo eficiente de modos y argumentos
        if (parsed.params.size() > 1) {
            modes = parsed.params[1];
            
            // Copia directa de argumentos sin creación temporal
            if (parsed.params.size() > 2) {
                args.reserve(parsed.params.size() - 2);  // Optimización de memoria
                for (size_t i = 2; i < parsed.params.size(); ++i) {
                    args.push_back(parsed.params[i]);
                }
            }
        }
    
        handleModeCommand(client_fd, target, modes, args, server);
    }
    else if (parsed.command == "INVITE") {
        if (parsed.params.size() < 2) {
            sendError(client_fd, "461", "INVITE :Not enough parameters", server);
            return;
        }
        handleInviteCommand(client_fd, parsed.params[0], parsed.params[1], server);
    }
    else if (parsed.command == "KICK") {
        std::string channel = parsed.params.size() > 0 ? parsed.params[0] : "";
        std::string user = parsed.params.size() > 1 ? parsed.params[1] : "";
        std::string reason = parsed.trailing;
        handleKickCommand(client_fd, channel, user, reason, server);
    }
    else if (parsed.command == "TOPIC") {
        std::string channel = parsed.params.empty() ? "" : parsed.params[0];
        std::string new_topic = parsed.trailing;
        handleTopicCommand(client_fd, channel, new_topic, server);
    }

}

void CommandHandler::handlePartCommand(int client_fd, const std::string& channel_name, 
                                      const std::string& reason, IRCServer& server) {
    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client || client->getStage() != CONNECTED) {
        std::string err_msg = ":server 451 * :You have not registered\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // Validar formato del canal
    if (channel_name.empty() || channel_name[0] != '#') {
        std::string err_msg = ":server 403 " + client->getNickname() + " " + channel_name + " :No such channel\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    Channel* channel = server.getChannelsBook().getChannel(channel_name);
    if (!channel) {
        std::string err_msg = ":server 403 " + client->getNickname() + " " + channel_name + " :No such channel\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // Verificar si el cliente está en el canal
    if (!channel->hasClient(client)) {
        std::string err_msg = ":server 442 " + client->getNickname() + " " + channel_name + " :You're not on that channel\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // Notificar a los miembros del canal
    std::string part_msg = ":" + client->getNickname() + "!~" + client->getRealname() + 
                          "@localhost PART " + channel_name;
    if (!reason.empty()) part_msg += " :" + reason;
    part_msg += "\r\n";
    
    channel->sendToAll(part_msg, NULL);

    // Eliminar al cliente del canal
    channel->removeClient(client);
    client->leaveChannel(channel_name);

    // Si el canal queda vacío, eliminarlo
    if (channel->getClients().empty()) {
        server.getChannelsBook().removeChannel(channel_name);
    }
}


void CommandHandler::handlePrivMsgCommand(int client_fd, const std::string& target, 
                                         const std::string& message, IRCServer& server) {
    Client* sender = server.getClientsBook().getClient(client_fd);
    
    // 1. Validar estado del cliente
    if (!sender || sender->getStage() != CONNECTED) {
        std::string err_msg = ":server 451 * :You have not registered\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // 2. Validar parámetros
    if (target.empty()) {
        std::string err_msg = ":server 411 " + sender->getNickname() + " :No recipient given\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }
    
    if (message.empty()) {
        std::string err_msg = ":server 412 " + sender->getNickname() + " :No text to send\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // 3. Determinar tipo de destino
    if (target[0] == '#') {
        // Mensaje a canal
        Channel* channel = server.getChannelsBook().getChannel(target);
        
        if (!channel) {
            std::string err_msg = ":server 403 " + sender->getNickname() + " " + target + " :No such channel\r\n";
            safeSend(client_fd, err_msg, server);
            return;
        }
        
        if (!channel->hasClient(sender)) {
            std::string err_msg = ":server 404 " + sender->getNickname() + " " + target + " :Cannot send to channel\r\n";
            safeSend(client_fd, err_msg, server);
            return;
        }

        // Construir y enviar mensaje
        std::string msg = ":" + sender->getNickname() + "!" + sender->getRealname() + 
                         " PRIVMSG " + target + " :" + message + "\r\n";
        channel->sendToAll(msg, sender);
    } else {
        // Mensaje privado
        Client* receiver = server.getClientsBook().getClientByNick(target);
        
        if (!receiver || receiver->getStage() != CONNECTED) {
            std::string err_msg = ":server 401 " + sender->getNickname() + " " + target + " :No such nick/channel\r\n";
            safeSend(client_fd, err_msg, server);
            return;
        }

        // Construir y enviar mensaje
        std::string msg = ":" + sender->getNickname() + "!" + sender->getRealname() + "@" + 
                         " PRIVMSG " + target + " :" + message + "\r\n";
        safeSend(receiver->getFd(), msg, server);
    }
}


void CommandHandler::handleNickCommand(int client_fd, const std::string& new_nick, IRCServer &server) 
{
    std::string _nick(new_nick);

    // Limpiar el nuevo nick
    _nick.erase(_nick.find_last_not_of("\r\n") + 1);  
    _nick.erase(0, _nick.find_first_not_of(" ")); 
    _nick.erase(_nick.find_last_not_of(" ") + 1); 

    if (_nick.empty()) 
    {
        std::string error_msg = "ERROR: Nickname cannot be empty.\n";
        safeSend(client_fd, error_msg, server);
        return;
    }

    if(server.getClientsBook().nickExists(_nick))
    {
        std::string error_msg = "ERROR: Nickname already in use.\n";
        safeSend(client_fd, error_msg, server);
        return;
    }

    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client) 
    {
        std::cerr << "ERROR: Client pointer is NULL for fd " << client_fd << std::endl;
        return;
    }
    
    // Guardar el nick antiguo para notificación
    std::string old_nick = client->getNickname();
    
    // Actualizar el nickname
    client->setNickname(_nick);
    
    // Notificar al cliente que cambió su nick
    std::string success_msg = ":" + old_nick + " NICK :" + _nick + "\r\n";
    safeSend(client_fd, success_msg, server);

    // Notificar a todos los canales donde está presente
    const std::set<std::string>& joinedChannels = client->getJoinedChannels();
    for (std::set<std::string>::const_iterator it = joinedChannels.begin(); it != joinedChannels.end(); ++it) 
    {
        Channel* channel = server.getChannelsBook().getChannel(*it);
        if (channel) 
        {
            // Construir mensaje en formato IRC estándar
            std::string msg = ":" + old_nick + "!" + client->getRealname() + 
                             "@localhost NICK :" + _nick + "\r\n";
            channel->sendToAll(msg, client);
        }
    }
    
    std::cout << "Client " << old_nick << " changed nickname to: " << _nick << std::endl;
}

void CommandHandler::handleJoinCommand(int client_fd, const std::string& channel_name_raw, const std::string& password, IRCServer& server) {
    


     std::string channel_name = channel_name_raw;
    
    // Limpieza profunda: eliminar TODOS los \r y \n en todo el string
    channel_name.erase(std::remove(channel_name.begin(), channel_name.end(), '\r'), channel_name.end());
    channel_name.erase(std::remove(channel_name.begin(), channel_name.end(), '\n'), channel_name.end());
    
    // Eliminar espacios al inicio y final
    channel_name.erase(0, channel_name.find_first_not_of(" "));
    if (!channel_name.empty()) {
        channel_name.erase(channel_name.find_last_not_of(" ") + 1);
    }

    // Validación básica del nombre del canal
    if (channel_name.empty() || channel_name[0] != '#') {
        std::string err_msg = ":server 403 " + channel_name + " :Invalid channel name\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    Client* client = server.getClientsBook().getClient(client_fd);
    if (!client) {
        std::cerr << "ERROR: Client not found for fd " << client_fd << std::endl;
        return;
    }


    // Verificar si el cliente está autenticado
    if (client->getStage() != CONNECTED) {
        std::string err_msg = ":server 451 " + client->getNickname() + " :You have not registered\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // Obtener el canal (puede ser NULL si no existe)
    Channel* channel = server.getChannelsBook().getChannel(channel_name);

    // Verificar contraseña solo si el canal existe y tiene contraseña
    if (channel && !channel->getPassword().empty()) {
        if (password != channel->getPassword()) {
            std::string err_msg = ":server 475 " + client->getNickname() + " " + channel_name + " :Cannot join channel (+k) - invalid password\r\n";
            safeSend(client_fd, err_msg, server);
            return;
        }
    }

    // Crear canal si no existe
    if (!channel) {
        if (!server.getChannelsBook().addChannel(channel_name)) {
            std::string err_msg = ":server 403 " + client->getNickname() + " " + channel_name + " :Channel creation failed\r\n";
            safeSend(client_fd, err_msg, server);
            return;
        }
        channel = server.getChannelsBook().getChannel(channel_name);
        
        // Verificar que se creó correctamente
        if (!channel) {
            std::cerr << "CRITICAL ERROR: Failed to create channel " << channel_name << std::endl;
            return;
        }
    }

    // 1. Verificar si el canal es solo por invitación (modo +i)
    if (channel->isInviteOnly() && !channel->isInvited(client)) {
        std::string err_msg = ":server 473 " + client->getNickname() + " " + channel_name + " :Cannot join channel (+i)\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // 2. Verificar si el cliente ya está en el canal
    if (channel->hasClient(client)) {
        std::string msg = ":server 443 " + client->getNickname() + " " + channel_name + " :You're already in that channel\r\n";
        safeSend(client_fd, msg, server);
        return;
    }

    // 3. Verificar límite de usuarios (modo +l)
    if (channel->getUserLimit() > 0 && 
        channel->getClients().size() >= channel->getUserLimit()) {
        std::string err_msg = ":server 471 " + client->getNickname() + " " + channel_name + " :Cannot join channel (+l) - channel is full\r\n";
        safeSend(client_fd, err_msg, server);
        return;
    }

    // Primer usuario = operador
    if (channel->getClients().empty()) {
        channel->addOperator(client);
    }

    // Unirse al canal
    channel->addClient(client);
    client->joinChannel(channel_name);

    // Limpiar invitación después de unirse exitosamente
    if (channel->isInviteOnly()) {
        channel->removeInvited(client); // Necesitarás implementar este método
    }

    // Construir mensaje JOIN
    std::string join_msg = ":" + client->getNickname() + "!" + client->getRealname() + 
                          "@localhost JOIN :" + channel_name + "\r\n";
    channel->sendToAll(join_msg, NULL);

    // Enviar tema del canal o aviso de no tema
    if (!channel->getTopic().empty()) {
        std::string topic_msg = ":server 332 " + client->getNickname() + " " + channel_name + " :" + channel->getTopic() + "\r\n";
        safeSend(client_fd, topic_msg, server);
    } else {
        std::string notopic_msg = ":server 331 " + client->getNickname() + " " + channel_name + " :No topic is set\r\n";
        safeSend(client_fd, notopic_msg, server);
    }

    // Construir lista de usuarios (operadores primero)
    std::string users_list;
    const std::set<Client*>& members = channel->getClients();
    
    for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
        Client* member = *it;
        if (channel->isOperator(member)) {
            users_list += "@";
        }
        users_list += member->getNickname() + " ";
    }

    if (!users_list.empty()) {
        users_list.erase(users_list.size() - 1); // Eliminar último espacio
    }

    // Enviar lista de nombres
    std::string names_msg = ":server 353 " + client->getNickname() + " = " + channel_name + " :" + users_list + "\r\n";
    std::string end_msg = ":server 366 " + client->getNickname() + " " + channel_name + " :End of /NAMES list\r\n";
    safeSend(client_fd, names_msg, server);
    safeSend(client_fd, end_msg, server);
}