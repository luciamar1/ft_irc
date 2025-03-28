#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sstream>

// Constructor
Server::Server(int port, const std::string& password) : 
    port(port), password(password), serverFd(-1) {
    setupServer();
}

// Destructor
Server::~Server() {
    close(serverFd);
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        delete it->second;
    }
}

/*********************** MÉTODOS PRIVADOS ***********************/

void Server::setupServer() {
    // Crear socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Error creating socket");
    }

    // Configurar dirección del servidor
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Asignar puerto al socket
    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverFd);
        throw std::runtime_error("Error binding socket");
    }

    // Escuchar conexiones
    if (listen(serverFd, MAX_CLIENTS) < 0) {
        close(serverFd);
        throw std::runtime_error("Error listening");
    }

    // Configurar modo no bloqueante
    setNonBlocking(serverFd);

    // Añadir socket del servidor a pollFds
    struct pollfd serverPollFd;
    serverPollFd.fd = serverFd;
    serverPollFd.events = POLLIN;
    pollFds.push_back(serverPollFd);

    std::cout << "Server started on port " << port << std::endl;
}

void Server::setNonBlocking(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        throw std::runtime_error("Error setting non-blocking mode");
    }
}

void Server::acceptClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);

    if (clientFd < 0) {
        std::cerr << "Error accepting client" << std::endl;
        return;
    }

    setNonBlocking(clientFd);

    // Añadir cliente a la lista
    struct pollfd newPollFd;
    newPollFd.fd = clientFd;
    newPollFd.events = POLLIN;
    pollFds.push_back(newPollFd);

    clients[clientFd] = new Client(clientFd);
    std::cout << "New client connected: " << clientFd << std::endl;
}

void Server::handleClientInput(int clientFd) {
    char buffer[BUFFER_SIZE];
    int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        removeClient(clientFd);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string message(buffer);
    
    processCommand(clients[clientFd], message);
}

void Server::processCommand(Client* client, const std::string& command) {
    
    

    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    // Implementar lógica de comandos aquí
    if (cmd == "NICK") {
        std::string nickname;
        iss >> nickname;
        client->setNickname(nickname);
        std::cout << "Nickname set to: " << nickname << std::endl;
    }
    std::cout << "Processing command: " << command << std::endl;
}

/*********************** MÉTODOS PÚBLICOS ***********************/

const std::string& Server::getPassword() const {
    return password;
}

void Server::removeClient(int clientFd) {
    // Eliminar de pollFds
    for (size_t i = 0; i < pollFds.size(); ++i) {
        if (pollFds[i].fd == clientFd) {
            pollFds.erase(pollFds.begin() + i);
            break;
        }
    }

    // Eliminar de clientes
    if (clients.find(clientFd) != clients.end()) {
        delete clients[clientFd];
        clients.erase(clientFd);
    }

    close(clientFd);
    std::cout << "Client disconnected: " << clientFd << std::endl;
}

void Server::run() {
    while (true) {
        int ret = poll(&pollFds[0], pollFds.size(), -1);
        if (ret < 0) {
            std::cerr << "Poll error" << std::endl;
            continue;
        }

        for (size_t i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents & POLLIN) {
                if (pollFds[i].fd == serverFd) {
                    acceptClient();
                } else {
                    handleClientInput(pollFds[i].fd);
                }
            }
        }
    }
}