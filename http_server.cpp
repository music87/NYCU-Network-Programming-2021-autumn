#include <iostream>
// to include <boost/asio.hpp>, first you need to download it through $ brew install boost, and then type-in $ brew info boost to check the library path, eg. /usr/local/Cellar/boost/1.76.0/include. after that, open the interface of Xcode project, select Build Settings -> Header Search Paths (not User Header Search Paths), double-click on the right side and input "/usr/local/Cellar/boost/1.76.0/include".
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <sys/wait.h>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include "global_variables.h"
/*
 http_server:
 do_accept(){
    fork(): do_read(), parse_request(), do_response(): setenv, dup, std::cout << "HTTP/1.1 200 OK\r\n"; , exec
    do_wait()
    do_accept()
 }
 */

/*
 [INFO] Test http_server
       http://nplinux5.cs.nctu.edu.tw:38860/printenv.cgi?course_name=NP
       http://nplinux5.cs.nctu.edu.tw:38860/hello.cgi
       http://nplinux5.cs.nctu.edu.tw:38860/welcome.cgi

 [INFO] Test http_server and console.cgi (combined)
       http://nplinux5.cs.nctu.edu.tw:38860/panel.cgi
 */

struct environment_variables{
    std::string REQUEST_METHOD;
    std::string REQUEST_URI;
    std::string QUERY_STRING;
    std::string SERVER_PROTOCOL;
    std::string HTTP_HOST;
    std::string SERVER_ADDR;
    std::string SERVER_PORT;
    std::string REMOTE_ADDR;
    std::string REMOTE_PORT;
    std::string CGI_FILE;
};

class session
: public std::enable_shared_from_this<session> {
public:
    session(boost::asio::ip::tcp::socket socket) : dsocket(std::move(socket)) {}
    void start();
    
private:
    void do_read();
    void parse_request();
    void do_response();
    
    boost::asio::ip::tcp::socket dsocket;
    char ddata[MAX_BUFFER_LENGTH];
    environment_variables env;
};

void session::start(){
    do_read();
}
void session::do_read(){
    auto self(shared_from_this());
    dsocket.async_read_some(boost::asio::buffer(ddata, MAX_BUFFER_LENGTH),
                            [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            parse_request();
            do_response();
        }
    });
}

void session::parse_request(){
    char request[MAX_BUFFER_LENGTH];
    strcpy(request, ddata); // "GET /panel.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=5678&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4= HTTP/1.1\r\nHost: 127.0.0.1:7086\r\nConnection: keep-alive\r\nsec-ch-ua: \"Google Chrome\";v=\"95\", \"Chromium\";v=\"95\", \";Not A Brand\";v=\"99\"\r\nsec-ch-ua-mobile: ?0\r\nsec-ch-ua-platform: \"macOS\"\r\nDNT: 1\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4638.69 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nSec-Fetch-Site: none\r\nSec-Fetch-Mode: navigate\r\nSec-Fetch-User: ?1\r\nSec-Fetch-Dest: document\r\nAccept-Encoding: gzip, deflate, br\r\nAccept-Language: en-US,en;q=0.9,zh-TW;q=0.8,zh;q=0.7\r\n\r\n\xff\U0000007f"
    int position = 1;
    char const *delimiter = " \r\n";
    char *token = strtok(request, delimiter);
    while(token!=NULL){
        if(position == 1)
            env.REQUEST_METHOD = std::string(token); // "GET"
        else if(position == 2){
            env.REQUEST_URI = std::string(token); // "/console.cgi", "/panel.cgi", "/console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=5678&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=", etc.
            size_t split = env.REQUEST_URI.find("?");
            if(split!=std::string::npos){
                env.CGI_FILE = env.REQUEST_URI.substr(0, split); // "/panel.cgi"
                env.QUERY_STRING = env.REQUEST_URI.substr(split+1); // ""
            } else{
                env.CGI_FILE = env.REQUEST_URI; // "/console.cgi"
                env.QUERY_STRING = ""; // "h0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=5678&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4="
            }
        }
        else if(position == 3)
            env.SERVER_PROTOCOL = std::string(token); // "HTTP/1.1"
        else if(position == 5){
            env.HTTP_HOST = std::string(token); // "127.0.0.1:7086"
            break;
        }
        token = strtok (NULL, delimiter);
        position++;
    }
    

    env.SERVER_ADDR = dsocket.local_endpoint().address().to_string(); // "127.0.0.1"
    env.SERVER_PORT = std::to_string(dsocket.local_endpoint().port()); // "7086"
    env.REMOTE_ADDR = dsocket.remote_endpoint().address().to_string(); // "127.0.0.1"
    env.REMOTE_PORT = std::to_string(dsocket.remote_endpoint().port()); // "50052"
}

