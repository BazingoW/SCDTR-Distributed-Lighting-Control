//SERIAL_ASYNC.CPP
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include "pigpio.h"



using namespace boost::system;
using namespace boost::asio;
//GLOBALS
io_service io;
serial_port sp(io);
steady_timer tim(io);
streambuf read_buf; //read buffer
int counter = 0;
boost::asio::io_service io_service2;
boost::asio::posix::stream_descriptor out(io, ::dup(STDOUT_FILENO));
std::string input_buffer2;

void handler(boost::system::error_code /*error*/, std::size_t /*bytes_transferred*/) {
   if (std::getline(std::cin, input_buffer2)) {
       input_buffer2 += "\n";
       async_write(out, boost::asio::buffer(input_buffer2), handler);
   }
}

//HANDLERS FOR ASYNC CALLBACKS
//forward declaration of write_handler: timer_handler needs it
void write_handler(const error_code &ec, size_t nbytes);
//timer_handler
void timer_handler(const error_code &ec) {
   //timer expired – launch new write operation
   std::ostringstream os;
   os << "Counter = " << ++counter;
   async_write(sp, buffer(os.str()), write_handler);
}
void write_handler(const error_code &ec, size_t nbytes) {
   //writer done – program new deadline
    tim.expires_from_now(std::chrono::seconds(5));
    tim.async_wait(timer_handler);
}
void read_handler(const error_code &ec, size_t nbytes) {
   //data is now available at read_buf
   std::cout << &read_buf;
   //program new read cycle
    async_read_until(sp,read_buf,'\n',read_handler);
}

int main()
try{
    sp.open("/dev/ttyUSB0");    //connect to port
    sp.set_option(serial_port_base::baud_rate(9600));
    //program timer for write operations
    tim.expires_from_now(std::chrono::seconds(5));
    tim.async_wait(timer_handler);
    //program chain of read operations
    async_read_until(sp,read_buf,'\n',read_handler);

 async_write(out, boost::asio::buffer(""), handler);

    io.run(); //get things rolling
}
catch(std::exception &e){
    std::cout << e.what() << std::endl;
}
