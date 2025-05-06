#include "Client.hpp"

Client::Client() : fd(-1), nickname(""), realname("") {}

// Client::Client(int fd, std::string _nick) : fd(fd), nickname(_nick), realname("") {}

Client::Client(int fd, std::string nick, AuthStage stage )
    : fd(fd), nickname(nick), buffer(""), stage(stage) {}

std::string& Client::getBuffer() 
{ 
    return buffer; 
}

int Client::getFd() const {
    return fd;
}

void Client::setFd(int _fd) {
    fd = _fd;
}

std::string Client::getNickname() const {
    return nickname;
}

void Client::setNickname(const std::string& nick) {
    nickname = nick;
}

std::string Client::getRealname() const {
    return realname;
}

void Client::setRealname(const std::string& name) {
    realname = name;
}


AuthStage Client::getStage() const 
{ 
    return stage; 
}

void Client::setStage(AuthStage s) 
{ 
    stage = s; 
}