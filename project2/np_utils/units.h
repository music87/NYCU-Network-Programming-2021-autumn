#ifndef units_h
#define units_h
#include<cstring>
#include<vector>
using namespace std;

class stream_unit{
public:
    // constructor
    stream_unit() {};
    stream_unit(int input_readfd, int input_writefd, int input_errorfd, int input_npipe_write_end);
    // accessor
    int get_readfd();
    int get_writefd();
    int get_errorfd();
    int get_npipe_write_end();
    // mutator
    void set_readfd(int input_readfd);
    void set_writefd(int input_writefd);
    void set_errorfd(int input_errorfd);
    void set_npipe_write_end(int input_npipe_write_end);
private:
    int readfd = STDIN_FILENO;
    int writefd = STDOUT_FILENO;
    int errorfd = STDERR_FILENO;
    int npipe_write_end = -1;
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
    // mutator
    void set_s(int input_readfd, int input_writefd, int input_errorfd, int input_npipe_write_end);
private:
    string c;
    vector<const char*> argv;
    stream_unit s;
};

stream_unit::stream_unit(int input_readfd, int input_writefd, int input_errorfd, int input_npipe_write_end) : readfd(input_readfd), writefd(input_writefd), errorfd(input_errorfd), npipe_write_end(input_npipe_write_end){};

int stream_unit::get_readfd(){
    return readfd;
}
int stream_unit::get_writefd(){
    return writefd;
}
int stream_unit::get_errorfd(){
    return errorfd;
}
int stream_unit::get_npipe_write_end(){
    return npipe_write_end;
}

void stream_unit::set_readfd(int input_readfd){
    readfd = input_readfd;
}
void stream_unit::set_writefd(int input_writefd){
    writefd = input_writefd;
}
void stream_unit::set_errorfd(int input_errorfd){
    errorfd = input_errorfd;
}
void stream_unit::set_npipe_write_end(int input_npipe_write_end){
    npipe_write_end = input_npipe_write_end;
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

void cmd_unit::set_s(int input_readfd, int input_writefd, int input_errorfd, int input_npipe_write_end){
    s.set_readfd(input_readfd);
    s.set_writefd(input_writefd);
    s.set_errorfd(input_errorfd);
    s.set_npipe_write_end(input_npipe_write_end);
}


#endif /* units_h */
