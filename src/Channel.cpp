#include "Channel.hpp"
#include "Client.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>



// Constructor de Channel
Channel::Channel(const std::string& name)
	: _name(name), _topic(""), _password(""), _userLimit(0), _inviteOnly(false), _topicRestricted(false) {}

// Getter para el nombre del canal
const std::string& Channel::getName() const {
	return _name;
}

// Getter y setter para el topic del canal
const std::string& Channel::getTopic() const {
	return _topic;
}

void Channel::setTopic(const std::string& topic) {
	_topic = topic;
}

// Comprobación de si el canal es solo por invitación
bool Channel::isInviteOnly() const {
	return _inviteOnly;
}

void Channel::setInviteOnly(bool val) {
	_inviteOnly = val;
}

// Comprobación de si el canal tiene restricciones en el tema
bool Channel::isTopicRestricted() const {
	return _topicRestricted;
}

void Channel::setTopicRestricted(bool val) {
	_topicRestricted = val;
}

// Getter y setter para la contraseña del canal
const std::string& Channel::getPassword() const {
  
    
	return _password;
}

void Channel::setPassword(const std::string& pass) {
	_password = pass;
}

// Getter y setter para el límite de usuarios en el canal
size_t Channel::getUserLimit() const {
	return _userLimit;
}

void Channel::setUserLimit(size_t limit) {
	_userLimit = limit;
}

// Agregar un cliente al canal
void Channel::addClient(Client* client) {
	_clients.insert(client);
}

// Eliminar un cliente del canal
void Channel::removeClient(Client* client) {
	_clients.erase(client);
    _operators.erase(client);  // También quitar de operadores si lo era
    _invited.erase(client); 
}

// Verificar si un cliente está en el canal
bool Channel::hasClient(Client* client) const {
	return _clients.find(client) != _clients.end();
}

// Agregar un operador al canal
void Channel::addOperator(Client* client) {
	_operators.insert(client);
}

// Eliminar un operador del canal
void Channel::removeOperator(Client* client) {
	_operators.erase(client);
}

// Verificar si un cliente es operador en el canal
bool Channel::isOperator(Client* client) const {
	return _operators.find(client) != _operators.end();
}

// Invitar a un cliente al canal
void Channel::inviteClient(Client* client) {
	_invited.insert(client);
}

// Verificar si un cliente ha sido invitado
bool Channel::isInvited(Client* client) const {
	return _invited.find(client) != _invited.end();
}

// Obtener la lista de clientes del canal
const std::set<Client*>& Channel::getClients() const {
	return _clients;
}

void Channel::removeInvited(Client* client) {
    _invited.erase(client);
}

// void Channel::sendToAll(const std::string& message, Client* sender)
// {
//     for (std::set<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) 
// 	{
//         Client* client = *it;

//         if (client == NULL)
//             continue;

//         // Si hay sender, y el cliente es el sender, lo saltamos
//         if (sender && client->getFd() == sender->getFd())
//             continue;

//         send(client->getFd(), message.c_str(), message.length(), 0);
//     }
// }
void Channel::sendToAll(const std::string& message, Client* sender)
{
    for (std::set<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) 
    {
        Client* client = *it;
        if (!client) 
            continue;
        if (sender && client == sender)
            continue;

        client->getOutputBuffer() += message; 
    }
}

void Channel::setMode(char mode, bool enabled, const std::string& arg) 
{
    switch (mode) {
        case 'i': _inviteOnly = enabled; break;
        case 't': _topicRestricted = enabled; break;
        case 'k': 
            if (enabled) _password = arg;
            else _password = "";
            break;
        case 'o':
            // Implementado en los métodos addOperator/removeOperator
            break;
        case 'l':
            if (enabled) _userLimit = std::atoi(arg.c_str());
            else _userLimit = 0;
            break;
    }
}

std::string Channel::getModeString() const 
{
    std::ostringstream oss;
    oss << "+";
    if (_inviteOnly) oss << "i";
    if (_topicRestricted) oss << "t";
    if (!_password.empty()) oss << "k";
    if (_userLimit > 0) oss << "l";
    return oss.str();
}