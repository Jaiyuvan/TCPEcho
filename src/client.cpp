#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

int main() {
    char buffer[BUFFER_SIZE];
    const char* message = "Hello from client";

    int serverConnection = socket(AF_INET, SOCK_STREAM, 0);
    if (serverConnection < 0) {
        std::cerr << "Socket creation error\n";
        return 1;
    }

    struct sockaddr_in severAddress{};
    severAddress.sin_family = AF_INET;
    severAddress.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &severAddress.sin_addr) <= 0) {
        std::cout << "Invalid Address Error\n";
        close(serverConnection);
        return 1;
    }

    // FIX 1: connect returns 0 on success, < 0 on error
    if (connect(serverConnection, (struct sockaddr*)&severAddress, sizeof(severAddress)) < 0) {
        std::cout << "Server connection failed\n";
        close(serverConnection);
        return 1;
    }

    if (send(serverConnection, message, strlen(message), 0) < 0) {
        std::cout << "Data sending failed\n";
    }

    ssize_t data_recv = recv(serverConnection, buffer, BUFFER_SIZE - 1, 0);
    if (data_recv > 0) {
        buffer[data_recv] = '\0';
        std::cout << "Received from server: " << buffer << "\n";
    } 
    else if (data_recv == 0) {
        std::cout << "Server disconnected\n";
    } 
    else {
        std::cout << "Recv ERROR\n";
    }

    close(serverConnection);
    return 0;
}