#ifndef commands_np_single_proc_h
#define commands_np_single_proc_h
#include <arpa/inet.h>
#include <vector>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <cstdio>
#include <sys/select.h>
#include <regex>
#include <deque>
#include "client_units.h"
#include "units.h"
#define MAX_N_ORD_PIPE 5000
#define MAX_N_FORK 100
using namespace std;

/*void broadcast(string type, vector<client_unit>::iterator from_client, int nfds, fd_set afds, int server_sock_fd, vector<client_unit> client_list={}, string from_message="", int targetID=-1){
    // construct broadcast message
    string broadcast_message;
    if(type == "login"){
        broadcast_message = "*** User '(no name)' entered from " + from_client->get_IP_port() + ". ***\n";
    } else if(type == "logout"){
        broadcast_message = "*** User '" + from_client->get_nickname() + "' left. ***\n";
    } else if(type == "yell"){
        broadcast_message = "*** " + from_client->get_nickname() +" yelled ***: " + from_message + "\n";
    } else if(type == "name"){
        broadcast_message = "*** User from "  + from_client->get_IP_port() + " is named '" + from_client->get_nickname() + "'. ***\n";
    } else if(type == "userPipeReceive"){
        vector<client_unit>::iterator to_client = client_unit::convert_ID_to_client(client_list, targetID);
        broadcast_message = "*** " + from_client->get_nickname() + " (#" + to_string(from_client->get_ID()) + ") just received from " + to_client->get_nickname() + " (#" + to_string(to_client->get_ID()) + ") by '" + from_message + "' ***\n";
    } else if(type == "userPipeSend"){
        vector<client_unit>::iterator to_client = client_unit::convert_ID_to_client(client_list, targetID);
        broadcast_message = "*** " + from_client->get_nickname() + " (#" + to_string(from_client->get_ID()) + ") just piped '" + from_message + "' to " + to_client->get_nickname() + " (#" + to_string(to_client->get_ID()) + ") ***\n";

    } else{
        // error
        exit(EXIT_FAILURE);
    }
    
    // broadcast to every active client
    for(int fd=0; fd < nfds; fd++){
        if(fd == server_sock_fd) continue;
        if (!FD_ISSET(fd, &afds)) continue;
        if (::send(fd, broadcast_message.c_str(), broadcast_message.size(), 0) < 0)
            perror("broadcast failed");
    }
}*/

void broadcast(string message, int nfds, fd_set afds, int server_sock_fd){
    // broadcast to every active client
    for(int fd=0; fd < nfds; fd++){
        if(fd == server_sock_fd) continue;
        if (!FD_ISSET(fd, &afds)) continue;
        if (::send(fd, message.c_str(), message.size(), 0) < 0)
            perror("broadcast failed");
    }
}

void login(int slave_sock_fd, int server_sock_fd, int input_nfds, fd_set input_afds){
    string welcome_message =
        "****************************************\n"
        "** Welcome to the information server. **\n"
        "****************************************\n";
    if(::send(slave_sock_fd, welcome_message.c_str(), welcome_message.size(), 0) <0)
        perror("welcome failed");
    string message = "*** User '(no name)' entered from " + client_unit::convert_sockfd_to_client(slave_sock_fd)->get_IP_port() + ". ***\n";
    broadcast(message, input_nfds, input_afds, server_sock_fd);
}

