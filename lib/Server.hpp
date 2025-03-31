#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>      
#include <netinet/in.h>
#include <poll.h>
class CommandHandler; 
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
    
    std::map<int, std::string>& getClientsInfo();  
    void setClientsInfo(int _client_fd, std::string _nick);
    

    void run();
    void acceptClient(CommandHandler &handler);
    void removeClient(int client_fd);
    void handleClientData(int client_fd, CommandHandler &handler);


};

#endif