void session::do_response(){
    std::cout << "do response, pid=" << getpid() << std::endl;
    // setenv
    setenv("REQUEST_METHOD", env.REQUEST_METHOD.c_str(), true);
    setenv("REQUEST_URI", env.REQUEST_URI.c_str(), true);
    setenv("CGI_FILE", env.CGI_FILE.c_str(), true);
    setenv("QUERY_STRING", env.QUERY_STRING.c_str(), true);
    setenv("SERVER_PROTOCOL", env.SERVER_PROTOCOL.c_str(), true);
    setenv("HTTP_HOST", env.HTTP_HOST.c_str(), true);
    setenv("SERVER_ADDR", env.SERVER_ADDR.c_str(), true);
    setenv("SERVER_PORT", env.SERVER_PORT.c_str(), true);
    setenv("REMOTE_ADDR", env.REMOTE_ADDR.c_str(), true);
    setenv("REMOTE_PORT", env.REMOTE_PORT.c_str(), true);
    
    // dup
    dup2(dsocket.native_handle(), STDIN_FILENO);
    dup2(dsocket.native_handle(), STDOUT_FILENO);
    dup2(dsocket.native_handle(), STDERR_FILENO);
        
    // exec
    std::cout << "HTTP/1.1 200 OK\r\n" ;
    std::cout << "Content-Type: text/html\r\n" ;
    std::cout << std::flush;
    std::vector<const char*> argv;
    argv.push_back(("."+env.CGI_FILE).c_str());
    //argv.push_back(env.CGI_FILE.c_str());
    argv.push_back(NULL);
    execv(argv.front(), const_cast<char* const *> (argv.data()));
    perror("exec failed"); // achieve this line means execv() failed
    exit(EXIT_FAILURE);
}

class server{
public:
    server(boost::asio::io_context& io_context, short port)
    : io_context(io_context), dacceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)), dsignal(io_context, SIGCHLD) {
        do_accept();
    }
private:
    void do_wait();
    void do_accept();
    boost::asio::io_context &io_context;
    boost::asio::ip::tcp::acceptor dacceptor;
    boost::asio::signal_set dsignal;
    
};

void server::do_wait(){
    // parent process
    dsignal.async_wait(
            [this](boost::system::error_code ec, int sig) {
                while(waitpid(-1, NULL, WNOHANG) > 0);
                do_wait();
            });
}

void server::do_accept(){
    dacceptor.async_accept(
                           [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                               if (!ec) {
                                   // refer to https://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/overview/posix/fork.html
                                   io_context.notify_fork(boost::asio::io_service::fork_prepare);
                                   pid_t cpid;
                                   if((cpid=fork())<0){
                                       perror("fork failed");
                                   } else if(cpid == 0){
                                       // child process
                                       io_context.notify_fork(boost::asio::io_service::fork_child);
                                       dacceptor.close();
                                       dsignal.cancel();
                                       std::make_shared<session>(std::move(socket))->start();
                                   } else{
                                       // parent process
                                       io_context.notify_fork(boost::asio::io_service::fork_parent);
                                       do_wait();
                                       std::string client_addr = socket.remote_endpoint().address().to_string();
                                       std::string client_port = std::to_string(socket.remote_endpoint().port());
                                       std::cout << "client's browser connects from " + client_addr + ":" + client_port << std::endl;
                                       socket.close();
                                       do_accept();
                                   }
                               }
                           });
}

int main(int argc, const char * argv[]) {
    signal(SIGCHLD, SIG_IGN);
    // to simulate command line argument in Xcode, select Product -> Scheme -> Edit Scheme ... -> Run -> Arguments -> Argument Passed On Launch, and then add the argument you want to pass.
    if (argc != 2) {
        std::cerr << "Usage: ./http_server <port>\n";
        exit(EXIT_FAILURE);
    }
    std::cout << "http server start ... pid=" << getpid() << std::endl;
    boost::asio::io_context io_context;
    server s(io_context, atoi(argv[1]));
    while(true){
        // to handle the error of exception: remote_endpoint: Transport endpoint is not connected
        try{
            io_context.run();
            break;
        }
        catch (std::exception &e){
            std::cout << "io_context's failed exception: " << e.what() << std::endl;
        }
    }
    
    return 0;
}

