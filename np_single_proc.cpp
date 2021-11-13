#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include "commands_np_single_proc.h"
#include "npshell_np_single_proc.h"
#define MAX_BUFFER_SIZE 15000
#define MAX_CLIENTS 31
using namespace std;
/* ====================
 This is a single-process concurrent chat-like remote working ststems (rwg), and it also supports all functions in project 1
 Usage example:
     server: $ ./np_single_proc 7086
     client: $ telnet localhost 7086
     check: $ watch -n 2 ss -tnlp | grep np or $ watch -n 2 lsof -i -P -n | grep LISTEN
 Implementation:
     Use pipe to implement user pipe
     Use socket to send messages directory
 ==================== */

int passiveTCP(int port){
    int server_sock_fd;
    struct sockaddr_in server_addr;
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
    
    return server_sock_fd;
}

int main(int argc, char *argv[]){
    cout << "It's np_single_proc, pid=" << getpid() << endl;
    memset(client_unit::user_pipes, -1, sizeof(client_unit::user_pipes)); // initialize user_pipes
    int port=-1, server_sock_fd, nfds;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    fd_set afds, rfds;
    //server_sock_fd = passiveTCP(port=7086);
    server_sock_fd = passiveTCP(port=atoi(argv[1]));
    
    nfds = server_sock_fd+1; // record current maximal file descriptor
    FD_ZERO(&afds);
    FD_SET(server_sock_fd, &afds); // add server_sock_fd into active file descriptor table to handle whether there is new connection
    
    
    while(true){
        // suppose server is always on
        memcpy(&rfds, &afds, sizeof(rfds));
        // wait until someone connect to this server, someone type-in something on their console and press [enter], or other actions that will use at least one file decriptor in rfds
        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0,(struct timeval *)0) < 0)
            perror("server select failed");
        for(int fd=0; fd<nfds; fd++){
            // for master socket(accept new connection) and all clients' slave socket(read/write request)
            if(!FD_ISSET(fd, &rfds)) continue; // until find one file descriptor of server or clients that wants to do some actions
            if(fd == server_sock_fd){
                // socket accept, handle new connection
                int slave_sock_fd;
                client_addr_len = sizeof(client_addr);
                if((slave_sock_fd = accept(server_sock_fd, (struct sockaddr *) &client_addr, &client_addr_len))<0)
                    perror("server socket acception failed, connection broke");
                cout << "client connects from " << inet_ntoa(client_addr.sin_addr) << ", port=" << ntohs(client_addr.sin_port) << endl;
                
                FD_SET(slave_sock_fd, &afds); // add new client's slave socket into afds bit array to further handle his request
                nfds = max(nfds, slave_sock_fd+1); // update nfds
                client_unit new_client(slave_sock_fd, client_unit::get_lowest_avaliable_ID() , "(no name)", (string) inet_ntoa(client_addr.sin_addr) + ":" + to_string(ntohs(client_addr.sin_port)));
                client_unit::client_alist.push_back(new_client); // update client list
                // welcome message
                login(slave_sock_fd, server_sock_fd, nfds, afds);
                ::send(slave_sock_fd, "% ", 2, 0); // print command line prompt to client
            } else{
                // client status change
                char client_message[MAX_BUFFER_SIZE];
                ssize_t client_message_amount = ::recv(fd, client_message, sizeof(client_message), 0);
                bool connection_alive = true;
                
                // handle message from client
                if(client_message_amount > 0){
                    vector<client_unit>::iterator client = client_unit::convert_sockfd_to_client(fd);
                    connection_alive = implement_npshell_once(client, client_message, server_sock_fd, nfds, afds);
                }
                
                // determine client leave or not, if leave than clear client, else print % to him
                if((client_message_amount <= 0) | (!connection_alive)){
                    // client leave eg. client type-in "exit"/"^d" or client's connection is broke
                    if((client_message_amount == 0) || (!connection_alive))
                        cout << "client leave" << endl;
                    else if(client_message_amount < 0)
                        perror("client message transfer failed");
                    logout(fd, server_sock_fd, nfds, afds);
                } else{
                    // print command line prompt to client
                    ::send(fd, "% ", 2, 0);
                }
                
            }
        } // check next element in rfds
    } // server is always on
    return 0;
}
