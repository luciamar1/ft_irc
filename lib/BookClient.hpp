#ifndef BOOK_CLIENT_HPP
#define BOOK_CLIENT_HPP

#include <map>
#include "Client.hpp"

class BookClient 
{
    private:
        std::map<int, Client *> clients;

    public:
        BookClient();
        ~BookClient();
        bool addClient(int fd, std::string nick, std::string user, AuthStage stage) ;
        void    removeClient(int fd);
        bool    nickExists(const std::string& nick);
        bool    fdExists(int fd);

        Client* getClientByNick(const std::string& nick);
        Client* getClient(int fd);
        std::map<int, Client *>&    getmap();
        void printbook();
};

#endif
