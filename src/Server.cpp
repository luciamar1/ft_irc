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
// IRCServer::IRCServer(int port, const std::string& password) : port(port), password(password) {
//     struct sockaddr_in server_addr;

//     // Crear el socket
//     server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd < 0) {
//         std::perror("socket");  // Si falla al crear el socket, termina el programa
//         std::exit(EXIT_FAILURE);
//     }

  
//     // Configurar la dirección del servidor
//     std::memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces de red
//     server_addr.sin_port = htons(port);  // Asignar el puerto proporcionado

//     // Asociar el socket con la dirección del servidor
//     if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         std::perror("bind");  // Si falla la asociación, termina el programa
//         std::exit(EXIT_FAILURE);
//     }

//     // Escuchar conexiones entrantes
//     if (listen(server_fd, 10) < 0) {
//         std::perror("listen");  // Si falla al escuchar, termina el programa
//         std::exit(EXIT_FAILURE);
//     }

//     // Preparar el conjunto de clientes con el socket del servidor
//     struct pollfd pfd;
//     pfd.fd = server_fd;
//     pfd.events = POLLIN;  // Indica que estamos esperando eventos de lectura
//     clients.push_back(pfd);  // Añadir el servidor a la lista de clientes

//     std::cout << "IRC Server running on port " << port << " and awaiting connections..." << std::endl;
// }

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

    // CORRECCIÓN: Configurar non-blocking para el socket del servidor
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

// IRCServer::IRCServer(int port, const std::string& password) : port(port), password(password) {
//     struct sockaddr_in server_addr;

//     // Crear el socket
//     server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd < 0) {
//         std::perror("socket");
//         std::exit(EXIT_FAILURE);
//     }

//     // 🔥 Añadir opción SO_REUSEADDR para permitir reusar el puerto inmediatamente
//     int opt = 1;
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) )
//     {
//         std::perror("setsockopt");
//         std::exit(EXIT_FAILURE);
//     }

//     // Configurar la dirección del servidor
//     std::memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//     server_addr.sin_port = htons(port);

//     // Asociar el socket con la dirección del servidor
//     if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         std::perror("bind");
//         std::exit(EXIT_FAILURE);
//     }

//     // Escuchar conexiones entrantes
//     if (listen(server_fd, 10) < 0) {
//         std::perror("listen");
//         std::exit(EXIT_FAILURE);
//     }

//     // Preparar el conjunto de clientes con el socket del servidor
//     struct pollfd pfd;
//     pfd.fd = server_fd;
//     pfd.events = POLLIN;
//     clients.push_back(pfd);

//     std::cout << "IRC Server running on port " << port << " and awaiting connections..." << std::endl;
// }

BookClient& IRCServer::getClientsBook()
{
    return this->clients_info;
}
ChannelBook& IRCServer::getChannelsBook()
{
    return this->channels;
}

// Destructor del servidor
IRCServer::~IRCServer() 
{
    close(server_fd);
    
    // Cerrar todos los sockets de clientes
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd != server_fd) {
            close(clients[i].fd);
        }
    }
    std::cout << "Server has shut down. All client connections have been closed." << std::endl;

}





// Solicitar el nickname

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

        // CORRECCIÓN: Uso directo de F_SETFL con O_NONBLOCK
        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
            std::perror("fcntl");
            close(client_fd);
            continue;
        }

        // Añadir a la lista de poll
        struct pollfd pfd;
        pfd.fd = client_fd;
        pfd.events = POLLIN;
        clients.push_back(pfd);

        // Registrar cliente
        clients_info.addClient(client_fd, "", "", WAITING_PASSWORD);

        const char* welcome_msg = "Welcome to the IRC Server! Please enter the password:\n";
        send(client_fd, welcome_msg, strlen(welcome_msg), 0);
    }
}


// Eliminar cliente

