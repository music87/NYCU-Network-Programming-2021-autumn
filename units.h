#ifndef units_h
#define units_h
#include<cstring>
#include<vector>
using namespace std;

class stream_unit{
public:
    // constructor
    stream_unit() {};
    stream_unit(int input_readfd, int input_writefd, int input_errorfd);
    // accessor
    int get_readfd();
    int get_writefd();
    int get_errorfd();
    int get_pipe_write_end();
private:
    int readfd = STDIN_FILENO;
    int writefd = STDOUT_FILENO;
    int errorfd = STDERR_FILENO;
    int pipe_write_end = NULL;
};

class cmd_unit{
public:
    // constructor
    cmd_unit(string input_c, vector<const char*> input_argv);
    cmd_unit(string input_c);
    // accessor
    string get_c();
    vector<const char*> get_argv();
    stream_unit get_s();
private:
    string c;
    vector<const char*> argv;
    stream_unit s;
};

stream_unit::stream_unit(int input_readfd, int input_writefd, int input_errorfd) : readfd(input_readfd), writefd(input_writefd), errorfd(input_errorfd){};

int stream_unit::get_readfd(){
    return readfd;
}
int stream_unit::get_writefd(){
    return writefd;
}
int stream_unit::get_errorfd(){
    return errorfd;
}

cmd_unit::cmd_unit(string input_c, vector<const char*> input_argv): c(input_c), argv(input_argv) {};
cmd_unit::cmd_unit(string input_c){
    c.assign(input_c);
    argv.push_back(NULL);
}
string cmd_unit::get_c(){
    return c;
}
vector<const char*> cmd_unit::get_argv(){
    return argv;
}
stream_unit cmd_unit::get_s(){
    return s;
}


#endif /* units_h */