void logout(int slave_sock_fd, int server_sock_fd, int input_nfds, fd_set &afds){
    vector<client_unit>::iterator client = client_unit::convert_sockfd_to_client(slave_sock_fd);
    string message = "*** User '" + client->get_nickname() + "' left. ***\n";
    broadcast(message, input_nfds, afds, server_sock_fd);
    // check and close remaining user pipe related to this client
    for(int i=1; i<=MAX_CLIENTS;i++){
        if(client_unit::user_pipes[client->get_ID()][i][0]!=-1){
            close(client_unit::user_pipes[client->get_ID()][i][0]);
            client_unit::user_pipes[client->get_ID()][i][0]=-1;
        }
        if(client_unit::user_pipes[client->get_ID()][i][1]!=-1){
            close(client_unit::user_pipes[client->get_ID()][i][1]);
            client_unit::user_pipes[client->get_ID()][i][1]=-1;
        }
        if(client_unit::user_pipes[i][client->get_ID()][0]!=-1){
            close(client_unit::user_pipes[i][client->get_ID()][0]);
            client_unit::user_pipes[i][client->get_ID()][0]=-1;
        }
        if(client_unit::user_pipes[i][client->get_ID()][1]!=-1){
            close(client_unit::user_pipes[i][client->get_ID()][1]);
            client_unit::user_pipes[i][client->get_ID()][1]=-1;
        }
    }
    // check and close remaining number pipe related to this client
    for(deque<stream_unit>::iterator it=client->one_line_pipes.begin(); it<client->one_line_pipes.end(); it++){
        if(it->get_writefd()!=STDOUT_FILENO)
            close(it->get_writefd());
        if(it->get_readfd()!=STDIN_FILENO)
            close(it->get_readfd());
        if(it->get_errorfd()!=STDERR_FILENO)
            close(it->get_errorfd());
        if(it->get_npipe_write_end()!=-1)
            close(it->get_npipe_write_end());
    }
    client->erase_client();
    if(close(slave_sock_fd)<0)
        perror("close left client's socket failed");
    FD_CLR(slave_sock_fd, &afds);
}


void set_client_env(vector<client_unit>::iterator client, vector<cmd_unit> cmd_group){
    if(cmd_group.front().get_argv().size() != (3+1)){
        // invalid amount of arguments
        cerr << "ERROR: setenv requires exactly two arguments" << endl;
    } else{
        // add environment variable if it doesn't exist; overwrite it if it does exits
        string variable = cmd_group.front().get_argv().at(1);
        string value = cmd_group.front().get_argv().at(2);
        client->add_environment_variables(variable, value);
    }
}

