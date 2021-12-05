#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include "global_variables.h"

class shell_unit{
public:
    shell_unit(std::string input_session, std::string input_host, std::string input_port, std::string input_test_file) : session(input_session), host(input_host), port(input_port), test_file(input_test_file){};
    // bad coding style but I'm tired ...
    std::string session;
    std::string host;
    std::string port;
    std::string test_file;
};

void set_html_format(std::vector<shell_unit> shell_info_list){
    // refer to extra_files/cgi/sample_console.cgi
    std::string html_prefix =
    "Content-type: text/html\r\n\r\n"
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n";
    
    std::string head =
    "  <head>\n"
    "    <meta charset=\"UTF-8\" />\n"
    "    <title>NP Project 3 Sample Console</title>\n"
    "    <link\n"
    "      rel=\"stylesheet\"\n"
    "      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
    "      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
    "      crossorigin=\"anonymous\"\n"
    "    />\n"
    "    <link\n"
    "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
    "      rel=\"stylesheet\"\n"
    "    />\n"
    "    <link\n"
    "      rel=\"icon\"\n"
    "      type=\"image/png\"\n"
    "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
    "    />\n"
    "    <style>\n"
    "      * {\n"
    "        font-family: 'Source Code Pro', monospace;\n"
    "        font-size: 1rem !important;\n"
    "      }\n"
    "      body {\n"
    "        background-color: #212529;\n"
    "      }\n"
    "      pre {\n"
    "        color: #cccccc;\n"
    "      }\n"
    "      b {\n"
    "        color: #01b468;\n"
    "      }\n"
    "    </style>\n"
    "  </head>\n";
    
    std::string th = "";
    std::string td = "";
    for(std::vector<shell_unit>::iterator it=shell_info_list.begin(); it<shell_info_list.end(); it++){
        th += "<th scope=\"col\">" + it->host + ":" + it->port + "</th>\n";
        td += "<td><pre id=\"s" + it->session + "\" class=\"mb-0\"></pre></td>\n";
    }
    
    std::string body =
    "  <body>\n"
    "    <table class=\"table table-dark table-bordered\">\n"
    "      <thead>\n"
    "        <tr>\n"
                + th +
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody>\n"
    "        <tr>\n"
                + td +
    "        </tr>\n"
    "      </tbody>\n"
    "    </table>\n"
    "  </body>\n";
    
    std::string html_suffix =
    "</html>\n";
    
    std::string format = html_prefix + head + body + html_suffix;
    
    std::cout << format << std::endl;
}

std::vector<shell_unit> parse_QUERY_STRING(std::string query){
    std::vector<shell_unit> shell_info_list;
    char const *delimiter = "h";
    char *tmp_query = strdup(query.c_str());
    char *token = strtok(tmp_query, delimiter);
    int shell_amount = 0;
    while(token!=NULL){
        std::string cur = std::string(token); //"0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt&" or "0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt"
        if(cur.substr(cur.find("=")+1)!=""){
            // handle host
            std::string cur_host = cur.substr(cur.find("=")+1, cur.find("&")-cur.find("=")-1);
            cur = cur.substr(cur.find("&")+1);
            if(cur_host == "")
                break; // if the amount of host is less than 5, than early stop
            
            // handle port
            std::string cur_port = cur.substr(cur.find("=")+1, cur.find("&")-cur.find("=")-1);
            cur = cur.substr(cur.find("&")+1);
            
            // handle test file
            std::string cur_test_file;
            if(cur.find("&")!=std::string::npos){
                cur_test_file = cur.substr(cur.find("=")+1, cur.find("&")-cur.find("=")-1);
            } else{
                cur_test_file = cur.substr(cur.find("=")+1);
            }
            
            shell_info_list.push_back(shell_unit(std::to_string(shell_amount),cur_host, cur_port, cur_test_file));
            shell_amount++;
        }
        token = strtok(NULL, delimiter);
    }
    
    return shell_info_list;
}

