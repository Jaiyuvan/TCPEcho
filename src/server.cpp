#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include<fcntl.h>
#include<netinet/in.h>
#include<poll.h>
#include<vector>
#include<csignal>
#include<cstring>

constexpr int port=8080;
constexpr int bufferSize=1024;
volatile sig_atomic_t keepRun=1;
void signalHandler(int sig){
    if (sig==SIGINT || sig==SIGTERM){
    keepRun=0;
    }
}

void nonBlocking(int fd){
    int flags=fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flags | O_NONBLOCK);
}
int main(){

    struct sigaction sa;
    std::memset(&sa,0,sizeof(sa));
    sa.sa_handler=signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    if (sigaction(SIGINT,&sa,nullptr)<0 || sigaction(SIGTERM,&sa,nullptr)<0){
        std::cout<<"Error in sigaction\n";
    }

    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    if (server_fd==-1){
        perror("Server socket creation failed");
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(port);
    serverAddr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_fd,(struct sockaddr*)&serverAddr,sizeof(sockaddr))<0){
        perror("Server Socket Binding failed");
    }

    if(listen(server_fd,10)<0){
        perror("Listening failed");
        close(server_fd);
        return 1;
    }
    
    std::vector<pollfd> fds;
    fds.push_back({server_fd,POLLIN,0});


    while (keepRun){
        int poll_count=poll(fds.data(),fds.size(),-1);
        if (poll_count<0){
            if (errno==EINTR){
                if (!keepRun) break;
                continue;
            }
            std::cerr<<"Poll error";
            break;
        }

        size_t current_size=fds.size();

        for(size_t i=0;i<current_size;i++){
            if (!(fds[i].revents & POLLIN)){
                continue;
            }
            if (fds[i].fd==server_fd){
                sockaddr_in cli_addr{};
                socklen_t cli_soc_len=sizeof(cli_addr);
                int client_fd=accept(server_fd,(sockaddr*)&cli_addr,&cli_soc_len);

                if (client_fd>=0){
                    nonBlocking(client_fd);
                    fds.push_back({client_fd, POLLIN, 0});
                    std::cout << "New client connected: fd " << client_fd << "\n";
                }
            }
            else{
                char buffer[bufferSize];
                ssize_t bytes_read=recv(fds[i].fd,buffer,bufferSize,0);
                if (bytes_read<=0){
                    std::cout << "Client disconnected: fd " << fds[i].fd << "\n";
                    close(fds[i].fd);
                    fds[i] = fds.back();
                    fds.pop_back();
                    --i;
                    --current_size;
                }
                else{
                    send(fds[i].fd,buffer,bytes_read,0);

                }
            }
        }
    }
    for (const auto& pfd:fds){
        close(pfd.fd);
    }
}
