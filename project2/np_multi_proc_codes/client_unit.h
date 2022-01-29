#ifndef client_unit_np_multi_proc_h
#define client_unit_np_multi_proc_h
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <deque>
#include"../np_utils/units.h"
#include "global_variables.h"

using namespace std;



class client_unit{
public:
    // no static variables or functions here since it's not practical to use static for different process
    pid_t pid;
    int sockfd;
    int ID; //can't use FD table to record minimal avaliable client ID since there may have file redirection command which requires something like open(file, "w")
    char nickname[MAX_NICKNAME_LENGTH];
    char IP_port[MAX_IP_PORT_LENGTH];
    char direct_message[MAX_MESSAGE_LENGTH]; // tell() and yell() will send some message to this variable
    deque<stream_unit>* one_line_pipes; // each client has his own one_line_pipes table, all the one line command comming from him shares his one_line_pipes table
    vector<pid_t>* user_pipe_send_waiting_process;
};

void initialize_client_alist(){
    key_t shmkey;
    // get unique key of shared memory
    if((shmkey=ftok(".", 'b'))<0){
        perror("ftok failed");
    }
    
    // declare a shared memory
    if((client_alist_id = shmget(shmkey, sizeof(client_unit)*MAX_CLIENTS, IPC_CREAT | 0600))<0){
        perror("shared memory create failed");
    }
    // attach the shared memory to this process' addressing space
    if((client_alist = (client_unit*)shmat(client_alist_id, NULL, 0)) == (void*) -1){
        perror("attach shared memory failed");
    }
    for(int i=0; i<MAX_CLIENTS;i++){
        (client_alist+i)->pid=-1;
        (client_alist+i)->sockfd=-1;
        (client_alist+i)->ID=-1;
        strcpy((client_alist+i)->nickname,"");
        strcpy((client_alist+i)->IP_port,"");
        strcpy((client_alist+i)->direct_message,"");
    }
}


client_unit* get_lowest_avaliable_ID_client(int input_slave_sock_fd, pid_t input_pid, string input_nickname, string input_IP_port){
    int i;
    for(i=0; i<MAX_CLIENTS;i++){
        if((client_alist+i)->ID == -1){
            // catch lowest avaliable ID
            (client_alist+i)->ID = i+1;// client ID's index start from 1 while shared memory's index start from 0
            break;
        }
    }
    (client_alist+i)->sockfd = input_slave_sock_fd;
    (client_alist+i)->pid = input_pid;
    strcpy((client_alist+i)->nickname, input_nickname.c_str());
    strcpy((client_alist+i)->IP_port, input_IP_port.c_str());
    (client_alist+i)->one_line_pipes = new deque<stream_unit>(MAX_N_NUM_PIPE);
    (client_alist+i)->user_pipe_send_waiting_process = new vector<pid_t>;
    return (client_alist+i);
}

bool whether_client_exist(int input_ID){
    for(int i=0; i<MAX_CLIENTS;i++){
        if((client_alist+i)->ID == input_ID)
            return true;
    }
    return false;
}

client_unit* convert_ID_to_client(int input_ID){
    client_unit* corresponding_client=nullptr;
    for(int i=0; i<MAX_CLIENTS;i++){
        if((client_alist+i)->ID == input_ID){
            corresponding_client=(client_alist+i);
            break;
        }
    }
    return corresponding_client;
}

client_unit* convert_sockfd_to_client(int input_sockfd){
    client_unit* corresponding_client=nullptr;
    for(int i=0;i<MAX_CLIENTS; i++){
        if((client_alist+i)->sockfd == input_sockfd){
            corresponding_client = (client_alist+i);
            break;
        }
    }
    return corresponding_client;
}


#endif /* client_unit_np_multi_proc_h */
