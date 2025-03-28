#include "Client.hpp"

Client::Client(int fd) : fd(fd) {}

// Implementación correcta de getFd()
int Client::getFd() const {
    return fd;
}

// Destructor opcional
Client::~Client() {}
