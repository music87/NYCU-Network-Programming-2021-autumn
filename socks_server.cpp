#include <fstream>
#include <iostream>
#include <array>
// to include <boost/asio.hpp>, first you need to download it through $ brew install boost, and then type-in $ brew info boost to check the library path, eg. /usr/local/Cellar/boost/1.76.0/include. after that, open the interface of Xcode project, select Build Settings -> Header Search Paths (not User Header Search Paths), double-click on the right side and input "/usr/local/Cellar/boost/1.76.0/include".
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include <sys/wait.h>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <stdexcept>
#include <regex>
#include "global_variables.h"
#include "socks4_helper.h"

/*
 Test:
    Connect mode:
         curl --proxy socks4://127.0.0.1:7414 https://www.youtube.com
         curl --proxy socks4://127.0.0.1:7414 ftp://anonymous@127.0.0.1:1225/1.txt -O
         curl --proxy socks4://127.0.0.1:7414 ftp://anonymous@127.0.0.1:1225/-X "MKD newDir”
         curl --proxy socks4://127.0.0.1:7414 --ftp-port 0 ftp://anonymous@127.0.0.1:1225/1.txt
         console client (hw4.cgi)
    Bind mode:
         FlashFXP client (select ftp active mode with socks4 proxy)
*/
void debug(std::string who, const unsigned char *buffer, size_t length){
    std::cout << who + ": ";
    for(size_t i=0; i<length; i++)
        std::cout << buffer[i];
    std::cout << std::endl;
}

class session
: public std::enable_shared_from_this<session> {
public:
    session(boost::asio::ip::tcp::socket source_socket, boost::asio::ip::tcp::socket destination_socket, boost::asio::io_context& io_context) : dsource_socket(std::move(source_socket)), ddestination_socket(std::move(destination_socket)), dbind_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0)), dDNS(io_context) {}
    void start();
    
private:
    bool firewall();
    void start_connect();
    void do_resolve();
    void do_connect(boost::asio::ip::tcp::resolver::iterator it);
    void start_bind();
    void do_accept();
 
    void do_read_from_destination();
    void do_write_to_source(size_t length);
    void do_read_from_source();
    void do_write_to_destination(size_t length);
    
    void close_connection();
    
    
    boost::asio::ip::tcp::socket dsource_socket;
    unsigned char dsrc2dst_buffer[MAX_BUFFER_LENGTH];
    boost::asio::ip::tcp::socket ddestination_socket;
    unsigned char ddst2src_buffer[MAX_BUFFER_LENGTH];
    
    socks4_request dsource_request;
    
    boost::asio::ip::tcp::acceptor dbind_acceptor;
    boost::asio::ip::tcp::resolver dDNS;
    
    
};

void session::close_connection(){
    dsource_socket.close();
    ddestination_socket.close();
}

void session::start(){
    // parse request
    boost::asio::read(dsource_socket, dsource_request.buffers());
    std::cout <<
    "<S_IP>: " + dsource_socket.remote_endpoint().address().to_string() + "\n"
    "<S_PORT>: " + std::to_string(dsource_socket.remote_endpoint().port()) + "\n"
    "<D_IP>: " + dsource_request.get_address_friendly_view() + "\n"
    "<D_PORT>: " + dsource_request.get_port_friendly_view() + "\n"
    "<Command>: " + dsource_request.get_command_type_friendly_view() + "\n";
    
    // filter
    if (firewall()) {
        std::cout << "<Reply>: Accept\n" << std::endl;
    } else {
        std::cout << "<Reply>: Reject\n" << std::endl;
        socks4_response connect_response("Reject");
        boost::asio::write(dsource_socket, connect_response.buffers());
        close_connection();
        return;
    }

    
    // connect or bind
    // https://www.openssh.com/txt/socks4.protocol
    if(dsource_request.get_command_type_friendly_view() == "CONNECT"){
        start_connect();
    }else if(dsource_request.get_command_type_friendly_view() == "BIND"){
        start_bind();
    }
    
}

void session::start_connect(){
    // A reply packet is sent to the client when this connection is established, or when the request is rejected or the operation fails.
    do_resolve();
}