class shell
: public std::enable_shared_from_this<shell> {
public:
    shell(boost::asio::io_context& io_context, std::vector<shell_unit>::iterator it)
    : session(it->session), shell_address(boost::asio::ip::tcp::resolver::query(it->host, it->port)), DNS(io_context), tcp_socket(io_context){
        active_test_file.open("test_case/" + it->test_file, std::fstream::in);
    };
    void start();
    
private:
    void do_resolve();
    void do_connect(boost::asio::ip::tcp::resolver::iterator it);
    void display(std::string message, std::string mode);
    void do_read_from_tcp_socket();
    void do_write_to_tcp_scoket();
    
    std::string session;
    std::fstream active_test_file;
    boost::asio::ip::tcp::resolver::query shell_address;
    boost::asio::ip::tcp::resolver DNS;
    boost::asio::ip::tcp::socket tcp_socket;
    char dbuffer[MAX_BUFFER_LENGTH];
};

void shell::start(){
    // DNS.async_resolve()
    // tcp_socket.async_connect()
    // while(){
    //     tcp_socket.async_read_some()
    //     tcp_socket.write_some()
    // }
    do_resolve();
}

void shell::do_resolve(){
    auto self(shared_from_this());
    DNS.async_resolve(shell_address, [this, self](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it){
        if(!ec){
            do_connect(it);
        }
    });
}

void shell::do_connect(boost::asio::ip::tcp::resolver::iterator it){
    auto self(shared_from_this());
    tcp_socket.async_connect(*it, [this, self](const boost::system::error_code ec){
        if(!ec){
            do_read_from_tcp_socket();
        }
    });
    
}

void shell::display(std::string message, std::string mode){
    // convert special symbols to prevent mess up html syntax
    // std::cout << "original message=<<" << message << ">>"  << std::endl;
    
    boost::replace_all(message, "&", "&amp;");
    boost::replace_all(message, "'", "&#x27;");
    boost::replace_all(message, "\n", "&NewLine;");
    boost::replace_all(message, "\r", "");
    boost::replace_all(message, "\"", "&quot;");
    boost::replace_all(message, "\\", "&bsol;");
    boost::replace_all(message, "/", "&sol;");
    boost::replace_all(message, "<", "&lt;");
    boost::replace_all(message, ">", "&gt;");
    boost::replace_all(message, ",", "&sbquo;");
    
    if(mode == "input")
        std::cout << "<script>document.getElementById('s" + session + "').innerHTML += '<b>" + message + "</b>';</script>"  << std::endl;
    else if(mode == "output")
        std::cout << "<script>document.getElementById('s" + session + "').innerHTML += '" + message + "';</script>" << std::endl;
}


void shell::do_read_from_tcp_socket(){
    auto self(shared_from_this());
    tcp_socket.async_read_some(boost::asio::buffer(dbuffer, MAX_BUFFER_LENGTH), [this, self](boost::system::error_code ec, std::size_t length) {
        if(!ec){
            // output
            std::string message(dbuffer);
            message = message.substr(0, length);
            
            display(message, "output");
            
            
            // input
            if(message.find("% ") != std::string::npos){
                do_write_to_tcp_scoket();
            }
            
            // self-loop
            do_read_from_tcp_socket();
        }
    });
    
}
void shell::do_write_to_tcp_scoket(){
    auto self(shared_from_this());
    std::string command;
    getline(active_test_file, command, '\n');
    command += '\n';
    display(command, "input");
    tcp_socket.write_some(boost::asio::buffer(command, command.length()));
}





int main(){
    std::string env_QUERY_STRING = getenv("QUERY_STRING");
    
    std::vector<shell_unit> shell_info_list = parse_QUERY_STRING(env_QUERY_STRING);
    set_html_format(shell_info_list);
    
    boost::asio::io_context io_context;
    
    for(std::vector<shell_unit>::iterator shell_info = shell_info_list.begin(); shell_info<shell_info_list.end(); shell_info++){
        std::make_shared<shell>(io_context, shell_info)->start();
    }
    
    io_context.run();
    
    return 0;
    
}

