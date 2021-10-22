#ifndef built_in_cmd_unit
#define built_in_cmd_unit
#include<iostream>
#include<cstring>
#include<vector>
#include<cstdlib>
#include"parsing_line.h"
using namespace std;
class Built_In_Command{

public:
    void genv(vector<cmd_unit> cmd_group);
    void senv(vector<cmd_unit> cmd_group);

};

void Built_In_Command::genv(vector<cmd_unit> cmd_group){
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


void Built_In_Command::senv(vector<cmd_unit> cmd_group){
    if(cmd_group.front().get_argv().size() != (3+1)){
        // invalid amount of arguments
        cerr << "ERROR: setenv requires exactly two arguments" << endl;
    } else{
        // add environment variable if it doesn't exist; overwrite it if it does exits
        setenv(cmd_group.front().get_argv().at(1), cmd_group.front().get_argv().at(2), true);
    }
}
#endif
