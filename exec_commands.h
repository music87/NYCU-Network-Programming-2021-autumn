#ifndef exec_commands_h
#define exec_commands_h
#include<iostream>
#include<vector>
#include"parsing_line.h"
using namespace std;

class Exec_Command{
public:
    // constructor
    Exec_Command() {};
    Exec_Command(vector<cmd_unit> input_cmd_group): cmd_group(input_cmd_group) {};
    // self-defined function
    void execution();
private:
    vector<cmd_unit> cmd_group;
    void set_streams(); // set every process' input stream, output stream, and error stream
    void fork_exec();


};


void Exec_Command::set_streams(){


};

void Exec_Command::fork_exec(){

};


void Exec_Command::execution(){


};
#endif
