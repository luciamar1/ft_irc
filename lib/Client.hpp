#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
public:
    Client(int fd);
    ~Client();
    
    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    bool isRegistered() const;
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void registerUser();
    
private:
    int fd;
    std::string nickname;
    std::string username;
    bool registered;
};

#endif