// void IRCServer::removeClient(int client_fd) {
//     Client* client = clients_info.getClient(client_fd);
//     if (client) {
//         // Limpiar de todos los canales
//         const std::set<std::string>& channels = client->getJoinedChannels();
//         for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
//             Channel* channel = channels_book.getChannel(*it);
//             if (channel) {
//                 channel->removeClient(client);
//             }
//         }
//         // ... resto del código ...
//     }
// }
void IRCServer::removeClient(int client_fd) 
{
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



// void IRCServer::handleClientData(int client_fd, CommandHandler &handler) {
//     char buffer[512];
//     memset(buffer, 0, sizeof(buffer));
//     int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

//     if (bytes_received < 0) {
//         if (errno == EAGAIN || errno == EWOULDBLOCK)
//             return;  // No hay datos ahora
//         std::perror("recv handleClientData");
//         removeClient(client_fd);
//         return;
//     }
//     if (bytes_received == 0) {
//         removeClient(client_fd);
//         return;
//     }

//     buffer[bytes_received] = '\0';
//     Client* client = clients_info.getClient(client_fd);
//     if (!client) return;

//     client->getBuffer() += buffer;
//     size_t pos;
//     while ((pos = client->getBuffer().find('\n')) != std::string::npos) {
//         std::string line = client->getBuffer().substr(0, pos);
//         client->getBuffer().erase(0, pos + 1);
//         line.erase(line.find_last_not_of("\r\n") + 1);

//         if (client->getStage() == WAITING_PASSWORD) {
//             if (line != password) {
//                 send(client_fd, "Incorrect password. Connection closed.\n", 40, 0);
//                 removeClient(client_fd);
//                 return;
//             }
//             client->setStage(WAITING_NICKNAME);
//             send(client_fd, "Please enter your nickname:\n", 29, 0);
//             return;
//         }

//         if (client->getStage() == WAITING_NICKNAME) {
//             line.erase(0, line.find_first_not_of(" "));
//             line.erase(line.find_last_not_of(" ") + 1);
//             if (line.empty() || clients_info.nickExists(line)) {
//                 send(client_fd, "Nickname invalid or taken. Try again:\n", 38, 0);
//                 return;
//             }
//             client->setNickname(line);
//             client->setStage(WAITING_USERNAME);
//             send(client_fd, "Please enter your real name:\n", 29, 0);
//             return;
//         }

//         if (client->getStage() == WAITING_USERNAME)
//         {
            // line.erase(0, line.find_first_not_of(" "));
            // line.erase(line.find_last_not_of(" ") + 1);
            // if (line.empty() ) {
            //     send(client_fd, "Empty username. Try again:\n", 38, 0);
            //     return;
            // }
            // client->setRealname(line);
            // client->setStage(CONNECTED);
            // std::cout << "New client " << line << " connected with fd " << client_fd << ".\n";
            // send(client_fd, "You have successfully authenticated and joined the server.\n", 60, 0);
            // return;
//         }



//         if (client->getStage() == CONNECTED) {
            // std::string sender_nick = client->getNickname();
            // std::string full_message = "[Message from: " + sender_nick + "]: " + line + "\n";

            // std::cout << "--------------------------------------------\n";
            // std::cout << "[Message from: \033[1;34m" << sender_nick << "\033[0m]\n";
            // std::cout << "--------------------------------------------\n";
            // std::cout << "\033[1;32m" << line << "\033[0m\n";
            // std::cout << "--------------------------------------------\n";

            // handler.handleClientMessage(client_fd, line, *this);
//             //handler.sendToAllClients(full_message, client_fd, *this);
//         }
//     }
// }
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

        // ... [todo el código existente de procesamiento de estados] ...
        if (client->getStage() == WAITING_PASSWORD) {
            // ... [código existente] ...
            if (line != password) {
                send(client_fd, "Incorrect password. Connection closed.\n", 40, 0);
                removeClient(client_fd);
                return;
            }
            client->setStage(WAITING_NICKNAME);
            send(client_fd, "Please enter your nickname:\n", 29, 0);
            return;
        }
        else if (client->getStage() == WAITING_NICKNAME) {
            // ... [código existente] ...
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
        else if (client->getStage() == WAITING_USERNAME) {
            // ... [código existente] ...
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
        else if (client->getStage() == CONNECTED) {
            // ... [código existente] ...
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
        std::cerr << "Client fd " << client_fd << " buffer overflow (" 
                  << client->getBuffer().size() << " > " 
                  << Client::MAX_BUFFER_SIZE << "). Disconnecting." << std::endl;
        removeClient(client_fd);
        return;
    }
}