void get_client_env(vector<cmd_unit> cmd_group){
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

bool cmp(client_unit a, client_unit b) {
    return a.get_ID() < b.get_ID();
}
void who(int input_ID){
    // eg. who();
    // show information of all users.
    cout << "<ID>\t<nickname>\t<IP:port>\t<indicate me>" << endl;
    ::sort(client_unit::client_alist.begin(), client_unit::client_alist.end(), cmp);
    vector<client_unit>::iterator client = client_unit::convert_ID_to_client(input_ID); //should set after sorting because the place where the iterator points to is based on relative position in vector
    for(vector<client_unit>::iterator it = client_unit::client_alist.begin(); it<client_unit::client_alist.end(); it++){
        if(it->get_ID() == client->get_ID())
            cout << it->get_ID() << "\t" << it->get_nickname() << "\t" << it->get_IP_port() << "\t" << "<-me" << endl;
        else
            cout << it->get_ID() << "\t" << it->get_nickname() << "\t" << it->get_IP_port() << "\t" << "" << endl;
    }
    /*struct sockaddr_in c, s;
    socklen_t cLen = sizeof(c);
    socklen_t sLen = sizeof(s);
    getsockname(slave_sock_fd, (struct sockaddr*) &s, &sLen);
    getpeername(slave_sock_fd, (struct sockaddr*) &c, &cLen);
    cout << "client connects from " << inet_ntoa(c.sin_addr) << ", port=" << ntohs(c.sin_port) << endl;
    cout << "server is on " << inet_ntoa(s.sin_addr) << ", port=" << ntohs(s.sin_port) << endl;*/
}

void tell(vector<client_unit>::iterator from_client, vector<cmd_unit> cmd_group){
    // send a message to another user
    int to_ID = atoi(cmd_group.front().get_argv().at(1));
    string sentence = cmd_group.front().get_argv().at(2);
    bool to_client_exist = client_unit::whether_client_exist(to_ID);
    if(to_client_exist){
        vector<client_unit>::iterator to_client = client_unit::convert_ID_to_client(to_ID);
        string message =  "*** " + from_client->get_nickname() + " told you ***: " + sentence +"\n";
        if(::send(to_client->get_sockfd(), message.c_str(), message.size(), 0) <0)
            perror("tell failed");
    } else{
        cout << "*** Error: user #" << to_ID << " does not exist yet. ***"<< endl;
    }
}

void yell(vector<client_unit>::iterator client, vector<cmd_unit> cmd_group, int server_sock_fd, int input_nfds, fd_set input_afds){
    // send a message to all users
    if(cmd_group.front().get_argv().size()<2){
        cerr << "ERROR: yell requires some setences" << endl;
    }else{
        string sentence = cmd_group.front().get_argv().at(1);
        string message = "*** " + client->get_nickname() +" yelled ***: " + sentence + "\n";
        broadcast(message, input_nfds, input_afds, server_sock_fd);
    }
    
}

void name(vector<client_unit>::iterator client, vector<cmd_unit> cmd_group, int server_sock_fd, int input_nfds, fd_set input_afds){
    // eg. name(client, "daitau");
    // change user's name by himself
    // check if the name is different with all the other users
    string new_nickname = cmd_group.front().get_argv().at(1);
    bool avaliable = true;
    for(vector<client_unit>::iterator it=client_unit::client_alist.begin(); it < client_unit::client_alist.end();it++){
        if(new_nickname == it->get_nickname()){
            avaliable = false;
            break;
        }
    }
    // if not used, than rename; else fail
    if(avaliable){
        client->set_nickname(new_nickname);
        string message = "*** User from "  + client->get_IP_port() + " is named '" + client->get_nickname() + "'. ***\n";
        broadcast(message, input_nfds, input_afds, server_sock_fd);
    } else{
        cout << "*** User '" + new_nickname + "' already exists. ***" << endl;
    }
    
}

class Exec_command{
public:
    Exec_command(vector<client_unit>::iterator input_client, vector<cmd_unit> input_cmd_group, int input_server_sock_fd, int input_nfds, fd_set input_afds, string input_line) : client(input_client), cmd_group(input_cmd_group), server_sock_fd(input_server_sock_fd), nfds(input_nfds), afds(input_afds), whole_one_line_command(input_line) {
        devnull = open("/dev/null", O_WRONLY);
    };
    ~Exec_command(){
        close(devnull);
    }
    void execute();
    
private:
    vector<client_unit>::iterator client;
    vector<cmd_unit> cmd_group;
    int server_sock_fd;
    int nfds;
    fd_set afds;
    string whole_one_line_command;
    vector<pid_t> cpid_table;
    int ord_pipes[MAX_N_ORD_PIPE][2] = {-1};
    bool error_on = false; // redirect stderr or not
    bool has_numbered_pipe = false;
    bool has_user_pipe_send = false;
    bool has_user_pipe_receive = false;
    int user_pipe_target_client_ID = -1;
    int devnull;
    void set_numbered_pipes();
    bool set_user_pipes();
    void implement_one_line_command();
    pid_t implement_single_command(vector<cmd_unit>::iterator cmd, int n_ord_pipe);
};

void Exec_command::set_numbered_pipes(){
    // create numbered pipe
    regex numbered_pipe_stdout("\\|[0-9]+");
    regex numbered_pipe_stdout_stderr("![0-9]+");
    string last_cmd = cmd_group.back().get_c();
    client->one_line_pipes.push_back(stream_unit()); // required! or the many many numbered pipes will broke!!!
    if(regex_match(last_cmd, numbered_pipe_stdout) || (error_on = regex_match(last_cmd, numbered_pipe_stdout_stderr))){
        has_numbered_pipe = true;
        int target_one_line_command = stoi(last_cmd.substr(1));
        if(client->one_line_pipes.at(target_one_line_command).get_readfd() == STDIN_FILENO){
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
            client->one_line_pipes.at(target_one_line_command).set_readfd(numbered_pipe[0]);
            // record numbered pipe write end for target one line command's first commands to give other command a chance to also deliver something to this target one line command's first commands
            client->one_line_pipes.at(target_one_line_command).set_npipe_write_end(numbered_pipe[1]);
            // redirect current one line command's last commands's stdout from console to numbered pipe write end
            client->one_line_pipes.at(0).set_writefd(numbered_pipe[1]);
            if(error_on){
                // redirect current one line command's last commands's stderr from console to numbered pipe write end
                client->one_line_pipes.at(0).set_errorfd(numbered_pipe[1]);
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
            // reuse the numbered pipe
            client->one_line_pipes.at(0).set_writefd(client->one_line_pipes.at(target_one_line_command).get_npipe_write_end());
            if(error_on){
                client->one_line_pipes.at(0).set_errorfd(client->one_line_pipes.at(target_one_line_command).get_npipe_write_end());
            }
        }
        cmd_group.pop_back();
    }
}


bool Exec_command::set_user_pipes(){
    regex user_pipe_receive("<[0-9]+");
    regex user_pipe_send(">[0-9]+");
    // possible situations:
    // 1. % cat >2 <1
    // 2. % cat <1 >2
    // 3. % cat <1 | cat | cat | cat >2
    // => last_cmd always is >2 while second_cmd may be >2 or <1
    string last_cmd = cmd_group.back().get_c();
    string second_cmd = "";
    if(cmd_group.size()>1)
        second_cmd = cmd_group.at(1).get_c();
    int ReadEnd = 0;
    int WriteEnd = 1;
    
    if(regex_match(second_cmd, user_pipe_receive) || regex_match(last_cmd, user_pipe_receive)){
        // <#
        int target_ID = (regex_match(second_cmd, user_pipe_receive))? stoi(second_cmd.substr(1)) : stoi(last_cmd.substr(1));
        if(!client_unit::whether_client_exist(target_ID)){
            // target user doesn't exist
            //  client_tar: doesn't exist
            //                 ------------
            //  client_cur:  ->  O|O|O|O|O -> console
            //                 ------------
            //                      |
            //                      V
            //                           ------------
            //  client_cur:  /dev/null ->  O|O|O|O|O -> console
            //                           ------------
            // output *** Error: user #tar does not exist yet. *** on client_cur's stdout(console)
            cout << "*** Error: user #" << target_ID << " does not exist yet. ***" << endl;
            //client->one_line_pipes.front().set_readfd(devnull);
            //throw std::runtime_error("*** Error: user #" + to_string(target_ID) + " does not exist yet. ***");
            return false;
        } else{
            // target user exist
            vector<client_unit>::iterator client_target = client_unit::convert_ID_to_client(target_ID);
            if(client_unit::user_pipes[client_target->get_ID()][client->get_ID()][ReadEnd] != -1){
                // user pipe #tar->#cur exist
                //                         ------------
                //  client_tar:         ->  O|O|O|O|O -> already send
                //                         ------------
                //                         ------------
                //  client_cur:  console ->  O|O|O|O|O ->
                //                         ------------
                //                              |
                //                              V
                //                         ------------
                //  client_tar:         ->  O|O|O|O|O -> already send
                //                         ------------
                //                                    ------------
                //  client_cur:  user pipe read end ->  O|O|O|O|O ->
                //                                    ------------
                // conncet to the user pipe read end and broadcast *** client_cur (#cur) just received from client_tar (#tar) by 'command_cur' ***
                // after that, close that user pipe (later after finishing one line command, or later command executation will receive command from /dev/null rather than user pipe read end)
                has_user_pipe_receive = true; //diff
                string message = "*** " + client->get_nickname() + " (#" + to_string(client->get_ID()) + ") just received from " + client_target->get_nickname() + " (#" + to_string(client_target->get_ID()) + ") by '" + whole_one_line_command + "' ***\n";
                broadcast(message, nfds, afds, server_sock_fd);
                client->one_line_pipes.front().set_readfd(client_unit::user_pipes[client_target->get_ID()][client->get_ID()][ReadEnd]);
                user_pipe_target_client_ID = target_ID;
            } else{
                // user pipe #tar->#cur doesn't exist
                //                         ------------
                //  client_tar:         ->  O|O|O|O|O -> console
                //                         ------------
                //                         ------------
                //  client_cur:  console ->  O|O|O|O|O ->
                //                         ------------
                //                      |
                //                      V
                //                         ------------
                //  client_tar:         ->  O|O|O|O|O -> console
                //                         ------------
                //                           ------------
                //  client_cur:  /dev/null ->  O|O|O|O|O -> console
                //                           ------------
                // this means that user pipe doesn't exist
                // output *** Error: the pipe #tar->#cur does not exist yet. *** on client_cur's stdout(console)
                cout << "*** Error: the pipe #" << client_target->get_ID() << "->#" << client->get_ID() << " does not exist yet. ***" << endl;
                //client->one_line_pipes.front().set_readfd(devnull);
                //throw runtime_error("*** Error: the pipe #" + to_string(client_target->get_ID()) + "->#" + to_string(client->get_ID()) + " does not exist yet. ***");
                return false;
            }
        }
        // delete <# command from cmd_group
        vector<cmd_unit>::iterator it = regex_match(second_cmd, user_pipe_receive)? (cmd_group.begin()+1):(cmd_group.end()-1);
        cmd_group.erase(it);
    }
    
    
    if(regex_match(second_cmd, user_pipe_send) || regex_match(last_cmd, user_pipe_send)){
        // >#
        int target_ID = (regex_match(second_cmd, user_pipe_send))? stoi(second_cmd.substr(1)) : stoi(last_cmd.substr(1));
        if(!client_unit::whether_client_exist(target_ID)){
            // target user doesn't exist
            //                 ------------
            //  client_cur:  ->  O|O|O|O|O -> console
            //                 ------------
            //  client_tar: doesn't exist
            //                      |
            //                      V
            //                 ------------
            //  client_cur:  ->  O|O|O|O|O -> /dev/null
            //                 ------------
            // output *** Error: user #<user_id> does not exist yet. *** on client_cur's stdout(console)
            cout << "*** Error: user #" << target_ID << " does not exist yet. ***" << endl;
            //client->one_line_pipes.front().set_writefd(devnull);
            //throw runtime_error("*** Error: user #" + to_string(target_ID) + " does not exist yet. ***");
            return false;
        } else{
            // target user exist
            vector<client_unit>::iterator client_target = client_unit::convert_ID_to_client(target_ID);
            if(client_unit::user_pipes[client->get_ID()][client_target->get_ID()][WriteEnd] == -1){
                // user pipe #cur->#tar doesn't exist
                //                         ------------
                //  client_cur:         ->  O|O|O|O|O -> console
                //                         ------------
                //                         ------------
                //  client_tar:  console->  O|O|O|O|O ->
                //                         ------------
                //                              |
                //                              V
                //                         ------------
                //  client_cur:         ->  O|O|O|O|O -> user pipe write end
                //                         ------------
                //                                    ------------
                //  client_tar:  wait for receiving ->  O|O|O|O|O ->
                //                                    ------------
                // create a user pipe and broadcast *** client_cur (#cur) just piped 'command_cur' to client_tar (#tar) ***
                has_user_pipe_send = true; //diff
                if(pipe( client_unit::user_pipes[client->get_ID()][client_target->get_ID()] )<0){
                    perror("create user pipe error");
                } else{
                    // success to create user pipe
                    string message = "*** "+ client->get_nickname() + " (#" + to_string(client->get_ID()) + ") just piped '" + whole_one_line_command + "' to " + client_target->get_nickname() + " (#" + to_string(client_target->get_ID()) + ") ***\n";
                    broadcast(message, nfds, afds, server_sock_fd);
                    client->one_line_pipes.front().set_writefd(client_unit::user_pipes[client->get_ID()][client_target->get_ID()][WriteEnd]);
                    client_target->one_line_pipes.front().set_npipe_write_end(client_unit::user_pipes[client->get_ID()][client_target->get_ID()][WriteEnd]); // diff
                }
            } else{
                // user pipe #cur->#tar has already exist
                //                         ------------
                //  client_cur:         ->  O|O|O|O|O -> console
                //                         ------------
                //                                    ------------
                //  client_tar:  wait for receiving ->  O|O|O|O|O ->
                //                                    ------------
                //                              |
                //                              V
                //                         ------------
                //  client_cur:         ->  O|O|O|O|O -> /dev/null
                //                         ------------
                //                                    ------------
                //  client_tar:  wait for receiving ->  O|O|O|O|O ->
                //                                    ------------
                // this means that client_tar hasn't receive the command from client_cur, hence client_cur transfer data to client_tar fails this time
                // output *** Error: the pipe #cur->#tar already exists. *** on client_cur's stdout(console)
                cout << "*** Error: the pipe #" << client->get_ID() << "->#" << client_target->get_ID() << " already exists. ***" <<endl;
                //client->one_line_pipes.front().set_writefd(devnull);
                //throw runtime_error("*** Error: the pipe #" + to_string(client->get_ID()) + "->#" + to_string(client_target->get_ID()) + " already exists. ***");
                return false;
            }
        }
        // delete ># command from cmd_group
        vector<cmd_unit>::iterator it = regex_match(second_cmd, user_pipe_send)? (cmd_group.begin()+1):(cmd_group.end()-1);
        cmd_group.erase(it);
    }
    return true;
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
            client->one_line_pipes.insert(client->one_line_pipes.begin()+1, stream_unit());
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
            client->one_line_pipes.at(1).set_writefd(client->one_line_pipes.at(0).get_writefd());
            client->one_line_pipes.at(1).set_errorfd(client->one_line_pipes.at(0).get_errorfd());
            client->one_line_pipes.at(1).set_readfd(pseudo_pipe[0]);
            client->one_line_pipes.at(1).set_npipe_write_end(pseudo_pipe[1]);
            client->one_line_pipes.at(0).set_writefd(pseudo_pipe[1]);
            
            
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
                cmd->set_s(client->one_line_pipes.front().get_readfd(), cmd->get_s().get_writefd(), cmd->get_s().get_errorfd(), client->one_line_pipes.front().get_npipe_write_end());
            } else{
                // other command in one line command
                cmd->set_s(ord_pipes[index-1][0], cmd->get_s().get_writefd(), cmd->get_s().get_errorfd(), ord_pipes[index-1][1]);
            }
            // output stream
            if(cmd == cmd_group.begin() + n_ord_pipe){
                // last command in one line command
                if(error_on){
                    cmd->set_s(cmd->get_s().get_readfd(), client->one_line_pipes.front().get_writefd(), client->one_line_pipes.front().get_errorfd(), cmd->get_s().get_npipe_write_end());
                } else{
                    cmd->set_s(cmd->get_s().get_readfd(), client->one_line_pipes.front().get_writefd(), cmd->get_s().get_errorfd(), cmd->get_s().get_npipe_write_end());
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
            
            // check whether there exist user pipe which no longer will be used
            if(has_user_pipe_receive){
                close(client_unit::user_pipes[user_pipe_target_client_ID][client->get_ID()][0]);
                close(client_unit::user_pipes[user_pipe_target_client_ID][client->get_ID()][1]);
                client_unit::user_pipes[user_pipe_target_client_ID][client->get_ID()][0] = -1;
                client_unit::user_pipes[user_pipe_target_client_ID][client->get_ID()][1] = -1;
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
        if(has_numbered_pipe || has_user_pipe_send){
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
        
        client->one_line_pipes.pop_front();
    }
    

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
        
        // close all the ordinary pipes
        for(int i=0; i<n_ord_pipe; i++){
            close(ord_pipes[i][0]);
            close(ord_pipes[i][1]);
        }
        // close server socket
        close(server_sock_fd);
        
        // close other client's slave socket
        for(vector<client_unit>::iterator it=client_unit::client_alist.begin(); it< client_unit::client_alist.end(); it++){
            if(it->get_ID() != client->get_ID()){
                close(it->get_sockfd());
            }
        }
        
        // close all the user pipes
        for(int i=1; i<=MAX_CLIENTS;i++){
            for(int j=1;j<=MAX_CLIENTS;j++){
                if(client_unit::user_pipes[i][j][0]!=-1)
                    close(client_unit::user_pipes[i][j][0]);
                if(client_unit::user_pipes[i][j][1]!=-1)
                    close(client_unit::user_pipes[i][j][1]);
            }
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
    signal(SIGCHLD, SIG_IGN);
    bool success = set_user_pipes();
    if(!success)
        return;
    set_numbered_pipes();
    implement_one_line_command();
}



#endif /* commands_np_single_proc_h */
