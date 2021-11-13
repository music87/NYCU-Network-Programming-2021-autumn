#ifndef npshell_np_single_proc_h
#define npshell_np_single_proc_h
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <signal.h>
#include "parsing_line_np_single_proc.h"
#include "client_units.h"
using namespace std;

bool implement_npshell_once(vector<client_unit>::iterator client, string line, int server_sock_fd, int input_nfds, fd_set input_afds){
    //freopen("./1.txt", "r", stdin); // current working path will depend on which path server(not client) implements in
    
    bool connection=true;
    vector<cmd_unit> cmd_group;
    

    // initialize environment variable
    vector<pair<string,string>> environment_variables = client->get_environment_variables();
    for(vector<pair<string,string>>::iterator it=environment_variables.begin(); it<environment_variables.end(); it++)
        setenv(it->first.c_str(), it->second.c_str(), true);
    
    // reserve server console's stdin/stdout/stderr
    int server_stdfd[3];
    server_stdfd[0] = dup(STDIN_FILENO);
    server_stdfd[1] = dup(STDOUT_FILENO);
    server_stdfd[2] = dup(STDERR_FILENO);
    
    // redirect server console's output/input to client's console's output/input
    dup2(client->get_sockfd(), STDIN_FILENO);
    dup2(client->get_sockfd(), STDOUT_FILENO);
    dup2(client->get_sockfd(), STDERR_FILENO);
    
        stringstream ss; ss << line; // to get rid of garbage input like \xbd\xdf\xff\U0000007f
        getline(ss, line, '\n'); // read one line command
        cmd_group = parsing(line); // parse command
        if(line=="exit" || cin.eof()){
            connection = false; // terminate parent process
        } else if(line.empty()){
            // when encounter '\n', re-input the one line command again
        } else if(line.find("setenv")!=string::npos){
            set_client_env(client, cmd_group);
        } else if(line.find("printenv")!=string::npos){
            get_client_env(cmd_group);
        } else if(line.find("tell")!=string::npos){
            tell(client, cmd_group);
        } else if(line.find("yell")!=string::npos){
            yell(client, cmd_group, server_sock_fd, input_nfds, input_afds);
        } else if(line.find("who")!=string::npos){
            who(client->get_ID());
        } else if(line.find("name")!=string::npos){
            name(client, cmd_group, server_sock_fd, input_nfds, input_afds);
        } else{
            Exec_command execcmd(client, cmd_group, server_sock_fd, input_nfds, input_afds, line);
            execcmd.execute();
        }
    
    
    
    
    
    // clear environment variables
    for(vector<pair<string,string>>::iterator it=environment_variables.begin(); it<environment_variables.end(); it++)
        unsetenv(it->first.c_str());
    
    // return server console's stdout/stderr
    dup2(server_stdfd[0], STDIN_FILENO);
    close(server_stdfd[0]);
    dup2(server_stdfd[1], STDOUT_FILENO);
    close(server_stdfd[1]);
    dup2(server_stdfd[2], STDERR_FILENO);
    close(server_stdfd[2]);
    
    return connection;
}


#endif /* npshell_np_single_proc_h */
