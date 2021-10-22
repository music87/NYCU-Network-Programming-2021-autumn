#include<iostream>
#include<cstdlib>
#include<csignal>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<unistd.h>
#include<cstring>
#include<signal.h>
#include"parsing_line.h"
#include"builtin_commands.h"
#include"exec_commands.h"
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
    string line;
    vector<cmd_unit> cmd_group;
    setenv("PATH", "bin:.", true); // initialize environment variable PATH
    cout << "pid=" << getpid() << endl;
    cout << "PATH=" << getenv("PATH") << endl;
    
    while(true){
        cout << "% "; // print command line prompt
        getline(cin, line, '\n'); // read command
        cmd_group = parsing(line);
        Built_In_Command btcmd(cmd_group);
        Exec_Command excmd(cmd_group);
	if(line=="exit" || cin.eof()){
            exit(0); // terminate parent process
        } else if(line.find("setenv")!=string::npos){
            btcmd.senv(); 
	} else if(line.find("printenv")!=string::npos){
            btcmd.genv();
	} else{

            excmd.execution();
            //struct stat buffer;
            //cout << ("./bin/" + cmd_group.front().get_c()).c_str() << endl; 
            //cout << (stat(("./bin/" + cmd_group.front().get_c()).c_str(), &buffer) == 0) << endl;
          // (./bin/)xxx
          // unknown command
          // ordinary pipe
          // numbered pipe
          // file redirection
        }
        //convertToASCII(line);    
    }
    return 0;
}
