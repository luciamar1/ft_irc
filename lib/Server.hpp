#ifndef IRC_SERVER_HPP
#define IRC_SERVER_HPP

#include <string>
#include <vector>
#include <poll.h>
#include "CommandHandler.hpp"
#include "BookClient.hpp"  // Incluimos la clase Book
#include "ChannelBook.hpp"

class CommandHandler;
// Definición de la clase IRCServer
class IRCServer {
    private:
        // Atributos del servidor
        int server_fd;  // Descriptor de archivo para el socket del servidor
        int port;       // Puerto en el que el servidor escucha
        std::string password;  // Contraseña del servidor
        std::vector<struct pollfd> clients;  // Lista de clientes conectados
        BookClient clients_info;  // Usamos Book para almacenar la información de los clientes
        ChannelBook channels;
    
public:
    // Constructor y Destructor
    IRCServer(int port, const std::string& password);
    ~IRCServer();

    // Métodos para manejar clientes y mensajes
    void acceptClient();
    void removeClient(int client_fd);
    void handleClientData(int client_fd, CommandHandler& handler);
    void run();

    // Métodos de interacción con clientes
    int nickExist(std::string _nick);

    BookClient&  getClientsBook();
    ChannelBook&  getChannelsBook();


    // Métodos para obtener y configurar información de clientes

};

#endif  // IRC_SERVER_HPP
