#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include "Server.hpp"

class IRCServer;
class CommandHandler {
public:
    CommandHandler();
    ~CommandHandler();

    void handleNickCommand(int client_fd, const std::string& new_nick, IRCServer& server);
    void handleClientMessage(int client_fd, const std::string& message, IRCServer& server);
    void sendToAllClients(const std::string& message, int sender_fd, IRCServer& server);
    //void handlePrivMsgCommand(int client_fd, const std::string& target, const std::string& message, IRCServer& server);
    void handlePrivMsgCommand(int client_fd, const std::string& params, IRCServer& server);
    void handlePartCommand(int client_fd, const std::string& channel_name, const std::string& reason, IRCServer& server);
    void handleJoinCommand(int client_fd, const std::string& channel_name_raw, IRCServer& server);
};

#endif