void session::start_bind(){
    // The SOCKS server uses the client information to decide whether the request is to be granted. The reply it sends back to the client has the same format as the reply for CONNECT request However, for a granted request (CD is 90), the reply's DSTPORT and DSTIP fields are meaningful.  In that case, the SOCKS server obtains a socket to wait for an incoming connection and sends the port number and the IP address of that socket to the client in DSTPORT and DSTIP, respectively. If the DSTIP in the reply is 0 (the value of constant INADDR_ANY), then the client should replace it by the IP address of the SOCKS server to which the cleint is connected.
    // The SOCKS server sends a second reply packet to the client when the anticipated connection from the application server is established. The SOCKS server checks the IP address of the originating host against the value of DSTIP specified in the client's BIND request.  If a mismatch is found, the CD field in the second reply is set to 91 and the SOCKS server closes both connections.  If the two match, CD in the second reply is set to 90 and the SOCKS server gets ready to relay the traffic on its two connections.
    
    // listen
    dbind_acceptor.listen();
    
    // first bind reply to prepare data channel [proxy ==data==> client]
    socks4_response bind_response1("Accept", dbind_acceptor.local_endpoint().address().to_v4().to_bytes(), ((dbind_acceptor.local_endpoint().port() >> 8) & 0xff), (dbind_acceptor.local_endpoint().port() & 0xff));
    boost::asio::write(dsource_socket, bind_response1.buffers());
    
    // ... client will notify destination host to connect to this proxy ...
    
    // accept
    dbind_acceptor.accept(ddestination_socket);
    
    // second bind reply to check whether the application server is legal. note that port can be different but address should be the same
    std::string app_server_addr = ddestination_socket.remote_endpoint().address().to_string();
    std::string req_dst_addr = dsource_request.get_address_friendly_view();
    if(app_server_addr==req_dst_addr){
        socks4_response bind_response2("Accept");
        boost::asio::write(dsource_socket, bind_response2.buffers());
    } else{
        socks4_response bind_response2("Reject");
        boost::asio::write(dsource_socket, bind_response2.buffers());
        close_connection();
        return;
    }
    
    // start relaying traffic
    do_read_from_destination();
    do_read_from_source();
}

void session::do_resolve(){
    auto self(shared_from_this());
    boost::asio::ip::tcp::resolver::query destination_address(boost::asio::ip::tcp::resolver::query(dsource_request.get_address_friendly_view(), dsource_request.get_port_friendly_view()));
    dDNS.async_resolve(destination_address, [this, self](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it){
        if(!ec){
            do_connect(it);
        } else{
            close_connection();
            boost::asio::detail::throw_error(ec, "resolve");
        }
    });
}

void session::do_connect(boost::asio::ip::tcp::resolver::iterator it){
    auto self(shared_from_this());
    ddestination_socket.async_connect(*it, [this, self](const boost::system::error_code ec){
        if(!ec){
            // connect reply
            socks4_response connect_response("Accept");
            boost::asio::write(dsource_socket, connect_response.buffers());
            // start relaying traffic
            do_read_from_destination();
            do_read_from_source();
        } else{
            close_connection();
            boost::asio::detail::throw_error(ec, "connect");
        }
    });
}

void session::do_read_from_destination(){
    auto self(shared_from_this());
    ddestination_socket.async_receive(boost::asio::buffer(ddst2src_buffer, MAX_BUFFER_LENGTH), [this, self](boost::system::error_code ec, std::size_t length) {
        if(!ec){
            do_write_to_source(length);
        } else{
            close_connection();
            boost::asio::detail::throw_error(ec, "read_from_destination");
        }
    });
}

void session::do_write_to_source(size_t length){
    auto self(shared_from_this());
    dsource_socket.async_send(boost::asio::buffer(ddst2src_buffer, length), [this](boost::system::error_code ec, std::size_t success_length) {
        if (!ec) {
            
            //debug("server", ddst2src_buffer, success_length);
            do_read_from_destination();
        } else{
            close_connection();
            boost::asio::detail::throw_error(ec, "write_to_source");
        }
    });
}

