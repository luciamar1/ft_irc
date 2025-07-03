#include "BookClient.hpp"
#include <iostream>

bool BookClient::addClient(int fd, std::string nick, std::string user, AuthStage stage) 
{
    if (clients.find(fd) != clients.end()) return false;

    Client* new_client = NULL;
    try {
        new_client = new Client(fd, nick, user, stage);
    } 
    catch (const std::bad_alloc& e) {
        std::cerr << "Error: could not allocate memory for new Client: " << e.what() << std::endl;
        return false;
    }

    try {
        clients.insert(std::make_pair(fd, new_client));
    } 
    catch (...) {
        delete new_client;
        return false;
    }
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
    std::map<int, Client*>::iterator it = clients.find(fd);
    if (it != clients.end()) {
        delete it->second;   // Liberar memoria del Client
        clients.erase(it);   // Eliminar entrada del mapa
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

Client* BookClient::getClientByNick(const std::string& nick) 
{
    std::map<int, Client*>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getNickname() == nick) {
            return it->second;
        }
    }
    return NULL;
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
