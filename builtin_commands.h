#ifndef builtin_commands_h
#define builtin_commands_h
#include<iostream>
#include<cstring>
#include<vector>
#include<cstdlib>
#include"parsing_line.h"
using namespace std;
class Built_In_Command{

public:
    //constructor
    Built_In_Command() {}; 
    Built_In_Command(vector<cmd_unit> input_cmd_group): cmd_group(input_cmd_group) {};
    void genv();
    void senv();
private:
    vector<cmd_unit> cmd_group;
};

void Built_In_Command::genv(){
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


void Built_In_Command::senv(){
    if(cmd_group.front().get_argv().size() != (3+1)){
        // invalid amount of arguments
        cerr << "ERROR: setenv requires exactly two arguments" << endl;
    } else{
        // add environment variable if it doesn't exist; overwrite it if it does exits
        setenv(cmd_group.front().get_argv().at(1), cmd_group.front().get_argv().at(2), true);
    }
}
#endif
