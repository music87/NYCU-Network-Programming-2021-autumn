#include <iostream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <filesystem>
#include "npshell.h"
#include "client_unit.h"
#include "commands.h"
#include "global_variables.h"
#include "../np_utils/passiveTCP.h"
#define MAX_BUFFER_SIZE 15000
#define MAX_CLIENTS 31
using namespace std;
/* ====================
 This is a concurrent connection-oriented chat-like remote working ststems (rwg) with shared memory and FIFO, and it also supports all functions in project 1
 Usage example:
     server: $ ./np_multi_proc 7086
     client: $ telnet localhost 7086
     check1: $ watch -n 2 ss -tnlp | grep np or $ watch -n 2 lsof -i -P -n | grep LISTEN
     check2: $ while :; do clear; ipcs -m; sleep 2; done
 Implementation:
     Use FIFO to implement user pipe
     Use shared memory to save clients infos and messages to do something like broadcast
 ==================== */

// declare global variables
int client_alist_id;
client_unit *client_alist;
client_unit *client;

void SIGCHLD_handler(int signal){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void SIGINT_handler(int signal){
    // server close and remove the shared memory. if the shared memory removing failed, then type-in $ echo 'ipcs -mc | grep $( whoami ) | awk "{print $1}" | xargs -rn1 ipcrm -m' | zsh -s
    cout << "server handle SIGINT" << endl;
    // detach the shared memory from this process' addressing space
    if(shmdt(client_alist)<0){
        perror("detach client_alist shared memory failed");
        exit(EXIT_FAILURE);
    }
    
    // remove shared memory
    if(shmctl(client_alist_id, IPC_RMID, 0) <0){
        perror("remove client_alist shared memory failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void SIGUSR1_handler(int signal){
    // handle message from tell or broadcast
    cout << client->direct_message;
    strcpy(client->direct_message, "");
}


void create_empty_directory(string dirpath){
    // create an empty directory for user pipe, if directory exist, then first delete it
    struct stat info;
    if (stat(dirpath.c_str(), &info) == 0 && info.st_mode & S_IFDIR){
        // remove all the contents in the directory
        //for (const auto& entry : std::filesystem::directory_iterator(dirpath))
        //    std::filesystem::remove_all(entry.path());
        // remove the directory
        if(rmdir(dirpath.c_str()))
            perror("delete directory failed");
    }
    // create the directory
    if(mkdir(dirpath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR))
        perror("directory create failed");
}

int main(int argc, char *argv[]){
    cout << "It's np_multi_proc, pid=" << getpid() << endl;
    int port=-1, server_sock_fd, slave_sock_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pid_t cpid;
    string dirpath = "./user_pipe/";
    
    // register SIGCHLD signal for slave process
    signal(SIGCHLD, SIGCHLD_handler);
    // register SIGINT signal for close server and clear the shared memory
    signal(SIGINT, SIGINT_handler);
    // register self-defined signal SIGUSR1 for communication between slave processes
    signal(SIGUSR1, SIGUSR1_handler);
    
    // create a blank directory for user pipe
    // create_empty_directory(dirpath);
    
    // declare, attach, and initialize a shared memory for client_alist
    initialize_client_alist();
    
    // create, bind, listen and return the server socket
    //server_sock_fd = passiveTCP(port=7086);
    server_sock_fd = passiveTCP(port=atoi(argv[1]));
    
    // suppose server is always on
    while(true){
        // socket accept, handle new connection
        if((slave_sock_fd = accept(server_sock_fd, (struct sockaddr *) &client_addr, &client_addr_len))<0){
            perror("server socket acception failed, connection broke");
            exit(EXIT_FAILURE);
        }
        cout << "client connects from " << inet_ntoa(client_addr.sin_addr) << ", port=" << ntohs(client_addr.sin_port) << endl;
        
        if((cpid = fork()) < 0){
            perror("server fork error");
            exit(EXIT_FAILURE);
        }
        
        if(cpid == 0){
            // child process
            close(server_sock_fd);
            dup2(slave_sock_fd, STDIN_FILENO);
            dup2(slave_sock_fd, STDOUT_FILENO);
            dup2(slave_sock_fd, STDERR_FILENO);
            
            client = get_lowest_avaliable_ID_client(slave_sock_fd, getpid(), "(no name)", (string) inet_ntoa(client_addr.sin_addr)+":"+to_string(ntohs(client_addr.sin_port)));

            login();
            npshell(dirpath);
            
            // detach the shared memory from this process' addressing space
            if(shmdt(client_alist)<0){
                perror("detach client_alist shared memory failed");
                exit(EXIT_FAILURE);
            }

            close(slave_sock_fd);
            exit(EXIT_SUCCESS);
        } else{
            // parent process
            close(slave_sock_fd);
        }
    }
    return 0;
}

