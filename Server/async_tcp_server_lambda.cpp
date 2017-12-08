#include <iostream>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost;
using namespace boost::asio;
using ip::tcp;

#define NLUMS 2



class DSys
 {
   public:

float curLum[NLUMS];

   DSys (float cl1, float cl2 )
   {
     curLum[0]=cl1;
      curLum[1]=cl2;

   }
};

class conn :  public enable_shared_from_this<conn> {
private:
   	tcp::socket sock_;
   	std::string msg_;
   	conn(io_service& io) :  sock_(io)  {}
public:
	static shared_ptr<conn> create(io_service& io) {
         return shared_ptr<conn>(new conn(io));
    }
    tcp::socket& socket() {return sock_;}

    void loop(DSys *d)
     {
       auto self = shared_from_this();

       sock_.async_read_some(boost::asio::buffer(data_, max_length),
       [this,self,d](const boost::system::error_code &ec, std::size_t length){


 std::cout << "A MESSAGE WAS READ:"<<data_ << "WOW \n";
 std::cout << d->curLum[0] << '\n';
 d->curLum[0]++;
 boost::asio::async_write(sock_,boost::asio::buffer("Lux at id are 10\n"),
    [this,self,d](const boost::system::error_code &ec, std::size_t length){

//std::cout << curLums << '\n';
loop(d);


    });


       });
 }


    void start(DSys* d) {
    	auto self = shared_from_this();
    	async_write(sock_, buffer("Hello World\n"),
    	[this, self,d](boost::system::error_code ec, std::size_t length) {

       std::cout << "A MSG WAS SENT" << '\n';
loop(d);
    	//	std::cout << "Sent! Now closing connection" << std::endl;
    	});
    }


    enum { max_length = 1024 };
    char data_[max_length];
};

class tcp_server {
private:
    tcp::acceptor acceptor_;
public:
    tcp_server(io_service& io, DSys *d)
     : acceptor_(io, tcp::endpoint(tcp::v4(), 10000))  {

       std::cout << d->curLum[0] << '\n';
     	start_accept(d);
     }
private:
   void start_accept(DSys *d) {
       	shared_ptr<conn> new_conn =
        	conn::create(acceptor_.get_io_service());
       	acceptor_.async_accept(new_conn->socket(),
       	[this, new_conn,d](boost::system::error_code ec) {
       		new_conn->start(d);
         	start_accept(d);
		});
   }
};
int main()  try {

DSys dsystem(10,20);
std::cout << dsystem.curLum[1]<< "asdsa\n";


    io_service io;
    tcp_server server(io,&dsystem);
    io.run();
} catch(std::exception &e) {std::cout << e.what();}
