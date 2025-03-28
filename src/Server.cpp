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

// Destructor del servidor
IRCServer::~IRCServer() {
    close(server_fd);  // Cerrar el socket del servidor
    // Cerrar todos los sockets de clientes
    for (size_t i = 0; i < clients.size(); ++i) {
        close(clients[i].fd);
    }
    std::cout << "Server has shut down. All client connections have been closed." << std::endl;
}

// Aceptar un nuevo cliente
void IRCServer::acceptClient() 
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) 
    {
        std::perror("accept");  // Si falla al aceptar la conexión, retorna sin hacer nada
        return;
    }

    // Enviar mensaje de bienvenida y solicitar la contraseña
    const char *request = "Welcome to the IRC Server! Please enter the password:\n";
    send(client_fd, request, strlen(request), 0);

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_fd);  // Si no se recibe información, cerrar la conexión
        return;
    }

    buffer[bytes_received] = '\0';
    std::string received_password(buffer);
    received_password.erase(received_password.find_last_not_of("\r\n") + 1);  // Eliminar saltos de línea al final

    // Verificar la contraseña
    if (received_password != password) {
        const char *error_msg = "Incorrect password. Connection closed.\n";
        send(client_fd, error_msg, strlen(error_msg), 0);  // Enviar error si la contraseña es incorrecta
        close(client_fd);  // Cerrar la conexión del cliente
        std::cout << "Connection attempt failed. Incorrect password entered." << std::endl;
        return;
    }

    
    
    // Si la contraseña es correcta, agregar el cliente a la lista
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);

    clients_info[client_fd] = ""; // Inicializar el nickname del cliente como vacío

    const char *request2 = "Perfect! now enter your nickname:\n";
    send(client_fd, request2, strlen(request2), 0);

    char buffer2[256];
    memset(buffer2, 0, sizeof(buffer2));
    int bytes_received2 = recv(client_fd, buffer2, sizeof(buffer2) - 1, 0);
    if (bytes_received2 <= 0) {
        close(client_fd);  // Si no se recibe información, cerrar la conexión
        return;
    }
    std::string _nick(buffer2);
    _nick.erase(_nick.find_last_not_of("\r\n") + 1); // Eliminar saltos de línea al final
    _nick.erase(0, _nick.find_first_not_of(" ")); // Eliminar espacios al principio
    _nick.erase(_nick.find_last_not_of(" ") + 1); 
    handleNickCommand(client_fd, _nick); 
    std::cout << "New client connected with file descriptor " << client_fd << "." << std::endl;
}

// Eliminar un cliente
void IRCServer::removeClient(int client_fd) 
{
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            close(client_fd);  // Cerrar la conexión del cliente
            clients.erase(clients.begin() + i);  // Eliminar el cliente de la lista
            clients_info.erase(client_fd);  // Eliminar el nickname del cliente
            std::cout << "Client with file descriptor " << client_fd << " has disconnected." << std::endl;
            break;
        }
    }
}

// Ejecutar el servidor y manejar los eventos
void IRCServer::run() 
{
    while (true) {
        // Esperar eventos de entrada de los clientes
        if (poll(&clients[0], clients.size(), -1) < 0) {
            std::perror("poll");  // Si hay error en poll, continuamos
            continue;
        }

        // Manejar los eventos para cada cliente
        for (size_t i = 0; i < clients.size(); ++i) {
            if (clients[i].revents & POLLIN) {
                if (clients[i].fd == server_fd) {
                    acceptClient();  // Aceptar un nuevo cliente
                } else {
                    handleClientData(clients[i].fd);  // Manejar los datos del cliente
                }
            }
        }
    }
}

// Manejar el comando NICK (cambiar el nickname del cliente)
void IRCServer::handleNickCommand(int client_fd, const std::string& new_nick) 
{
    std::string trimmed_nick = new_nick;
    trimmed_nick.erase(trimmed_nick.find_last_not_of("\r\n") + 1);  // Eliminar saltos de línea

    if (trimmed_nick.empty()) {
        std::string error_msg = "ERROR: Nickname cannot be empty.\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
        return;
    }

    // Verificar que el nickname no esté en uso
    for (std::map<int, std::string>::iterator it = clients_info.begin(); it != clients_info.end(); ++it) {
        if (it->second == trimmed_nick) {
            std::string error_msg = "ERROR: Nickname already in use.\n";
            send(client_fd, error_msg.c_str(), error_msg.length(), 0);
            return;
        }
    }

    // Establecer el nuevo nickname
    clients_info[client_fd] = trimmed_nick;
    std::string success_msg = "Nickname successfully set to: " + trimmed_nick + "\n";
    send(client_fd, success_msg.c_str(), success_msg.length(), 0);
    std::cout << "Client with fd " << client_fd << " has changed their nickname to: " << trimmed_nick << "." << std::endl;
}

// Manejar los mensajes enviados por los clientes
void IRCServer::handleClientMessage(int client_fd, const std::string& message) 
{
    std::istringstream iss(message);
    std::string command, params;

    iss >> command;
    std::getline(iss, params);

    if (!params.empty() && params[0] == ' ') {
        params = params.substr(1);  // Eliminar espacios extra al inicio de los parámetros
    }

    // Manejo de comandos específicos
    if (command == "NICK") {
        handleNickCommand(client_fd, params);  // Cambiar el nickname
    } else if (command == "QUIT") {
        removeClient(client_fd);  // Desconectar al cliente
    } 
    // else {
    //     std::string error_msg = "ERROR: Unknown command\n";
    //     send(client_fd, error_msg.c_str(), error_msg.length(), 0);  // Comando desconocido
    //     std::cout << "Unknown command received from client " << client_fd << ": " << command << std::endl;
    // }
}

void IRCServer::sendToAllClients(const std::string& message, int sender_fd) 
{
    for (std::map<int, std::string>::const_iterator it = clients_info.begin(); it != clients_info.end(); ++it) {
        if (it->first != sender_fd) {
            send(it->first, message.c_str(), message.size(), 0);
        }
    }
}



void IRCServer::handleClientData(int client_fd)
{
    char buffer[512];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        removeClient(client_fd);  // Si no se recibe datos, desconectar al cliente
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);

    // Eliminar saltos de línea y espacios extra
    message.erase(message.find_last_not_of("\r\n") + 1);

    // Obtener el nickname del cliente o usar el fd si no tiene nick
    std::string sender_nick;
    if (clients_info.find(client_fd) != clients_info.end() && !clients_info[client_fd].empty()) {
        sender_nick = clients_info[client_fd];  // Usar el nickname si está disponible
    } else {
        // Si no tiene nickname, usar un mensaje claro con el fd
        std::stringstream ss;
        ss << client_fd;
        sender_nick = "No nickname assigned (fd: " + ss.str() + ")";
    }

    // Formatear el mensaje con el nombre del cliente, agregar salto de línea
    std::string full_message = "[Message from: \033[1;34m" + sender_nick + " \033[0m]: " + message + "\n";

    // Mostrar el mensaje en consola para el servidor con color (solo servidor)
    std::cout << "--------------------------------------------\n";
    std::cout << "[Message from: \033[1;34m" << sender_nick << "\033[0m]\n";  // Nombre en azul o fd
    std::cout << "--------------------------------------------\n";
    std::cout << "\033[1;32m" << message << "\033[0m\n";  // Mensaje en verde
    std::cout << "--------------------------------------------\n";

    // Procesar comandos y mensajes
    handleClientMessage(client_fd, message);

    // Enviar el mensaje completo con el nombre del cliente a todos los demás clientes (sin códigos de color)
    sendToAllClients(full_message, client_fd);
}
