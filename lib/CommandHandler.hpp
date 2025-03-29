#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sstream>
#include <map>
#include <cstdio>

class CommandHandler {
public:
    CommandHandler();
    ~CommandHandler();
    void handleNickCommand(int client_fd, const std::string& new_nick, IRCServer &server );
    void handleClientMessage(int client_fd, const std::string& message, IRCServer &server ) ;
    void sendToAllClients(const std::string& message, int sender_fd, IRCServer &server) ;

};
#endif