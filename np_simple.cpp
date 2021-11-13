#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include"npshell_np_simple.h"
#define MAX_CLIENTS 2
using namespace std;
/* ====================
 This is a concurrent connection-oriented server which allows one client connect to it, and it also supports all functions in project 1
 Usage example:
     server: $ ./np_simple 7086
     client: $ telnet localhost 7086
     check: $ watch -n 2 ss -tnlp | grep np or $ watch -n 2 lsof -i -P -n | grep LISTEN
 ==================== */
int main(int argc, char *argv[]) {
    cout << "It's np_simple, pid="<< getpid() << endl;
    
    int port, server_sock_fd, slave_sock_fd;
    struct sockaddr_in client_addr, server_addr;
    socklen_t client_addr_len;
    pid_t cpid;
    //port = atoi(argv[1]);
    port = 7086;
    
    // create socket
    if ((server_sock_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("server socket creation failed\n");
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((u_short)port);
    int one = 1;
    setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    // bind socket
    if (::bind(server_sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        perror("server socket binding failed\n");
    
    // socket listen
    if(::listen(server_sock_fd, MAX_CLIENTS) <0)
        perror("server socket listening failed");
    cout << "server is listening on port " << port << endl;
    
    // socket accept
    while(true){
        // for each single client
        client_addr_len = sizeof(client_addr);
        if((slave_sock_fd = accept(server_sock_fd, (struct sockaddr *) &client_addr, &client_addr_len))<0)
            perror("server socket acception failed, connection broke");
        
        cout << "client connects from " << inet_ntoa(client_addr.sin_addr) << ", port=" << ntohs(client_addr.sin_port) << endl;
        
        // fork
        if((cpid = fork()) < 0)
            perror("fork error");
        if(cpid == 0){
            // child process
            close(server_sock_fd);
            dup2(slave_sock_fd, STDIN_FILENO);
            dup2(slave_sock_fd, STDOUT_FILENO);
            dup2(slave_sock_fd, STDERR_FILENO);
            npshell();
            close(slave_sock_fd);
            exit(0);
        }
        else{
            // parent process
            close(slave_sock_fd);
            wait(nullptr);
            cout << "client leave" << endl;
        }
        
    }
    
    return 0;
}
