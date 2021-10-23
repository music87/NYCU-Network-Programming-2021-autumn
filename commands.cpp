#include "commands.h"
void Command::execution(){
    cerr << "ERROR: class Command's abstract funcition execution override" << endl;
    throw exception();
}

void Builtin_setenv::execution(){
    if(cmd_group.front().get_argv().size() != (3+1)){
        // invalid amount of arguments
        cerr << "ERROR: setenv requires exactly two arguments" << endl;
    } else{
        // add environment variable if it doesn't exist; overwrite it if it does exits
        setenv(cmd_group.front().get_argv().at(1), cmd_group.front().get_argv().at(2), true);
    }
}

void Builtin_getenv::execution(){
    char *re;
    if(cmd_group.front().get_argv().size() != (2+1)){
        // invalid amount of arguments for printenv, argv's last item is NULL
        cerr << "ERROR: printenv requires exactly one argument" << endl;
    } else if((re=getenv(cmd_group.front().get_argv().at(1))) == NULL){
        // environment variable(cmd_group[1]) does not exist in the environment
        cout << ""; // print nothing
    } else{
        // environment variable exists, print its value
        cout << re << endl;
    }
}


void Exec_command::set_streams(){
    // set every process' input stream, output stream, and error stream
    // cout << cmd_group.at(0).get_c() << endl;
    // set numbered pipe stream
    // set ordinary pipe stream
}

void Exec_command::SIGCHLD_handler(int input_signal){
    
    int status;
    // waitpid: parameter -1 means waiting for any child process, parameter WHOHANG returns immediately if no child has exited, waitpid function returns positive number which means the pid of successfully changed status' child process, the waitpid loop will continue until the last child process exits, after then waitpid will return -1(means no child process exist)
    while (waitpid(-1, &status, WNOHANG) > 0);
    // perror(""); // expect to print "no child process"
    // write(STDOUT_FILENO, "% ", 2); // print command line prompt (better at starting, '\n', and after handling SIHCHLD signal)
}

pid_t Exec_command::fork_exec(cmd_unit cmd, int n_ord_pipe){
    pid_t cpid;
    while((cpid = fork())<0){
        // if fork error, than fork again
    };
    if (cpid == 0){
        // set redirection stream, file_fd is got from open() function
        // input stream
        if(cmd.get_readfd() != STDIN_FILENO){
            if(dup2(cmd.get_readfd(), STDIN_FILENO)<0){
                // int dup2(int oldfd, int newfd) makes newfd be the copy of oldfd, closing newfd first if necessary
                // here break the connection between STDIN_FILENO and console, and then reconnect STDIN_FILENO to the place which pointed by cmd.readfd
                // if dup error, than dup will return -1
                perror("ERROR: readfd dup failed, ");
                throw exception();
            }
            // because the place which pointed by cmd.readfd has also been pointed by this process' STDIN_FILENO
            close(cmd.get_readfd());
        }
        
        // output stream
        if(cmd.get_writefd() != STDOUT_FILENO){
            if(dup2(cmd.get_writefd(), STDOUT_FILENO)<0){
                perror("ERROR: writefd dup failed, ");
                throw exception();
            }
            close(cmd.get_writefd());
        }
        
        // error stream
        if(cmd.get_errorfd() != STDERR_FILENO){
            if(dup2(cmd.get_errorfd(), STDERR_FILENO)<0){
                perror("ERROR: errorfd dup failed, ");
                throw exception();
            }
            close(cmd.get_errorfd());
        }
        // remove redundant pointer to avoid reaching maximum pipe limit, numbered pipe not yet
        // eg. remove pipe write end, or pipe read end which is useless to this child process
        // in fact, all the file descriptor which is pointed to ordinary pipe should be remove since this child process' STDIN_FILENO, STDOUT_FILENO, and STDERR_FILENO has taken over the job of pointing to appropriate pipe
        for(int i=0; i<n_ord_pipe; i++){
            close(FD_ord_pipe_table[i][0]);
            close(FD_ord_pipe_table[i][1]);
        }
        
        // redirection stream
        if(cmd.get_c() == ">"){
            int filefd;
            if((filefd = open(cmd.get_argv().at(1), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)<0)){
                perror("ERROR: open file failed, ");
                throw exception();
            };
            if(dup2(filefd, STDOUT_FILENO)<0){
                perror("ERROR: dup failed, ");
                throw exception();
            };
            close(filefd);
        }
        // implement and deal with the unknown command
        execvp(cmd.get_c().c_str(), const_cast<char* const *> (cmd.get_argv().data()));
        //perror("ERROR: exec failed, ");
        cerr << "Unknown command: [" << cmd.get_c() << "]." << endl;
        exit(-1);
    }
    return cpid;
    
}

void Exec_command::execution(){
    cmd_unit cmd(""); int n_ord_pipe = 0;
    set_streams();
    signal(SIGCHLD, SIGCHLD_handler);
    pid_t cpid = fork_exec(cmd_group.at(0), n_ord_pipe);
    cpid_table.push_back(cpid);
    
    // before next one line commmand, all the child process should exit. hence we need to wait to suspend parent process and prevent it continue dealing with the next one line command.
    for(vector<int>::iterator it = cpid_table.begin(); it!=cpid_table.end(); it++){
        if(waitpid((*it), NULL, 0)<0)
            perror("");
    }
    cpid_table.clear();
    
}
