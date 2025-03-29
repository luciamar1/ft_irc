#include "Server.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    // Comprobamos que se pasen los 3 argumentos correctamente (programa, puerto y contraseña)
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;  // Salimos con un código de error
    }

    // Convertimos el puerto de string a entero
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {  // Verificamos que el puerto sea válido
        std::cerr << "Invalid port number. Port must be between 1 and 65535." << std::endl;
        return 1;
    }

    // La contraseña se pasa directamente desde el argumento
    std::string password = argv[2];

    // Aseguramos que la contraseña no esté vacía
    if (password.empty()) {
        std::cerr << "Password cannot be empty." << std::endl;
        return 1;
    }

    // Creamos el servidor con el puerto y la contraseña
    IRCServer server(port, password);  // Pasamos el puerto y la contraseña al constructor
    
    server.run();  // Iniciamos el servidor
    return 0;
}
