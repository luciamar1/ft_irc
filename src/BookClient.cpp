#include "BookClient.hpp"
#include <iostream>

bool BookClient::addClient(int fd, std::string _nick, AuthStage stage) 
{
    if (clients.find(fd) != clients.end())
        return false;
    clients[fd] = new Client(fd, _nick, stage);
    return true;
}

void BookClient::printbook()
{
    std::map<int, Client *>::iterator it = clients.begin();
        while(it != clients.end())
        {
            std::cout << "nick = " <<it->second->getNickname() <<  " fd = " << it->first  << std::endl;
            it ++;
        }
}

BookClient::BookClient()
{

}
BookClient::~BookClient()
{
    std::map<int, Client *>::iterator it = clients.begin();
        while(it != clients.end())
        {
            delete it->second;
            it ++;
        }
        clients.clear();

}

void BookClient::removeClient(int fd) {
    if(clients.find(fd) != clients.end()) {
        delete clients[fd];
        clients.erase(fd); 
    }
}
bool BookClient::nickExists(const std::string& nick) 
{
    std::map<int, Client *>::iterator it = clients.begin();
    while(it != clients.end())
    {
        if (it->second->getNickname() == nick)
        {
            
            return true;
        }
        it ++;
    }
    return false;
}

bool BookClient::fdExists(int fd)
{
    std::map<int, Client *>::iterator it = clients.find(fd);
    if(it != clients.end())
    {
        return true;
    }
    return false;
}

Client* BookClient::getClient(int fd) 
{
    if (clients.find(fd) != clients.end())
        return clients[fd];
    return NULL;
}

std::map<int, Client *>& BookClient::getmap() 
{
    return clients;
}
