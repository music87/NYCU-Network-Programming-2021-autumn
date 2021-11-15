#ifndef npshell_np_multi_proc_h
#define npshell_np_multi_proc_h
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
#include "../np_utils/parsing_line_proc.h"
#include "client_unit.h"
#include "commands.h"
#include "global_variables.h"
using namespace std;

void npshell(string dirpath){
    //freopen("./1.txt", "r", stdin); // current working path will depend on which path server(not client) implements in
    string line;
    vector<cmd_unit> cmd_group;
    setenv("PATH", "bin:.", true);
    

    

    // initialize environment variable
    while(true){
        cout << "% "; // print command line prompt
        getline(cin, line, '\n'); // read one line command
        cmd_group = parsing(line); // parse command
        if(line=="exit" || cin.eof()){
            logout(dirpath);
            exit(0); // terminate the slave process which serves to a specific client
        } else if(line.empty()){
            continue; // when encounter '\n', re-input the one line command again
        } else if(line.find("setenv")!=string::npos){
            set_client_env(cmd_group);
        } else if(line.find("printenv")!=string::npos){
            get_client_env(cmd_group);
        } else if(line.find("tell")!=string::npos){
            tell(cmd_group);
        } else if(line.find("yell")!=string::npos){
            yell(cmd_group);
        } else if(line.find("who")!=string::npos){
            who(client->ID);
        } else if(line.find("name")!=string::npos){
            name(cmd_group);
        } else{
            Exec_command execcmd(cmd_group, line, dirpath);
            execcmd.execute();
        }
    }
}



#endif /* npshell_np_multi_proc_h */
