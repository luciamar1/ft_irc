
#include "Channel.hpp"
#include "Client.hpp"
#include <algorithm>

Channel::Channel(const std::string& name, Client* founder) : 
    name(name), topic(""), key(""), userLimit(0), modes(0) {
    addClient(founder);
    addOperator(founder);
}

const std::string& Channel::getName() const { return name; }
const std::string& Channel::getTopic() const { return topic; }
void Channel::setTopic(const std::string& topic) { this->topic = topic; }

void Channel::addClient(Client* client) {
    if (std::find(clients.begin(), clients.end(), client) == clients.end()) {
        clients.push_back(client);
    }
}

void Channel::removeClient(Client* client) {
    clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
    removeOperator(client);
}

void Channel::broadcast(const std::string& message, Client* sender) {
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (*it != sender) {
            send((*it)->getFd(), message.c_str(), message.size(), 0);
        }
    }
}

bool Channel::isOperator(Client* client) const {
    return std::find(operators.begin(), operators.end(), client) != operators.end();
}

void Channel::addOperator(Client* client) {
    if (!isOperator(client)) {
        operators.push_back(client);
    }
}

void Channel::removeOperator(Client* client) {
    operators.erase(std::remove(operators.begin(), operators.end(), client), operators.end());
}