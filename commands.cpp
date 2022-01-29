#include "commands.h"
deque<stream_unit> Exec_command::num_pipes(MAX_N_NUM_PIPE);

void Command::execute(){
    cerr << "ERROR: class Command's abstract funcition execution override" << endl;
    throw exception();
}

void Builtin_setenv::execute(){
    if(cmd_group.front().get_argv().size() != (3+1)){
        // invalid amount of arguments
        cerr << "ERROR: setenv requires exactly two arguments" << endl;
    } else{
        // add environment variable if it doesn't exist; overwrite it if it does exits
        setenv(cmd_group.front().get_argv().at(1), cmd_group.front().get_argv().at(2), true);
    }
}

void Builtin_getenv::execute(){
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


void Exec_command::set_numbered_pipes(){
    // create numbered pipe
    regex numbered_pipe_stdout("\\|[0-9]+");
    regex numbered_pipe_stdout_stderr("![0-9]+");
    string last_cmd = cmd_group.back().get_c();
    num_pipes.push_back(stream_unit()); // required! or the many many numbered pipes will broke!!!
    if(regex_match(last_cmd, numbered_pipe_stdout) || (error_on = regex_match(last_cmd, numbered_pipe_stdout_stderr))){
        has_numbered_pipe = true;
        int target_one_line_command = stoi(last_cmd.substr(1));
        if(num_pipes.at(target_one_line_command).get_readfd() == STDIN_FILENO){
            // target one line command's first commands's stdin comes from console
            //                 ------------
            //  cur:         ->  O|O|O|O|O -> console
            //                 ------------
            //                 ------------
            //  tar:  console->  O|O|O|O|O ->
            //                 ------------
            //                      |
            //                      V
            //                 ------------
            //  cur:         ->  O|O|O|O|O ->numbered pipe write end
            //                 ------------
            //                                 ------------
            //  tar:  numbered pipe read end->  O|O|O|O|O ->
            //                                 ------------
            // create a numbered pipe
            int numbered_pipe[2];
            pipe(numbered_pipe);
            // redirect target one line command's first commands's stdin from console to numbered pipe read end
            num_pipes.at(target_one_line_command).set_readfd(numbered_pipe[0]);
            // record numbered pipe write end for target one line command's first commands to give other command a chance to also deliver something to this target one line command's first commands
            num_pipes.at(target_one_line_command).set_npipe_write_end(numbered_pipe[1]);
            // redirect current one line command's last commands's stdout from console to numbered pipe write end
            num_pipes.at(0).set_writefd(numbered_pipe[1]);
            if(error_on){
                // redirect current one line command's last commands's stderr from console to numbered pipe write end
                num_pipes.at(0).set_errorfd(numbered_pipe[1]);
            }
        } else {
            // target process' stdin comes from numbered pipe
            //                 ------------
            //  cur:         ->  O|O|O|O|O ->console
            //                 ------------
            //                                 ------------
            //  tar:  numbered pipe read end->  O|O|O|O|O ->
            //                                 ------------
            //                      |
            //                      V
            //                 ------------
            //  cur:         ->  O|O|O|O|O ->numbered pipe write end
            //                 ------------
            //                                 ------------
            //  tar:  numbered pipe read end->  O|O|O|O|O ->
            //                                 ------------
            num_pipes.at(0).set_writefd(num_pipes.at(target_one_line_command).get_npipe_write_end());
            if(error_on){
                num_pipes.at(0).set_errorfd(num_pipes.at(target_one_line_command).get_npipe_write_end());
            }
        }
        cmd_group.pop_back();
    }
}

void Exec_command::implement_one_line_command(){
    // set every command' input stream, output stream, and error stream
    
    // compute and create ordinary pipe
    int N_ord_pipe = 0;
    for(vector<cmd_unit>::iterator it = cmd_group.begin(); it<cmd_group.end(); it++){
        if(it->get_c() == "|"){
            // after delete "|" in cmd_group, the placed which it points to is changed from "|" to next command
            cmd_group.erase(it, it+1);
            N_ord_pipe++;
        }
    }
    
    while(!cmd_group.empty()){
        // limit the amount of one line command's ordinary pipe to prevent encounter fork error
        int n_ord_pipe;
        if(N_ord_pipe > MAX_N_FORK){
            n_ord_pipe = MAX_N_FORK;
            // create psuedo pipe to batch process cmd_group
            int pseudo_pipe[2];
            pipe(pseudo_pipe);
            // insert a psuedo pipe into the numbered pipe list
            num_pipes.insert(num_pipes.begin()+1, stream_unit());
            // the first numbered pipe's stdout will be moved to psuedo pipe's stdout
            //                       ------------
            //                    A->  O|O|O|O|O ->B
            //                       ------------
            //                            |
            //                            V
            //                       ------------
            //                    A->  O|O|O|O|O ->pseudo pipe write end
            //                       ------------
            //                       ------------
            // pseudo pipe read end->  O|O|O|O|O ->B
            //                       ------------
            num_pipes.at(1).set_writefd(num_pipes.at(0).get_writefd());
            num_pipes.at(1).set_errorfd(num_pipes.at(0).get_errorfd());
            num_pipes.at(1).set_readfd(pseudo_pipe[0]);
            num_pipes.at(1).set_npipe_write_end(pseudo_pipe[1]);
            num_pipes.at(0).set_writefd(pseudo_pipe[1]);
            
            
        } else{
            n_ord_pipe = N_ord_pipe;
        }
        N_ord_pipe =  N_ord_pipe - n_ord_pipe-1; // one line command should be "first command | second command" rather than "| first command | second command"
        
        // create ordinary pipes
        for(int i=0; i<n_ord_pipe; i++){
            if(pipe(ord_pipes[i])<0){
                throw exception();
            }
        }
        
        // for each command
        for(vector<cmd_unit>::iterator cmd=cmd_group.begin(); cmd < cmd_group.begin() + n_ord_pipe + 1; cmd++){
            // n ordinary pipes can connect n+1 commands
            // set command's stream
            // input stream and input stream's pipe write end
            long index = distance(cmd_group.begin(), cmd);
            if(cmd == cmd_group.begin()){
                // first command in one line command
                cmd->set_s(num_pipes.front().get_readfd(), cmd->get_s().get_writefd(), cmd->get_s().get_errorfd(), num_pipes.front().get_npipe_write_end());
            } else{
                // other command in one line command
                cmd->set_s(ord_pipes[index-1][0], cmd->get_s().get_writefd(), cmd->get_s().get_errorfd(), ord_pipes[index-1][1]);
            }
            // output stream
            if(cmd == cmd_group.begin() + n_ord_pipe){
                // last command in one line command
                if(error_on){
                    cmd->set_s(cmd->get_s().get_readfd(), num_pipes.front().get_writefd(), num_pipes.front().get_errorfd(), cmd->get_s().get_npipe_write_end());
                } else{
                    cmd->set_s(cmd->get_s().get_readfd(), num_pipes.front().get_writefd(), cmd->get_s().get_errorfd(), cmd->get_s().get_npipe_write_end());
                }
            } else{
                // other command in one line command
                if(error_on){
                    cmd->set_s(cmd->get_s().get_readfd(), ord_pipes[index][1], ord_pipes[index][1], cmd->get_s().get_npipe_write_end());
                } else{
                    cmd->set_s(cmd->get_s().get_readfd(), ord_pipes[index][1], cmd->get_s().get_errorfd(), cmd->get_s().get_npipe_write_end());
                }

            }
            
            // exec command
            pid_t cpid = implement_single_command(cmd, n_ord_pipe);
            cpid_table.push_back(cpid);
            
            // after child process has executed file redirection command , delete ">" in parent process
            if(((cmd+1)!=cmd_group.end()) && (cmd+1)->get_c()==">")
                cmd_group.erase(cmd+1);
            
            // close numbered pipes' write end
            if(cmd == cmd_group.begin() && cmd->get_s().get_readfd() != STDIN_FILENO){
                if(cmd->get_s().get_npipe_write_end()!=-1)
                    close(cmd->get_s().get_npipe_write_end());
                close(cmd->get_s().get_readfd());
            }
            
        }

        cmd_group.erase(cmd_group.begin(), cmd_group.begin() + n_ord_pipe+1);
        
        // close ordinary pipes
        for(int i=0; i<n_ord_pipe; i++){
            close(ord_pipes[i][0]);
            close(ord_pipes[i][1]);
        }
        
        // before next one line commmand, all the child process should exit. hence we need to wait to suspend parent process and prevent it continue dealing with the next one line command.
        // however, if the last command is numbered pipe, then parnet process does not wait for child process to prevent that no other child process will read that numbered pipe and may lead to full out pipe
        
        vector<pid_t>::iterator wait_end;
        if(has_numbered_pipe){
            wait_end = cpid_table.end() -1;
        } else{
            wait_end = cpid_table.end();
        }
        for(vector<int>::iterator it = cpid_table.begin(); it<wait_end; it++){
            if(waitpid((*it), NULL, 0)<0){
                // perror("");
            }
        }
        
        cpid_table.clear();
        
        num_pipes.pop_front();
    }
}

void Exec_command::SIGCHLD_handler(int input_signal){
    
    int status;
    // waitpid: parameter -1 means waiting for any child process, parameter WHOHANG returns immediately if no child has exited, waitpid function returns positive number which means the pid of successfully changed status' child process, the waitpid loop will continue until the last child process exits, after then waitpid will return -1(means no child process exist)
    // the reason that parent process wait when catch SIGCHLD signal is to prevent some pipes is full and no other child process can read those pipes
    while (waitpid(-1, &status, WNOHANG) > 0);
    // perror(""); // expect to print "no child process"
}

pid_t Exec_command::implement_single_command(vector<cmd_unit>::iterator cmd, int n_ord_pipe){
    pid_t cpid;
    while((cpid = fork())<0){
        // if fork error, than fork again
    };
    if (cpid == 0){
        // set redirection stream, file_fd is got from open() function
        // remove redundant pointer to avoid reaching maximum pipe limit, numbered
        // eg. remove pipe write end, or pipe read end which is useless to this child process
        // in fact, all the file descriptor which is pointed to ordinary pipe should be remove since this child process' STDIN_FILENO, STDOUT_FILENO, and STDERR_FILENO has taken over the job of pointing to appropriate pipe
        // however, both ends of the numbered pipe may not be pointed by STDIN_FILENO, STDOUT_FILENO, or STDERR_FILENO, and this child process still connects to the end
        
        // input stream
        if(cmd->get_s().get_readfd() != STDIN_FILENO){
            if(dup2(cmd->get_s().get_readfd(), STDIN_FILENO)<0){
                // int dup2(int oldfd, int newfd) makes newfd be the copy of oldfd, closing newfd first if necessary
                // here break the connection between STDIN_FILENO and console, and then reconnect STDIN_FILENO to the place which pointed by cmd.readfd
                // if dup error, than dup will return -1
                perror("ERROR: readfd dup failed, ");
                throw exception();
            }
            // because the place which pointed by cmd.readfd has also been pointed by this process' STDIN_FILENO
            // close(cmd.get_s().get_readfd());
            // close numbered pipe write end
            if(cmd->get_s().get_npipe_write_end() != -1)
                close(cmd->get_s().get_npipe_write_end());
        }
        
        // output stream
        if(cmd->get_s().get_writefd() != STDOUT_FILENO){
            if(dup2(cmd->get_s().get_writefd(), STDOUT_FILENO)<0){
                perror("ERROR: writefd dup failed, ");
                throw exception();
            }
            // close(cmd.get_s().get_writefd());
        }
        
        // error stream
        if(cmd->get_s().get_errorfd() != STDERR_FILENO){
            if(dup2(cmd->get_s().get_errorfd(), STDERR_FILENO)<0){
                perror("ERROR: errorfd dup failed, ");
                throw exception();
            }
            // close(cmd.get_s().get_errorfd());
        }

        for(int i=0; i<n_ord_pipe; i++){
            close(ord_pipes[i][0]);
            close(ord_pipes[i][1]);
        }
        
        // file redirection stream
        if( (cmd!=(cmd_group.end()-1)) && (cmd+1)->get_c() == ">"){
            int filefd;
            if((filefd = open((cmd+1)->get_argv().at(1), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))<0){
                perror("ERROR: open file failed, ");
                throw exception();
            }
            if(dup2(filefd, STDOUT_FILENO)<0){
                perror("ERROR: filefd dup failed, ");
                throw exception();
            }
            close(filefd);
            // cmd_group.erase(cmd+1); // delete ">" in cmd_group, not work because here is in child process
        }
        // implement and deal with the unknown command
        execvp(cmd->get_c().c_str(), const_cast<char* const *> (cmd->get_argv().data()));
        //perror("ERROR: exec failed, ");
        cerr << "Unknown command: [" << cmd->get_c() << "]." << endl;
        exit(-1);
    }
    return cpid;
    
}

void Exec_command::execute(){
    set_numbered_pipes();
    signal(SIGCHLD, SIGCHLD_handler);
    implement_one_line_command();
    
    /*pid_t cpid = implement_single_command(cmd_group.at(0), 0);
    cpid_table.push_back(cpid);
    
    // before next one line commmand, all the child process should exit. hence we need to wait to suspend parent process and prevent it continue dealing with the next one line command.
    for(vector<int>::iterator it = cpid_table.begin(); it!=cpid_table.end(); it++){
        if(waitpid((*it), NULL, 0)<0){
            // perror("");
        }
    }
    cpid_table.clear();*/
    
}