void session::do_write_to_destination(size_t length){
    auto self(shared_from_this());
    ddestination_socket.async_send(boost::asio::buffer(dsrc2dst_buffer, length), [this](boost::system::error_code ec, std::size_t success_length) {
        if (!ec) {
            //debug("client", dsrc2dst_buffer, success_length);
            do_read_from_source();
        } else{
            close_connection();
            boost::asio::detail::throw_error(ec, "write_to_destination");
        }
    });
}

void session::do_read_from_source(){
    auto self(shared_from_this());
    dsource_socket.async_receive(boost::asio::buffer(dsrc2dst_buffer, MAX_BUFFER_LENGTH), [this, self](boost::system::error_code ec, std::size_t length) {
        if(!ec){
            do_write_to_destination(length);
        } else{
            close_connection();
            boost::asio::detail::throw_error(ec, "read_from_source");
        }
    });
}

bool session::firewall(){
    std::ifstream rules("./socks.conf");
    //std::ifstream rules("/Users/xujiawei/Documents/網路程式設計/project4/309551177_np_project4/309551177_np_project4/socks.conf");
    
    // analyze rules
    std::vector<std::regex> white_list;
    while(true){
        std::string line;
        getline(rules, line, '\n');
        if(rules.eof())
            break;
        std::string ip_address = line.substr(sizeof("permit x ")-1);
        boost::replace_all(ip_address, "*", "[0-9]+");
        boost::replace_all(ip_address, ".", "\\.");
        std::regex rule(ip_address);
        
        if((dsource_request.get_command_type_friendly_view() == "CONNECT") && (line.find("permit c ")!=std::string::npos)){
            white_list.push_back(rule);
        } else if((dsource_request.get_command_type_friendly_view() == "BIND") && (line.find("permit b ")!=std::string::npos)){
            white_list.push_back(rule);
        }
    }
    
    // determine reject or not
    bool pass = false;
    for( auto rule: white_list){
        if(regex_match(dsource_request.get_address_friendly_view(), rule)){
            pass = true;
            break;
        }
    }
    
    return pass;
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
                           [this](boost::system::error_code ec, boost::asio::ip::tcp::socket source_socket) {
                               if (!ec) {
                                   // refer to https://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/overview/posix/fork.html
                                   boost::asio::ip::tcp::socket destination_socket{io_context};
                                   io_context.notify_fork(boost::asio::io_service::fork_prepare);
                                   pid_t cpid;
                                   if((cpid=fork())<0){
                                       perror("fork failed");
                                   } else if(cpid == 0){
                                       sleep(10);
                                       // child process
                                       io_context.notify_fork(boost::asio::io_service::fork_child);
                                       dacceptor.close();
                                       dsignal.cancel();
                                       std::make_shared<session>(std::move(source_socket), std::move(destination_socket), io_context)->start();
                                   } else{
                                       // parent process
                                       io_context.notify_fork(boost::asio::io_service::fork_parent);
                                       do_wait();
                                       //std::string client_addr = source_socket.remote_endpoint().address().to_string();
                                       //std::string client_port = std::to_string(source_socket.remote_endpoint().port());
                                       //std::cout << "client's browser connects from " + client_addr + ":" + client_port << std::endl;
                                       source_socket.close();
                                       destination_socket.close();
                                       do_accept();
                                   }
                               } else{
                                   do_accept();
                               }
                           });
}

int main(int argc, const char * argv[]) {
    signal(SIGCHLD, SIG_IGN);
    // to simulate command line argument in Xcode, select Product -> Scheme -> Edit Scheme ... -> Run -> Arguments -> Argument Passed On Launch, and then add the argument you want to pass.
    if (argc != 2) {
        std::cerr << "Usage: ./socks_server <port>\n";
        exit(EXIT_FAILURE);
    }
    // std::cout << "socks(server) start ... pid=" << getpid() << std::endl;
    boost::asio::io_context io_context;
    server s(io_context, atoi(argv[1]));
    while(true){
        // to handle the error of exception: remote_endpoint: Transport endpoint is not connected
        try{
            io_context.run();
            break;
        }
        catch (std::exception &e){
            // std::cout << "io_context's failed: " << e.what() << std::endl;
        }
    }
    
    return 0;
}

