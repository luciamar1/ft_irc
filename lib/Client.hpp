#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client {
private:
    int fd;  // File descriptor del cliente

public:
    Client(int fd);   // Constructor
    ~Client();        // Destructor (agregado por seguridad)

    int getFd() const;  // Declaración correcta
};

#endif
