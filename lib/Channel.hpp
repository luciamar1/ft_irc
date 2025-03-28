#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Client.hpp"
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h> 

class Channel {
public:
    Channel(const std::string& name, Client* founder);
    
    const std::string& getName() const;
    const std::string& getTopic() const;
    void setTopic(const std::string& topic);
    void addClient(Client* client);
    void removeClient(Client* client);
    void broadcast(const std::string& message, Client* sender = NULL);
    bool isOperator(Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);
    
private:
    std::string name;
    std::string topic;
    std::vector<Client*> clients;
    std::vector<Client*> operators;
    std::string key;
    unsigned int userLimit;
    int modes;
};

#endif