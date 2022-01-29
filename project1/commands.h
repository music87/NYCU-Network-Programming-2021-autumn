#ifndef commands_h
#define commands_h
#include<iostream>
#include<cstdio>
#include<cstring>
#include<vector>
#include<deque>
#include<cstdlib>
#include<signal.h>
#include<regex>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include"units.h"
#define MAX_N_ORD_PIPE 5000
#define MAX_N_NUM_PIPE 1001
#define MAX_N_FORK 100
using namespace std;
/*======== declare ========*/
class Command{
public:
    // constructor, offspring class should explicitly use base class constructor;
    Command(vector<cmd_unit> input_cmd_group) : cmd_group(input_cmd_group) {};
    // abstruct self-defined function, if there is no virtual, execution will always implement Command::execute() rather than its offspring's execute()
    virtual void execute();
protected:
    vector<cmd_unit> cmd_group;
};

class Builtin_setenv : public Command {
public:
    Builtin_setenv(vector<cmd_unit> input_cmd_group) : Command(input_cmd_group) {} ;
    void execute() override;
};

class Builtin_getenv : public Command {
public:
    Builtin_getenv(vector<cmd_unit> input_cmd_group) : Command(input_cmd_group) {} ;
    void execute() override;
};

class Exec_command : public Command {
public:
    Exec_command(vector<cmd_unit> input_cmd_group) : Command(input_cmd_group) {} ;
    void execute() override;
    
private:
    vector<pid_t> cpid_table;
    int ord_pipes[MAX_N_ORD_PIPE][2] = {-1};
    bool error_on = false; // redirect stderr or not
    bool has_numbered_pipe = false;
    // static to make all the one line command shared the "only one" num_pipes table
    // deque<stream_unit> num_pipes = deque<stream_unit> (MAX_N_NUM_PIPE);
    static deque<stream_unit> num_pipes;
    void set_numbered_pipes();
    void implement_one_line_command();
    // static to make class mambers shared by all objects of a class
    static void SIGCHLD_handler(int input_signal);
    pid_t implement_single_command(vector<cmd_unit>::iterator cmd, int n_ord_pipe);
    
};

#endif /* commands_h */
