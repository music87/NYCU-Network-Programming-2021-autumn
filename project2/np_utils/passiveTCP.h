#ifndef passiveTCP_h
#define passiveTCP_h
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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

#endif /* passiveTCP_h */
