#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <cstdio>  

// Destructor para limpiar correctamente los recursos.
IRCServer::~IRCServer() {
    close(server_fd);  // Cierra el socket del servidor
    for (size_t i = 0; i < clients.size(); ++i) {
        close(clients[i].fd);  // Cierra todos los sockets de los clientes
    }
    std::cout << "Server shut down." << std::endl;
}

// Constructor que inicializa el servidor y escucha conexiones.
IRCServer::IRCServer(int port, const std::string& password) : port(port), password(password) {
    struct sockaddr_in server_addr;

    // Crear el socket del servidor.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        std::exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor.
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Asignar el socket a la dirección y puerto.
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        std::exit(EXIT_FAILURE);
    }

    // Comenzar a escuchar conexiones entrantes.
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        std::exit(EXIT_FAILURE);
    }

    // Inicializar la estructura pollfd para el servidor.
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);

    std::cout << "IRC Server running on port " << port << std::endl;
}

// Función para aceptar un nuevo cliente.
void IRCServer::acceptClient() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }

    // Mensaje de bienvenida que solicita la contraseña
    const char *request = "Welcome to the IRC Server! Please enter the password:\n";
    send(client_fd, request, strlen(request), 0);

    // Leer la contraseña enviada por el cliente
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }

    // Aseguramos que la cadena esté terminada en NULL
    buffer[bytes_received] = '\0';

    // Convertimos el buffer a una string
    std::string received_password(buffer);

    // Eliminamos los saltos de línea (\n) y espacios al final de la contraseña
    received_password.erase(received_password.find_last_not_of("\r\n") + 1);

    std::cout << "Received password: '" << received_password << "'" << std::endl;  // Para debug

    // Comparar la contraseña proporcionada con la correcta
    if (received_password != password) {
        const char *error_msg = "Incorrect password. Connection closed.\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        close(client_fd);
        return;
    }

    // Añadir el cliente a la lista si la contraseña es correcta
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    clients.push_back(pfd);
    std::cout << "New client connected." << std::endl;
}


// Función para manejar la desconexión de un cliente.
void IRCServer::removeClient(int client_fd) {
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            close(client_fd);
            clients.erase(clients.begin() + i);  // Elimina el cliente de la lista
            std::cout << "Client disconnected." << std::endl;
            break;
        }
    }
}



// Función que maneja el ciclo principal de escucha y procesamiento de datos.
void IRCServer::run() {
    while (true) {
        // Usar poll() para manejar múltiples descriptores de archivo.
        if (poll(&clients[0], clients.size(), -1) < 0) {
            perror("poll");
            continue;  // Si ocurre un error en poll(), intentar de nuevo
        }

        // Revisar todos los clientes para ver si hay actividad.
        for (size_t i = 0; i < clients.size(); ++i) {
            if (clients[i].revents & POLLIN) {  // Si hay datos para leer
                if (clients[i].fd == server_fd) {
                    acceptClient();  // Aceptar un nuevo cliente
                } else {
                    handleClientData(clients[i].fd);  // Procesar los datos del cliente
                }
            }
        }
    }
}

// Función para manejar los datos de un cliente.
void IRCServer::handleClientData(int client_fd) {
    char buffer[512];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        removeClient(client_fd);
    } else {
        buffer[bytes_received] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        if (std::strncmp(buffer, "/quit", 5) == 0) {
            removeClient(client_fd);
        } else {
            send(client_fd, "Message received\n", 17, 0);
        }
    }
}

