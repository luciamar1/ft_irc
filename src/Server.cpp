// #include "Server.hpp"
// #include "CommandHandler.hpp"
// #include <iostream>
// #include <cstdlib>
// #include <cstring>
// #include <fcntl.h> 
// #include <unistd.h>
// #include <sys/socket.h>
// #include <cerrno>
// #include <arpa/inet.h>
// #include <poll.h>
// #include <sstream>
// #include <map>
// #include <cstdio> // Necesario para perror()


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
#include <cstdio>

// Implementación de safeSend
void IRCServer::safeSend(int client_fd, const std::string& message) {
    Client* client = clients_info.getClient(client_fd);
    if (client) {
        client->getOutputBuffer() += message;
    }
}

IRCServer::IRCServer(int port, const std::string& password) : port(port), password(password) {
    struct sockaddr_in server_addr;

    // Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::perror("socket");
        std::exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::perror("setsockopt");
        std::exit(EXIT_FAILURE);
    }

    // Configurar dirección
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::perror("bind");
        std::exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        std::perror("listen");
        std::exit(EXIT_FAILURE);
    }

    // Configurar non-blocking
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
        std::perror("fcntl");
        std::exit(EXIT_FAILURE);
    }

    // Preparar poll
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);

    std::cout << "IRC Server running on port " << port << std::endl;
}

BookClient& IRCServer::getClientsBook() {
    return this->clients_info;
}

ChannelBook& IRCServer::getChannelsBook() {
    return this->channels;
}

IRCServer::~IRCServer() {
    close(server_fd);
    
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd != server_fd) {
            close(clients[i].fd);
        }
    }
    std::cout << "Server has shut down." << std::endl;
}

void IRCServer::acceptClient() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            std::perror("accept");
            break;
        }

        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
            std::perror("fcntl");
            close(client_fd);
            continue;
        }

        struct pollfd pfd;
        pfd.fd = client_fd;
        pfd.events = POLLIN;
        clients.push_back(pfd);

        clients_info.addClient(client_fd, "", "", WAITING_PASSWORD);

        // Usar safeSend en lugar de send directo
        safeSend(client_fd, "Welcome to the IRC Server! Please enter the password:\n");
    }
}

void IRCServer::removeClient(int client_fd) 
{
    // ... (código existente sin cambios) ...

      Client* client = clients_info.getClient(client_fd);
    bool client_was_registered = false;
    std::string nickname = "";
    std::string realname = "";  // Añadimos para almacenar realname seguro
    
    // Paso 1: Eliminar al cliente de todos los canales donde está unido
    if (client) 
    {
        client_was_registered = true;
        nickname = client->getNickname();
        realname = client->getRealname();  // Almacenar ANTES de cualquier operación
        
        // Hacer copia de los canales porque leaveChannel modifica el set original
        std::set<std::string> joinedChannels = client->getJoinedChannels();
        
        for (std::set<std::string>::const_iterator it = joinedChannels.begin(); 
             it != joinedChannels.end(); ++it) {
            const std::string& channel_name = *it;
            Channel* channel = channels.getChannel(channel_name);
            
            // VERIFICACIÓN CRÍTICA: Asegurarnos que el canal existe
            if (channel) {
                // Construir mensaje de QUIT ANTES de modificar el cliente
                std::string quit_msg = ":" + nickname + "!~" + realname +  // Usar valor almacenado
                                      "@localhost QUIT :Connection closed\r\n";
                channel->sendToAll(quit_msg, NULL);
                
                // Eliminar al cliente del canal
                channel->removeClient(client);
                
                // Si el canal queda vacío, eliminarlo
                if (channel->getClients().empty()) {
                    channels.removeChannel(channel_name);
                }
            }
        }
    }

    // Paso 2: Cerrar conexión y eliminar de estructuras
    close(client_fd);
    
    // Eliminar de la lista de poll
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            clients.erase(clients.begin() + i);
            break;
        }
    }
    
    // Eliminar del libro de clientes
    clients_info.removeClient(client_fd);
    
    // Paso 3: Log
    if (client_was_registered) {
        std::cout << "Client with nickname: " << nickname
                  << " (fd: " << client_fd << ") has disconnected." << std::endl;
    } else {
        std::cout << "Unregistered client (fd: " << client_fd << ") has disconnected." << std::endl;
    }
    
}

