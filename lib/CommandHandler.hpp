#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include <string>

class CommandHandler {
public:
    static void processCommand(int client_fd, const std::string &command);
};
#endif