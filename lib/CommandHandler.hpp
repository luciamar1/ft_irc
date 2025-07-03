#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include <vector>
#include "Server.hpp"

class IRCServer;

class CommandHandler {
public:
    struct ParsedMessage {
        std::string command;
        std::vector<std::string> params;
        std::string trailing;
    };

    CommandHandler();
    ~CommandHandler();

    static ParsedMessage parseMessage(const std::string& raw_message);
    void handleClientMessage(int client_fd, const std::string& message, IRCServer& server);

    // Métodos de comandos
    void handleNickCommand(int client_fd, const std::string& new_nick, IRCServer& server);
    void handlePrivMsgCommand(int client_fd, const std::string& target, const std::string& message, IRCServer& server);
    void handlePartCommand(int client_fd, const std::string& channel_name, const std::string& reason, IRCServer& server);
    void handleJoinCommand(int client_fd, const std::string& channel_name_raw,const std::string& password, IRCServer& server);
    void sendToAllClients(const std::string& message, int sender_fd, IRCServer& server);
    void handleModeCommand(int client_fd, const std::string& target, const std::string& mode_str, const std::vector<std::string>& args,IRCServer& server) ;
    void handleKickCommand(int client_fd, const std::string& channel_name, const std::string& user, const std::string& reason, IRCServer& server);
    void handleInviteCommand(int client_fd, const std::string& nick, const std::string& channel_name, IRCServer& server);
    void handleTopicCommand(int client_fd, const std::string& channel_name, const std::string& new_topic, IRCServer& server); 
private:
    // Helper para enviar errores
    void sendError(int client_fd, const std::string& code, const std::string& msg);
};

#endif



// #ifndef COMMAND_HANDLER_HPP
// #define COMMAND_HANDLER_HPP

// #include <string>
// #include "Server.hpp"

// class IRCServer;
// class CommandHandler {

// public:
//     CommandHandler();
//     ~CommandHandler();

//     void handleNickCommand(int client_fd, const std::string& new_nick, IRCServer& server);
//     void handleClientMessage(int client_fd, const std::string& message, IRCServer& server);
//     void sendToAllClients(const std::string& message, int sender_fd, IRCServer& server);
//     //void handlePrivMsgCommand(int client_fd, const std::string& target, const std::string& message, IRCServer& server);
//     void handlePrivMsgCommand(int client_fd, const std::string& params, IRCServer& server);
//     void handlePartCommand(int client_fd, const std::string& channel_name, const std::string& reason, IRCServer& server);
//     void handleJoinCommand(int client_fd, const std::string& channel_name_raw, IRCServer& server);
// };

// #endif
