#include "Client.hpp"

Client::Client() : fd(-1), nickname(""), realname("") {}

// Client::Client(int fd, std::string _nick) : fd(fd), nickname(_nick), realname("") {}

Client::Client(int fd, std::string _nick, std::string _user, AuthStage stage )
    : fd(fd), nickname(_nick), realname(_user),buffer(""), stage(stage) {}

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

void Client::joinChannel(const std::string& channelName) {
    joinedChannels.insert(channelName);
}

void Client::leaveChannel(const std::string& channelName) {
    joinedChannels.erase(channelName);
}

bool Client::isInChannel(const std::string& channelName) const {
    return joinedChannels.find(channelName) != joinedChannels.end();
}

const std::set<std::string>& Client::getJoinedChannels() const {
    return joinedChannels;
}

std::string& Client::getOutputBuffer() 
{ 
    return output_buffer; 
}

void Client::safeSend(const std::string& message) {
    output_buffer += message;
}