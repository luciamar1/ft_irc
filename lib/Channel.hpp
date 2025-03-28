
#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
public:
    std::string name;
    std::string topic;
    std::vector<Client*> clients;
    Channel(std::string name);
};

#endif