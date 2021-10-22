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
    Built_In_Command btcmd;
    setenv("PATH", "bin:.", true); // initialize environment variable PATH
    cout << "pid=" << getpid() << endl;
    cout << "PATH=" << getenv("PATH") << endl;
    
    while(true){
        cout << "% "; // print command line prompt
        getline(cin, line, '\n'); // read command
        cmd_group = parsing(line);
	if(line=="exit" || cin.eof()){
            exit(0); // terminate parent process
        } else if(line.find("setenv")!=string::npos){
            btcmd.senv(cmd_group); 
	} else if(line.find("printenv")!=string::npos){
            btcmd.genv(cmd_group);
	} else{
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
