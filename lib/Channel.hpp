
// Channel.hpp
#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
#include <vector>
#include <sys/socket.h> 

class Client; // forward declaration

class Channel {
private:
	std::string _name;
	std::string _topic;
	std::string _password;
	size_t      _userLimit;
	bool        _inviteOnly;
	bool        _topicRestricted;

	std::set<Client*> _clients;
	std::set<Client*> _operators;
	std::set<Client*> _invited;

public:
	Channel(const std::string& name);

	// Getters y setters
	const std::string& getName() const;
	const std::string& getTopic() const;
	void setTopic(const std::string& topic);

	bool isInviteOnly() const;
	void setInviteOnly(bool val);

	bool isTopicRestricted() const;
	void setTopicRestricted(bool val);

	const std::string& getPassword() const;
	void setPassword(const std::string& pass);

	size_t getUserLimit() const;
	void setUserLimit(size_t limit);

	// Gestión de usuarios
	void addClient(Client* client);
	void removeClient(Client* client);
	bool hasClient(Client* client) const;

	// Operadores
	void addOperator(Client* client);
	void removeOperator(Client* client);
	bool isOperator(Client* client) const;

	// Invitados
	void inviteClient(Client* client);
	bool isInvited(Client* client) const;
 	void removeInvited(Client* client);
	
	const std::set<Client*>& getClients() const;

	void sendToAll(const std::string& message, Client* sender);

	//Mode
	void setMode(char mode, bool enabled, const std::string& arg = "");
	std::string getModeString() const;
};

#endif
