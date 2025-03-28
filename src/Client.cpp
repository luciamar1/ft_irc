#include "Client.hpp"

Client::Client(int fd) : fd(fd), registered(false) {}
Client::~Client() {}

int Client::getFd() const { return fd; }
const std::string& Client::getNickname() const { return nickname; }
const std::string& Client::getUsername() const { return username; }
bool Client::isRegistered() const { return registered; }

void Client::setNickname(const std::string& nickname) {
    this->nickname = nickname;
}

void Client::setUsername(const std::string& username) {
    this->username = username;
}

void Client::registerUser() {
    if (!nickname.empty() && !username.empty()) {
        registered = true;
    }
}