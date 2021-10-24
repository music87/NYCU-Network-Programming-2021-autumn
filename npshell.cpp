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
#include"parsing_line.h"
#include"commands.cpp"
using namespace std;
// fork
// pipe
// dup, dup2
// close
// exec*
// wait, waitpid
// signal
// setenv
// getenv

int main(){
    // freopen("./1.txt", "r", stdin);
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
            //struct stat buffer;
            //cout << ("./bin/" + cmd_group.front().get_c()).c_str() << endl;
            //cout << (stat(("./bin/" + cmd_group.front().get_c()).c_str(), &buffer) == 0) << endl;
            // (./bin/)xxx
            // unknown command
            // ordinary pipe
            // numbered pipe
            // file redirection
        }
        
        ptr -> execute();
        //convertToASCII(line);
    }
    return 0;
}
