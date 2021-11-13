#ifndef client_units_h
#define client_units_h
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <deque>
#include "units.h"
#define MAX_CLIENTS 31
#define MAX_N_NUM_PIPE 1001

using namespace std;

class client_unit{
public:
    // constructor
    client_unit(int input_sockfd, int input_ID, string input_nickname, string input_IP_port);
    // accessor
    static int get_lowest_avaliable_ID();
    static bool whether_client_exist(int input_ID);
    static bool check_name_avaliable(string input_name);
    int get_sockfd();
    int get_ID();
    string get_nickname();
    string get_IP_port();
    string get_me();
    vector<pair<string,string>> get_environment_variables();
    // mutator
    void set_me(string input_me);
    void set_nickname(string input_nickname);
    void erase_client();
    void add_environment_variables(string input_variable, string input_value);
    // self-defined functions
    static vector<client_unit>::iterator convert_ID_to_client(int input_ID);
    static vector<client_unit>::iterator convert_sockfd_to_client(int sockfd);
    // public variable
    static vector<client_unit> client_alist;
    // each client has his own one_line_pipes table, all the one line command comming from him shares his one_line_pipes table
    deque<stream_unit> one_line_pipes;
    static int user_pipes[MAX_CLIENTS+1][MAX_CLIENTS+1][2];
    // userpipes[1][2] means client1 send data to client2 through pipe write end, and client 2 receive client1's data through pipe read end
    // userpipes[2][1] means client2 send data to client1 through pipe write end, and client1 receive client2's data through pipe read end
    // userpipes[1][2] != userpipes[2][1]
    //         |  client1 |  client2  | client3
    // --------|---------------------------------
    // client1 |    X     |  =R==W=   |  =R==W=
    // --------|---------------------------------
    // client2 |  =R==W=  |     X     |  =R==W=
    // --------|---------------------------------
    // client3 |  =R==W=  |  =R==W=   |    X
    // --------|---------------------------------
private:
    int sockfd;
    int ID; //can't use FD table to record minimal avaliable client ID since there may have file redirection command which requires something like open(file, "w")
    string nickname;
    string IP_port; //=inet_ntoa(client_addr.sin_addr);
    string me; //="<-me" or ""
    static bool avaliable_ID_table[MAX_CLIENTS+1]; // all the client shares one ID table to get lowest avaliable ID, false means that ID is not used, using index start from 1 (not 0)
    vector<pair<string,string>> environment_variables;
};
// initialize avaliable_ID_table, no ID is used at the begining
bool client_unit::avaliable_ID_table[] = {false};
vector<client_unit> client_unit::client_alist = {};
int client_unit::user_pipes[MAX_CLIENTS+1][MAX_CLIENTS+1][2] = {}; // nothing meaningful, just for satisfying legality, the actual initialization is in the main() function


client_unit::client_unit(int input_sockfd, int input_ID, string input_nickname, string input_IP_port) : sockfd(input_sockfd), ID(input_ID), nickname(input_nickname), IP_port(input_IP_port){
    one_line_pipes = deque<stream_unit>(MAX_N_NUM_PIPE);
    me = "";
    environment_variables.push_back(make_pair("PATH", "bin:."));
};

int client_unit::get_sockfd(){
    return sockfd;
}

int client_unit::get_ID(){
    return ID;
}
string client_unit::get_nickname(){
    return nickname;
}
string client_unit::get_IP_port(){
    return IP_port;
}

string client_unit::get_me(){
    return me;
}

vector<pair<string,string>> client_unit::get_environment_variables(){
    return environment_variables;
}

void client_unit::set_me(string input_me){
    me = input_me;
}

void client_unit::set_nickname(string input_nickname){
    nickname = input_nickname;
}

void client_unit::erase_client(){
    avaliable_ID_table[ID] = false;
    client_alist.erase(convert_ID_to_client(ID));
}

void client_unit::add_environment_variables(string input_variable, string input_value){
    bool variable_exist=false;
    for(vector<pair<string, string>>::iterator it = environment_variables.begin(); it<environment_variables.end(); it++){
        if(it->first == input_variable){
            variable_exist = true;
            it->second = input_value;
            break;
        }
    }
        
    if(!variable_exist)
        environment_variables.push_back(make_pair(input_variable, input_value));
}

int client_unit::get_lowest_avaliable_ID(){
    int lowest_avaliable_ID=-1;
    for(int i=1; i<=MAX_CLIENTS;i++){
        if(avaliable_ID_table[i]==false){
            // catch lowest avaliable ID
            avaliable_ID_table[i] = true;
            lowest_avaliable_ID = i; // using index start from 1 (not 0)
            break;
        }
    }
    return lowest_avaliable_ID;
}

bool client_unit::whether_client_exist(int input_ID){
    return avaliable_ID_table[input_ID];
}

vector<client_unit>::iterator client_unit::convert_ID_to_client(int input_ID){
    vector<client_unit>::iterator corresponding_client;
    for(corresponding_client = client_alist.begin(); corresponding_client < client_alist.end(); corresponding_client++){
        if(corresponding_client->get_ID() == input_ID){
            break;
        }
    }
    return corresponding_client;
}

vector<client_unit>::iterator client_unit::convert_sockfd_to_client(int input_sockfd){
    vector<client_unit>::iterator corresponding_client;
    for(corresponding_client = client_alist.begin(); corresponding_client < client_alist.end(); corresponding_client++){
        if(corresponding_client->get_sockfd() == input_sockfd)
            break;
    }
    return corresponding_client;
}


#endif /* client_units_h */
