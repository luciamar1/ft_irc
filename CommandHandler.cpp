
#include "CommandHandler.hpp"
#include <iostream>

void CommandHandler::processCommand(int client_fd, const std::string &command) {
    std::cout << "Processing command: " << command << std::endl;
}
