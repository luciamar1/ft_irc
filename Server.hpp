#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <netinet/in.h>
#include <poll.h>

class IRCServer {
private:
    int server_fd;  // Descriptor del socket del servidor
    int port;       // Puerto
    std::string password;  // Contraseña
    std::vector<struct pollfd> clients; // Lista de clientes conectados

public:
    IRCServer(int port, const std::string& password);
    ~IRCServer();
    void run();
    void acceptClient();
    void removeClient(int client_fd);
    void handleClientData(int client_fd);
};

#endif
