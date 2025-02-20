#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>      
#include <netinet/in.h>
#include <poll.h>

class IRCServer {
private:
    int server_fd;  // Descriptor del socket del servidor
    int port;       // Puerto
    std::string password;  // Contraseña
    std::vector<struct pollfd> clients; // Lista de clientes conectados
    std::map<int, std::string> clients_info; // Mapa de clientes con sus nombres
    std::map<int, std::string> clients_realname; // Mapa de cliente a realname

public:
    IRCServer(int port, const std::string& password);
    ~IRCServer();
    
    void run();
    void acceptClient();
    void removeClient(int client_fd);
    void handleClientData(int client_fd);
    void handleNickCommand(int client_fd, const std::string& new_nick);
    void handleUserCommand(int client_fd, const std::string& params);
    void handleClientMessage(int client_fd, const std::string& message);
};

#endif
