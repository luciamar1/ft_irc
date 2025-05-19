#include "Server.hpp"
#include "CommandHandler.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <arpa/inet.h>
#include <poll.h>
#include <sstream>
#include <map>
#include <cstdio> // Necesario para perror()

// Constructor del servidor
IRCServer::IRCServer(int port, const std::string& password) : port(port), password(password) {
    struct sockaddr_in server_addr;

    // Crear el socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::perror("socket");  // Si falla al crear el socket, termina el programa
        std::exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces de red
    server_addr.sin_port = htons(port);  // Asignar el puerto proporcionado

    // Asociar el socket con la dirección del servidor
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::perror("bind");  // Si falla la asociación, termina el programa
        std::exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, 10) < 0) {
        std::perror("listen");  // Si falla al escuchar, termina el programa
        std::exit(EXIT_FAILURE);
    }

    // Preparar el conjunto de clientes con el socket del servidor
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;  // Indica que estamos esperando eventos de lectura
    clients.push_back(pfd);  // Añadir el servidor a la lista de clientes

    std::cout << "IRC Server running on port " << port << " and awaiting connections..." << std::endl;
}

BookClient& IRCServer::getClientsBook()
{
    return this->clients_info;
}
ChannelBook& IRCServer::getChannelsBook()
{
    return this->channels;
}

// Destructor del servidor
IRCServer::~IRCServer() {
    close(server_fd);  // Cerrar el socket del servidor
    // Cerrar todos los sockets de clientes
    for (size_t i = 0; i < clients.size(); ++i) {
        close(clients[i].fd);
    }
    std::cout << "Server has shut down. All client connections have been closed." << std::endl;
}





// Solicitar el nickname

void IRCServer::acceptClient() 
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::perror("accept");
        return;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::perror("fcntl");
        close(client_fd);
        return;
    }

    // Añadir a la lista de clientes para poll
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);

    // Añadir cliente en estado de espera de contraseña
    clients_info.addClient(client_fd, "", "", WAITING_PASSWORD);

    const char* welcome_msg = "Welcome to the IRC Server! Please enter the password:\n";
    send(client_fd, welcome_msg, strlen(welcome_msg), 0);
}


// Eliminar cliente

void IRCServer::removeClient(int client_fd) {
    if (clients_info.fdExists(client_fd)) {
        std::cout << "Client with nickname: " << clients_info.getClient(client_fd)->getNickname()
                  << " with file descriptor: " << client_fd << " has disconnected." << std::endl;
        clients_info.removeClient(client_fd);
    }

    close(client_fd); // Cerrar primero

    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            clients.erase(clients.begin() + i);  // Luego borrar el pollfd
            break;
        }
    }
}


// Ejecutar el servidor y manejar los eventos
void IRCServer::run() {
    CommandHandler handler;
    while (true) {
        if (clients.empty()) continue;
        if (poll(&clients[0], clients.size(), -1) < 0) {
            std::perror("poll");
            continue;
        }
        // Manejar los eventos para cada cliente
        for (int i = static_cast<int>(clients.size()) - 1; i >= 0; --i) 
        {
            if (clients[i].revents & POLLIN) {
                if (clients[i].fd == server_fd) 
                {
                    acceptClient();  // Aceptar un nuevo cliente
                } 
                else 
                {
                    handleClientData(clients[i].fd, handler);  // Manejar los datos del cliente
                }
            }
        }
        
    }
}



void IRCServer::handleClientData(int client_fd, CommandHandler &handler) {
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;  // No hay datos ahora
        std::perror("recv handleClientData");
        removeClient(client_fd);
        return;
    }
    if (bytes_received == 0) {
        removeClient(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    Client* client = clients_info.getClient(client_fd);
    if (!client) return;

    client->getBuffer() += buffer;
    size_t pos;
    while ((pos = client->getBuffer().find('\n')) != std::string::npos) {
        std::string line = client->getBuffer().substr(0, pos);
        client->getBuffer().erase(0, pos + 1);
        line.erase(line.find_last_not_of("\r\n") + 1);

        if (client->getStage() == WAITING_PASSWORD) {
            if (line != password) {
                send(client_fd, "Incorrect password. Connection closed.\n", 40, 0);
                removeClient(client_fd);
                return;
            }
            client->setStage(WAITING_NICKNAME);
            send(client_fd, "Please enter your nickname:\n", 29, 0);
            return;
        }

        if (client->getStage() == WAITING_NICKNAME) {
            line.erase(0, line.find_first_not_of(" "));
            line.erase(line.find_last_not_of(" ") + 1);
            if (line.empty() || clients_info.nickExists(line)) {
                send(client_fd, "Nickname invalid or taken. Try again:\n", 38, 0);
                return;
            }
            client->setNickname(line);
            client->setStage(WAITING_USERNAME);
            send(client_fd, "Please enter your real name:\n", 29, 0);
            return;
        }

        if (client->getStage() == WAITING_USERNAME)
        {
            line.erase(0, line.find_first_not_of(" "));
            line.erase(line.find_last_not_of(" ") + 1);
            if (line.empty() ) {
                send(client_fd, "Empty username. Try again:\n", 38, 0);
                return;
            }
            client->setRealname(line);
            client->setStage(CONNECTED);
            std::cout << "New client " << line << " connected with fd " << client_fd << ".\n";
            send(client_fd, "You have successfully authenticated and joined the server.\n", 60, 0);
            return;
        }



        if (client->getStage() == CONNECTED) {
            std::string sender_nick = client->getNickname();
            std::string full_message = "[Message from: " + sender_nick + "]: " + line + "\n";

            std::cout << "--------------------------------------------\n";
            std::cout << "[Message from: \033[1;34m" << sender_nick << "\033[0m]\n";
            std::cout << "--------------------------------------------\n";
            std::cout << "\033[1;32m" << line << "\033[0m\n";
            std::cout << "--------------------------------------------\n";

            handler.handleClientMessage(client_fd, line, *this);
            //handler.sendToAllClients(full_message, client_fd, *this);
        }
    }
}
