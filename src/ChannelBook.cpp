#include "ChannelBook.hpp"

Channel* ChannelBook::getChannel(const std::string& name) {
    if (_channels.find(name) != _channels.end())
        return _channels[name];
    return NULL;
}

void ChannelBook::addChannel(const std::string& name, Channel* channel) {
    _channels[name] = channel;
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
