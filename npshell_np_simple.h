#ifndef npshell_h
#define npshell_h
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<csignal>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<unistd.h>
#include<cstring>
#include<signal.h>
#include"parsing_line_np_simple.h"
#include"commands_np_simple.h"
using namespace std;

void npshell(){
    //freopen("./1.txt", "r", stdin); // current working path will depend on which path server(not client) implements in
    string line;
    vector<cmd_unit> cmd_group;
    Command *ptr(nullptr);
    setenv("PATH", "bin:.", true); // initialize environment variable PATH
    while(true){
        cout << "% "; // print command line prompt
        getline(cin, line, '\n'); // read one line command
        cmd_group = parsing(line); // parse command
        if(line=="exit" || cin.eof()){
            exit(0); // terminate parent process
        } else if(line.empty()){
            continue; // when encounter '\n', re-input the one line command again
        } else if(line.find("setenv")!=string::npos){
            ptr = new Builtin_setenv(cmd_group);
        } else if(line.find("printenv")!=string::npos){
            ptr = new Builtin_getenv(cmd_group);
        } else{
            ptr = new Exec_command(cmd_group);
        }
        ptr -> execute();
    }
}
#endif /* npshell_h */
