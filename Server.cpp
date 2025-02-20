
#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sstream>
#include <map>
#include <cstdio> // Necesario para perror()

IRCServer::IRCServer(int port, const std::string& password) : port(port), password(password) {
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::perror("socket");
        std::exit(EXIT_FAILURE);
    }

    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::perror("bind");
        std::exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        std::perror("listen");
        std::exit(EXIT_FAILURE);
    }

    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);

    std::cout << "IRC Server running on port " << port << std::endl;
}

IRCServer::~IRCServer() {
    close(server_fd);
    for (size_t i = 0; i < clients.size(); ++i) {
        close(clients[i].fd);
    }
    std::cout << "Server shut down." << std::endl;
}

void IRCServer::acceptClient() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::perror("accept");
        return;
    }

    const char *request = "Welcome to the IRC Server! Please enter the password:\n";
    send(client_fd, request, strlen(request), 0);

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string received_password(buffer);
    received_password.erase(received_password.find_last_not_of("\r\n") + 1);

    if (received_password != password) {
        const char *error_msg = "Incorrect password. Connection closed.\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        close(client_fd);
        return;
    }

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);

    clients_info[client_fd] = ""; // Inicializa el nickname como vacío
    std::cout << "New client connected." << std::endl;
}

void IRCServer::removeClient(int client_fd) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            close(client_fd);
            clients.erase(clients.begin() + i);
            clients_info.erase(client_fd);
            std::cout << "Client disconnected." << std::endl;
            break;
        }
    }
}

void IRCServer::run() {
    while (true) {
        if (poll(&clients[0], clients.size(), -1) < 0) {
            std::perror("poll");
            continue;
        }

        for (size_t i = 0; i < clients.size(); ++i) {
            if (clients[i].revents & POLLIN) {
                if (clients[i].fd == server_fd) {
                    acceptClient();
                } else {
                    handleClientData(clients[i].fd);
                }
            }
        }
    }
}

void IRCServer::handleNickCommand(int client_fd, const std::string& new_nick) {
    std::string trimmed_nick = new_nick;
    trimmed_nick.erase(trimmed_nick.find_last_not_of("\r\n") + 1);

    if (trimmed_nick.empty()) {
        std::string error_msg = "ERROR: Nickname cannot be empty.\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
        return;
    }

    // Reemplazo de range-based for loop por iterador tradicional
    for (std::map<int, std::string>::iterator it = clients_info.begin(); it != clients_info.end(); ++it) {
        if (it->second == trimmed_nick) {
            std::string error_msg = "ERROR: Nickname already in use.\n";
            send(client_fd, error_msg.c_str(), error_msg.length(), 0);
            return;
        }
    }

    clients_info[client_fd] = trimmed_nick;
    std::string success_msg = "Nickname set to: " + trimmed_nick + "\n";
    send(client_fd, success_msg.c_str(), success_msg.length(), 0);
}

void IRCServer::handleClientMessage(int client_fd, const std::string& message) {
    std::istringstream iss(message);
    std::string command, params;

    iss >> command;
    std::getline(iss, params);

    if (!params.empty() && params[0] == ' ') {
        params = params.substr(1);
    }

    if (command == "NICK") {
        handleNickCommand(client_fd, params);
    } else if (command == "QUIT") {
        removeClient(client_fd);
    } else {
        std::string error_msg = "ERROR: Unknown command\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
    }
}

// void IRCServer::handleClientData(int client_fd) {
//     char buffer[512];
//     int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
//     if (bytes_received <= 0) {
//         removeClient(client_fd);
//         return;
//     }

//     buffer[bytes_received] = '\0';
//     std::string message(buffer);

//     // Quitar saltos de línea y espacios extra
//     message.erase(message.find_last_not_of("\r\n") + 1);

//     // Si el cliente tiene un nick, lo mostramos en los logs
//     std::string sender_nick = (clients_info.find(client_fd) != clients_info.end()) 
//                               ? clients_info[client_fd] 
//                               : "Unknown";

//     std::cout << "Received from " << sender_nick << ": " << message << std::endl;

//     // Manejo de comandos
//     if (message.find("/quit") == 0) {
//         removeClient(client_fd);
//     }
//     else if (message.find("/nick") == 0) {
//         std::string new_nick = message.substr(6);  // Extraer el nuevo nick
//         if(new_nick)
//             handleNickCommand(client_fd, new_nick);
//     }
//     else {
//         send(client_fd, "Unknown command\n", 16, 0);
//     }
// }

void IRCServer::handleClientData(int client_fd) {
    char buffer[512];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        removeClient(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);

    // Quitar saltos de línea y espacios extra
    message.erase(message.find_last_not_of("\r\n") + 1);

    // Obtener el nickname del usuario
    std::string sender_nick = (clients_info.find(client_fd) != clients_info.end()) 
                              ? clients_info[client_fd] 
                              : "Unknown";

    std::cout << "Received from " << sender_nick << ": " << message << std::endl;

    // Manejo de comandos
    if (message.find("/quit") == 0) {
        removeClient(client_fd);
    }
    else if (message.find("/nick") == 0) {
        // Verificar si hay un argumento después de "/nick"
        if (message.length() <= 6 || message.substr(6).find_first_not_of(" ") == std::string::npos) {
            std::string error_msg = "Error: Nickname cannot be empty.\n";
            send(client_fd, error_msg.c_str(), error_msg.size(), 0);
            return;
        }

        std::string new_nick = message.substr(6);
        
        // Eliminar espacios en blanco extra
        new_nick.erase(0, new_nick.find_first_not_of(" "));
        new_nick.erase(new_nick.find_last_not_of(" ") + 1);

        handleNickCommand(client_fd, new_nick);
    }
}