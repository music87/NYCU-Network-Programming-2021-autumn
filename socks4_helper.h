#pragma once
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/format.hpp>
#include "global_variables.h"

// ============ socks4 request ============
class socks4_request {
    // refer to https://www.boost.org/doc/libs/1_60_0/doc/html/boost_asio/example/cpp03/socks4/socks4.hpp
public:
    socks4_request(){};
    socks4_request(const std::string cmd, boost::asio::ip::address_v4::bytes_type addr, std::string port): dversion(0x04), duser_id(""), dnull_byte(0){
        if(cmd == "CONNECT")
            dcommand=connect;
        else if(cmd == "BIND")
            dcommand=bind;
        
        dport_high_byte = ((unsigned short)std::atoi((char*)port.c_str()) >> 8) & 0xff;
        dport_low_byte = ((unsigned short)std::atoi((char*)port.c_str())) & 0xff;
        
        daddress = addr;
    };
    
    boost::array<boost::asio::mutable_buffer, 7> buffers();
    boost::asio::ip::address_v4::bytes_type get_address();
    unsigned char get_port_high_byte();
    unsigned char get_port_low_byte();
    
    std::string get_address_friendly_view();
    std::string get_port_friendly_view();
    std::string get_command_type_friendly_view();
    
private:
    enum command_type {
        connect=0x01,
        bind=0x02
    };
    unsigned char dversion;
    unsigned char dcommand;
    unsigned char dport_high_byte; // destination port high byte
    unsigned char dport_low_byte; // destination port low byte
    boost::asio::ip::address_v4::bytes_type daddress; // destination address
    std::string duser_id;
    unsigned char dnull_byte;
};

boost::array<boost::asio::mutable_buffer, 7> socks4_request::buffers() {
    boost::array<boost::asio::mutable_buffer, 7> bufs =
    {
        {
            boost::asio::buffer(&dversion, 1),
            boost::asio::buffer(&dcommand, 1),
            boost::asio::buffer(&dport_high_byte, 1),
            boost::asio::buffer(&dport_low_byte, 1),
            boost::asio::buffer(daddress),
            boost::asio::buffer(duser_id),
            boost::asio::buffer(&dnull_byte, 1)
        }
    };
    return bufs;
}

boost::asio::ip::address_v4::bytes_type socks4_request::get_address(){
    return daddress;
}

unsigned char socks4_request::get_port_high_byte(){
    return dport_high_byte;
}

unsigned char socks4_request::get_port_low_byte(){
    return dport_low_byte;
}

std::string socks4_request::get_address_friendly_view() {
    return boost::asio::ip::address_v4(daddress).to_string();
}

std::string socks4_request::get_port_friendly_view() {
    unsigned short port;
    // eg. port = 1234, dport_high_byte = 4, dport_low_byte = 210, because 1234 = 4*256 + 210 (256 = 2^8 = 8 bits)
    port = (dport_high_byte << 8) & 0xff00; // recover high byte
    port = port | dport_low_byte; // recover low byte
    return std::to_string(port);
}

std::string socks4_request::get_command_type_friendly_view() {
    if (dcommand == connect) {
        return "CONNECT";
    } else if (dcommand == bind) {
        return "BIND";
    }
    throw std::logic_error("unknown socks4 request's command type");
}
// ============ socks4 request ============


// ============ socks4 response ============
class socks4_response{
public:
    socks4_response();
    socks4_response(std::string status);
    socks4_response(std::string status, boost::asio::ip::address_v4::bytes_type addr, unsigned char phb, unsigned char plb);
    //void construct(std::string status);
    //void construct(std::string status, boost::asio::ip::address_v4::bytes_type addr, unsigned char phb, unsigned char plb);
    
    boost::array<boost::asio::mutable_buffer, 5> buffers();
    bool success();
    
private:
    void assign_status(std::string status);
    
    enum status_type {
        request_granted = 0x5a,
        request_failed = 0x5b,
    };
    unsigned char dnull_byte;
    unsigned char dstatus;
    unsigned char dport_high_byte; // destination port high byte
    unsigned char dport_low_byte; // destination port low byte
    boost::asio::ip::address_v4::bytes_type daddress; // destination address
};

socks4_response::socks4_response(){}

socks4_response::socks4_response(std::string status){
    assign_status(status);
    dnull_byte = 0;
    dport_high_byte = 0;
    dport_low_byte = 0;
    daddress = boost::asio::ip::make_address_v4("0.0.0.0").to_bytes();
}


socks4_response::socks4_response(std::string status, boost::asio::ip::address_v4::bytes_type addr, unsigned char phb, unsigned char plb){
    assign_status(status);
    dnull_byte = 0;
    dport_high_byte = phb;
    dport_low_byte = plb;
    daddress = addr;
}

void socks4_response::assign_status(std::string status){
    if(status == "Accept")
        dstatus = request_granted;
    else if(status == "Reject")
        dstatus = request_failed;
}

boost::array<boost::asio::mutable_buffer, 5> socks4_response::buffers() {
    boost::array<boost::asio::mutable_buffer, 5> bufs =
    {
        {
            boost::asio::buffer(&dnull_byte, 1),
            boost::asio::buffer(&dstatus, 1),
            boost::asio::buffer(&dport_high_byte, 1),
            boost::asio::buffer(&dport_low_byte, 1),
            boost::asio::buffer(daddress)
        }
    };
    return bufs;
}

bool socks4_response::success(){
    if(dstatus == request_granted)
        return true;
    else
        return false;
}
// ============ socks4 response ============

