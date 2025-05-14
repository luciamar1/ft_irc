#include "ChannelBook.hpp"
#include <iostream>

Channel* ChannelBook::getChannel(const std::string& name) {
    if (_channels.find(name) != _channels.end())
        return _channels[name];
    return NULL;
}

bool ChannelBook::addChannel(const std::string& name) 
{
    if (_channels.find(name) != _channels.end())
    
        return true;

    try 
    {
        _channels[name] = new Channel(name);
    } 
    catch (const std::bad_alloc& e) 
    {
        std::cerr << "Error: could not allocate memory for new Channel: " << e.what() << std::endl;
        removeChannel(name); 
        return(false);
    }
    return true;

}

void ChannelBook::removeChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end()) {
        delete it->second;
        _channels.erase(it);
    }
}

bool ChannelBook::channelExists(const std::string& name) const {
    return _channels.find(name) != _channels.end();
}

std::map<std::string, Channel*>& ChannelBook::getAllChannels() {
    return _channels;
}

