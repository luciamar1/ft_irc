#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Server.hpp"

class CommandHandler {
public:
    static void handlePass(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleNick(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleUser(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleJoin(Server& server, Client* client, const std::vector<std::string>& args);
    static void handlePrivmsg(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleMode(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleKick(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleInvite(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleTopic(Server& server, Client* client, const std::vector<std::string>& args);
    static void handleQuit(Server& server, Client* client, const std::vector<std::string>& args);
};

#endif