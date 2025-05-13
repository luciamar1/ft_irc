#ifndef CHANNELBOOK_HPP
#define CHANNELBOOK_HPP

#include <map>
#include <string>
#include "Channel.hpp"

class ChannelBook {
    
    private:
        std::map<std::string, Channel*> _channels;
public:
    Channel* getChannel(const std::string& name);
    bool addChannel(const std::string& name, Channel* channel);
    void removeChannel(const std::string& name);
    bool channelExists(const std::string& name) const;
    std::map<std::string, Channel*>& getAllChannels();
};

#endif
