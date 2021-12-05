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
#define MAX_BUFFER_LENGTH 20000

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
    std::string FILE_NAME;
};

class session
: public std::enable_shared_from_this<session> {
public:
    session(boost::asio::ip::tcp::socket socket, int input_master_sockfd)
    : socket_(std::move(socket)), master_sockfd(input_master_sockfd) {
        slave_sockfd = socket_.native_handle();
    }
    void start();
    
private:
    void do_read();
    
    void parse_request();
    void check_environment_variables();
    void do_response();
    
    boost::asio::ip::tcp::socket socket_;
    int master_sockfd;
    int slave_sockfd;
    char data_[MAX_BUFFER_LENGTH];
    environment_variables env;
};

void session::start(){
    do_read();
}
void session::do_read(){
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, MAX_BUFFER_LENGTH),
                            [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            parse_request();
            // check_environment_variables();
            do_response();
        }
    });
}

void session::parse_request(){
    char request[MAX_BUFFER_LENGTH];
    strcpy(request, data_); // "GET /panel.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=5678&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4= HTTP/1.1\r\nHost: 127.0.0.1:7086\r\nConnection: keep-alive\r\nsec-ch-ua: \"Google Chrome\";v=\"95\", \"Chromium\";v=\"95\", \";Not A Brand\";v=\"99\"\r\nsec-ch-ua-mobile: ?0\r\nsec-ch-ua-platform: \"macOS\"\r\nDNT: 1\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4638.69 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\nSec-Fetch-Site: none\r\nSec-Fetch-Mode: navigate\r\nSec-Fetch-User: ?1\r\nSec-Fetch-Dest: document\r\nAccept-Encoding: gzip, deflate, br\r\nAccept-Language: en-US,en;q=0.9,zh-TW;q=0.8,zh;q=0.7\r\n\r\n\xff\U0000007f"
    int position = 1;
    char const *delimiter = " \r\n";
    char *token = strtok(request, delimiter);
    while(token!=NULL){
        if(position == 1)
            env.REQUEST_METHOD = std::string(token); // "GET"
        else if(position == 2){
            env.REQUEST_URI = std::string(token); // "/console.cgi", "/panel.cgi", "/panel.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=5678&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=", etc.
            size_t split = env.REQUEST_URI.find("?");
            if(split!=std::string::npos){
                env.FILE_NAME = env.REQUEST_URI.substr(0, split); // "/console.cgi"
                env.QUERY_STRING = env.REQUEST_URI.substr(split+1); // ""
            } else{
                env.FILE_NAME = env.REQUEST_URI;
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
    

    env.SERVER_ADDR = socket_.local_endpoint().address().to_string(); // "127.0.0.1"
    env.SERVER_PORT = std::to_string(socket_.local_endpoint().port()); // "7086"
    env.REMOTE_ADDR = socket_.remote_endpoint().address().to_string(); // "127.0.0.1"
    env.REMOTE_PORT = std::to_string(socket_.remote_endpoint().port()); // "50052"
}

void session::check_environment_variables(){
    std::cout << "REQUEST_METHOD = " << env.REQUEST_METHOD << std::endl;
    std::cout << "REQUEST_URI = " << env.REQUEST_URI << std::endl;
    std::cout << "FILE_NAME = " << env.FILE_NAME << std::endl;
    std::cout << "QUERY_STRING = " << env.QUERY_STRING << std::endl;
    std::cout << "SERVER_PROTOCOL = " << env.SERVER_PROTOCOL << std::endl;
    std::cout << "HTTP_HOST = " << env.HTTP_HOST << std::endl;
    std::cout << "SERVER_ADDR = " << env.SERVER_ADDR << std::endl;
    std::cout << "SERVER_PORT = " << env.SERVER_PORT << std::endl;
    std::cout << "REMOTE_ADDR = " << env.REMOTE_ADDR << std::endl;
    std::cout << "REMOTE_PORT = " << env.REMOTE_PORT << std::endl;
}

void session::do_response(){
    pid_t cpid;
    if((cpid=fork())<0){
        perror("fork failed");
    } else if(cpid == 0){
        // child process
        // setenv
        setenv("REQUEST_METHOD", env.REQUEST_METHOD.c_str(), true);
        setenv("REQUEST_URI", env.REQUEST_URI.c_str(), true);
        setenv("FILE_NAME", env.FILE_NAME.c_str(), true);
        setenv("QUERY_STRING", env.QUERY_STRING.c_str(), true);
        setenv("SERVER_PROTOCOL", env.SERVER_PROTOCOL.c_str(), true);
        setenv("HTTP_HOST", env.HTTP_HOST.c_str(), true);
        setenv("SERVER_ADDR", env.SERVER_ADDR.c_str(), true);
        setenv("SERVER_PORT", env.SERVER_PORT.c_str(), true);
        setenv("REMOTE_ADDR", env.REMOTE_ADDR.c_str(), true);
        setenv("REMOTE_PORT", env.REMOTE_PORT.c_str(), true);
        
        // dup
        dup2(slave_sockfd, STDIN_FILENO);
        dup2(slave_sockfd, STDOUT_FILENO);
        close(master_sockfd);
        
        // exec
        std::cout << "HTTP/1.1 200 OK\r\n" ;
        std::cout << "Content-Type: text/html\r\n" ;
        std::vector<const char*> argv;
        argv.push_back(("."+env.FILE_NAME).c_str());
        argv.push_back(NULL);
        execv(argv.front(), const_cast<char* const *> (argv.data()));
        perror("exec failed"); // achieve this line means execv() failed
        exit(EXIT_FAILURE);
        
    } else{
        // parent process
        if(waitpid(cpid, NULL, 0)<0){
            perror("waitpid failed");
        }
        socket_.close(); // close(slave_sockfd);
    }
}


class server{
public:
    server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
        do_accept();
    }
private:
    void do_accept();
    boost::asio::ip::tcp::acceptor acceptor_;
};

void server::do_accept(){
    acceptor_.async_accept(
                           [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                               if (!ec) {
                                   std::string client_addr = socket.remote_endpoint().address().to_string();
                                   std::string client_port = std::to_string(socket.remote_endpoint().port());
                                   std::cout << "client connects from " + client_addr + ":" + client_port << std::endl;
                                   int master_sockfd = acceptor_.native_handle();
                                   std::make_shared<session>(std::move(socket), master_sockfd)->start();
                               }
                               do_accept();
                           });
}

int main(int argc, const char * argv[]) {
    signal(SIGCHLD, SIG_IGN);
    // to simulate command line argument in Xcode, select Product -> Scheme -> Edit Scheme ... -> Run -> Arguments -> Argument Passed On Launch, and then add the argument you want to pass.
    if (argc != 2) {
        std::cerr << "Usage: ./http_server <port>\n";
        exit(EXIT_FAILURE);
    }
    std::cout << "http server start ... " << std::endl;
    boost::asio::io_context io_context;
    server s(io_context, atoi(argv[1]));
    io_context.run();
    
    return 0;
}

