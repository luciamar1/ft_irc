#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include "Common.hpp"
#include <vector>
#include <map>
#include <poll.h>

class Server {
public:
    Server(int port, const std::string& password);
    ~Server();
    
    void run();
    void removeClient(int clientFd); // Método público
    const std::string& getPassword() const; // Getter público

private:

    int port;
    std::string password;
    int serverFd;
    std::vector<struct pollfd> pollFds;
    std::map<int, Client*> clients;
    std::map<std::string, Channel*> channels;

    void setupServer();
    void acceptClient();
    void handleClientInput(int clientFd);
    void processCommand(Client* client, const std::string& command);
    void setNonBlocking(int fd);
    Client* getClientByNick(const std::string& nickname);
    Channel* getChannel(const std::string& name);
    void createChannel(const std::string& name, Client* founder);
    
};

#endif