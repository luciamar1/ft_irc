#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

enum AuthStage 
{
    WAITING_PASSWORD,
    WAITING_NICKNAME,
    WAITING_USERNAME,
    CONNECTED
};

class Client {
private:
    int fd;
    // static const size_t MAX_BUFFER_SIZE = 10240; // 10KB
    std::string nickname;
    std::string realname;
    std::string buffer;
    AuthStage stage;
    std::set<std::string> joinedChannels;

public:
    static const size_t MAX_BUFFER_SIZE = 10240;
    Client();
    Client(int fd, std::string _nick,std::string _user,  AuthStage _stage = WAITING_PASSWORD);

    int getFd() const;
    void setFd(int _fd);

    AuthStage getStage() const ;
    void setStage(AuthStage s) ;

    std::string getNickname() const;
    void setNickname(const std::string& nick);

    std::string getRealname() const;
    void setRealname(const std::string& name);

    std::string& getBuffer();

    void joinChannel(const std::string& channelName);
    void leaveChannel(const std::string& channelName);
    bool isInChannel(const std::string& channelName) const;
    const std::set<std::string>& getJoinedChannels() const;

};

#endif