void IRCServer::run() {
    CommandHandler handler;
    while (true) {
        if (clients.empty()) continue;

        // 1. Configurar eventos POLLOUT si hay datos pendientes
        for (size_t i = 0; i < clients.size(); ++i) {
            if (clients[i].fd != server_fd) {
                Client* client = clients_info.getClient(clients[i].fd);
                if (client && !client->getOutputBuffer().empty()) {
                    clients[i].events = POLLIN | POLLOUT;
                } else {
                    clients[i].events = POLLIN;
                }
            }
        }

        // 2. Esperar eventos
        if (poll(&clients[0], clients.size(), -1) < 0) {
            std::perror("poll");
            continue;
        }

        // 3. Manejar eventos
        for (int i = static_cast<int>(clients.size()) - 1; i >= 0; --i) {
            // Manejar errores/desconexiones primero
            if (clients[i].revents & (POLLERR | POLLHUP)) {
                removeClient(clients[i].fd);
                continue;
            }
            
            // Eventos de lectura
            if (clients[i].revents & POLLIN) {
                if (clients[i].fd == server_fd) {
                    acceptClient();
                } else {
                    handleClientData(clients[i].fd, handler);
                }
            }
            
            // Eventos de escritura
            if (clients[i].revents & POLLOUT) {
                Client* client = clients_info.getClient(clients[i].fd);
                if (client) {
                    std::string& output = client->getOutputBuffer();
                    if (!output.empty()) {
                        ssize_t sent = send(clients[i].fd, output.c_str(), output.size(), MSG_DONTWAIT);
                        if (sent > 0) {
                            output.erase(0, sent);
                        }
                    }
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

    // ========= PROTECCIÓN CONTRA DoS ========= //
    // Verificar si el nuevo dato excede el límite del buffer
    if (client->getBuffer().size() + bytes_received > Client::MAX_BUFFER_SIZE) {
        std::cerr << "Client fd " << client_fd << " exceeded buffer limit (" 
                  << Client::MAX_BUFFER_SIZE << " bytes). Disconnecting." << std::endl;
        removeClient(client_fd);
        return;
    }
    
    // Añadir datos al buffer
    client->getBuffer() += buffer;
    
    // Procesar líneas completas
    size_t pos;
    while ((pos = client->getBuffer().find('\n')) != std::string::npos) {
        std::string line = client->getBuffer().substr(0, pos);
        client->getBuffer().erase(0, pos + 1);
        line.erase(line.find_last_not_of("\r\n") + 1);

        if (client->getStage() == WAITING_PASSWORD) {
            if (line != password) {
                // CAMBIO: send -> safeSend
                safeSend(client_fd, "Incorrect password. Connection closed.\n");
                removeClient(client_fd);
                return;
            }
            client->setStage(WAITING_NICKNAME);
            // CAMBIO: send -> safeSend
            safeSend(client_fd, "Please enter your nickname:\n");
            return;
        }
        else if (client->getStage() == WAITING_NICKNAME) {
            line.erase(0, line.find_first_not_of(" "));
            line.erase(line.find_last_not_of(" ") + 1);
            if (line.empty() || clients_info.nickExists(line)) {
                // CAMBIO: send -> safeSend
                safeSend(client_fd, "Nickname invalid or taken. Try again:\n");
                return;
            }
            client->setNickname(line);
            client->setStage(WAITING_USERNAME);
            // CAMBIO: send -> safeSend
            safeSend(client_fd, "Please enter your real name:\n");
            return;
        }
        else if (client->getStage() == WAITING_USERNAME) {
            line.erase(0, line.find_first_not_of(" "));
            line.erase(line.find_last_not_of(" ") + 1);
            if (line.empty() ) {
                // CAMBIO: send -> safeSend
                safeSend(client_fd, "Empty username. Try again:\n");
                return;
            }
            client->setRealname(line);
            client->setStage(CONNECTED);
            // Mantengo tu mensaje original con fd
            std::cout << "New client " << line << " connected with fd " << client_fd << ".\n";
            // CAMBIO: send -> safeSend
            safeSend(client_fd, "You have successfully authenticated and joined the server.\n");
            return;
        }
        else if (client->getStage() == CONNECTED) {
            // MANTENGO TODO TU FORMATO COLORIDO ORIGINAL
            std::string sender_nick = client->getNickname();
            std::string full_message = "[Message from: " + sender_nick + "]: " + line + "\n";

            std::cout << "--------------------------------------------\n";
            std::cout << "[Message from: \033[1;34m" << sender_nick << "\033[0m]\n";
            std::cout << "--------------------------------------------\n";
            std::cout << "\033[1;32m" << line << "\033[0m\n";
            std::cout << "--------------------------------------------\n";

            handler.handleClientMessage(client_fd, line, *this);
        }
    }

    // Verificación adicional para datos residuales sin \n
    if (client->getBuffer().size() > Client::MAX_BUFFER_SIZE) {
        // Mantengo tu mensaje de error detallado original
        std::cerr << "Client fd " << client_fd << " buffer overflow (" 
                  << client->getBuffer().size() << " > " 
                  << Client::MAX_BUFFER_SIZE << "). Disconnecting." << std::endl;
        removeClient(client_fd);
        return;
    }
}