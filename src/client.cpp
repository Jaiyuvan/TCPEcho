#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include<string>
#include<poll.h>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
bool running=true;
void sendToServer(int serverConnection,pollfd &stdin_pollfd){
    std::string message_str={};
    if(std::getline(std::cin,message_str)){
        if(!message_str.empty()){
            message_str=message_str+'\n';
            const char*message=message_str.c_str();
            if (send(serverConnection, message, strlen(message), 0) < 0) {
            std::cout << "Data sending failed\n";
            }
        }
    }
    else{
        std::cout<<"Exiting from server"<<"\n";
        stdin_pollfd.fd=-1;
        shutdown(serverConnection,SHUT_WR);
    }    
}
void receiveFromServer(int serverConnection){
    char buffer[BUFFER_SIZE];
    ssize_t data_recv = recv(serverConnection, buffer, BUFFER_SIZE - 1, 0);
    
    if (data_recv > 0) {
        buffer[data_recv] = '\0';
        std::cout << "Received from server: " << buffer;
    } 
    else if (data_recv == 0) {
        std::cout << "Server disconnected\n";
        running=false;
    } 
    else {
        std::cout << "Recv ERROR\n";
        running=false;
    }

}
int main() {


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

    if (connect(serverConnection, (struct sockaddr*)&severAddress, sizeof(severAddress)) < 0) {
        std::cout << "Server connection failed\n";
        close(serverConnection);
        return 1;
    }
    
    struct pollfd fds[2];
    fds[0].fd=STDIN_FILENO;
    fds[0].events=POLLIN;

    fds[1].fd=serverConnection;
    fds[1].events = POLLIN | POLLHUP | POLLERR;


    while (running){
        int poll_count = poll(fds, 2, -1);
        if (poll_count < 0) {
            std::cout << "Poll error\n";
            break;
        }        
        if(fds[1].revents &(POLLIN | POLLHUP | POLLERR)){
            receiveFromServer(serverConnection);
        }    
        if (fds[0].fd!= -1 && (fds[0].revents & POLLIN)){
            sendToServer(serverConnection,fds[0]);
        }

    }

    close(serverConnection);
    return 0;
}
