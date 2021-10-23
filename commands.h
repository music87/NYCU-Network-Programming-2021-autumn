#ifndef commands_h
#define commands_h
#include<iostream>
#include<cstdio>
#include<cstring>
#include<vector>
#include<cstdlib>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include"parsing_line.h"
#define MAX_N_ORD_PIPE 3000
using namespace std;
/*======== declare ========*/
class Command{
public:
    // constructor, offspring class should explicitly use base class constructor;
    Command(vector<cmd_unit> input_cmd_group) : cmd_group(input_cmd_group) {};
    // abstruct self-defined function, if there is no virtual, execution will always implement Command::execution() rather than its offspring's execution()
    virtual void execution();
protected:
    vector<cmd_unit> cmd_group;
};

class Builtin_setenv : public Command {
public:
    Builtin_setenv(vector<cmd_unit> input_cmd_group) : Command(input_cmd_group) {} ;
    void execution() override;
};

class Builtin_getenv : public Command {
public:
    Builtin_getenv(vector<cmd_unit> input_cmd_group) : Command(input_cmd_group) {} ;
    void execution() override;
};

class Exec_command : public Command {
public:
    Exec_command(vector<cmd_unit> input_cmd_group) : Command(input_cmd_group) {} ;
    void execution() override;
    
private:
    vector<pid_t> cpid_table;
    int FD_ord_pipe_table[MAX_N_ORD_PIPE][2];
    void set_streams();
    //static to make class mambers shared by all objects of a class
    static void SIGCHLD_handler(int input_signal);
    pid_t fork_exec(cmd_unit cmd, int n_ord_pipe);
    
};

#endif /* commands_h */
