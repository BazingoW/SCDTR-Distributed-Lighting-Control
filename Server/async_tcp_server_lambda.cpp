#include <iostream>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <list>
#include <chrono>
#include <boost/asio/steady_timer.hpp>
using namespace boost;
using namespace boost::asio;
using ip::tcp;


boost::asio::io_service io;
boost::asio::steady_timer tim(io);


#define NLUMS 2
#define LISTSIZE 6


class DSys
 {
   public:

float curLum[NLUMS];
int curDuty[NLUMS];
int ocu[NLUMS];
float lowLum[NLUMS];
float external[NLUMS];
float refLum[NLUMS];
float  energy[NLUMS];
float  confErr[NLUMS];
float  confVar[NLUMS];

int streamLux[NLUMS]={ 0 };
int streamDuty[NLUMS]={ 0 };

std::list<float> lastLux [NLUMS];
std::list<int> lastDuty [NLUMS];

   DSys (float cl1, float cl2 )
   {
     curLum[0]=cl1;
      curLum[1]=cl2;
    //  streamLux=0;
      //streamDuty=0;
   }
};

std::list<int> InsertDuty(std::list<int> list, int value)
{
list.push_back (value);

if(list.size()>LISTSIZE)
list.pop_front();

return list;
}




std::list<float> InsertLux(std::list<float> list, float value)
{
list.push_back (value);

if(list.size()>LISTSIZE)
list.pop_front();

return list;
}


float getSysPower(DSys* d)
{
return 69;
}
float getSysEnergy(DSys* d)
{
return 8008;
}

float getSysConfErr(DSys* d)
{
return 8008135;
}

float getSysConfVar(DSys* d)
{
return -19;
}

void SerialSend(char* msg)
{

}

void timerLoop(DSys*d)
{
tim.expires_from_now(std::chrono::milliseconds(1000));
tim.async_wait([d](boost::system::error_code ec) {

 std::cout << "LOOOOP" << '\n';

if(d->streamLux[0])
{
  std::cout << "LUUUUX" << '\n';
}
if(d->streamDuty[0])
{
  std::cout << "DUUUUTY" << '\n';
}
 timerLoop(d);
//	std::cout << "Sent! Now closing connection" << std::endl;
});

}




std::string CheckCommand(char* cmd,DSys* d) {


    //esta a pedir a luminosidade
    if (cmd[0] == 'g' && cmd[2] == 'l') {


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
return "l " + std::to_string(id)  + " " +std::to_string(d->curLum[id])  + "\n";
        }

    }

    else if (cmd[0] == 'g' && cmd[2] == 'd') {//esta a pedir o duty cycle


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
return "d " + std::to_string(id)  + " " +std::to_string((d->curDuty[id]/(float)255) *100)  + "\n";
        }


    } else if (cmd[0] == 'g' && cmd[2] == 'o') {//esta a pedir ocupacao


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
return "o " + std::to_string(id)  + " " +std::to_string(d->ocu[id])  + "\n";

        }

    } else if (cmd[0] == 'g' && cmd[2] == 'L') {//esta a pedir lower bound L


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {


return "L " + std::to_string(id)  + " " +std::to_string(d->lowLum[id])  + "\n";
        }

    } else if (cmd[0] == 'g' && cmd[2] == 'O') {//esta a pedir external lumminance


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
return "O " + std::to_string(id)  + " " +std::to_string(d->external[id])  + "\n";
        }

}else if (cmd[0] == 'g' && cmd[2] == 'r') {//esta a pedir reference lumminance


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
return "r " + std::to_string(id)  + " " +std::to_string(d->refLum[id])  + "\n";
        }

    } else if (cmd[0] == 'g' && cmd[2] == 'p' && cmd[4] == 'T') {//total power comsumption



return "p T " + std::to_string(getSysPower(d))  + "\n";

    } else if (cmd[0] == 'g' && cmd[2] == 'p') {//instataneous power consumption


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
            return "p " + std::to_string(id)  + " " +std::to_string(d->curDuty[id] / (float) 255)  + "\n";
                    }




    }else if (cmd[0] == 'g' && cmd[2] == 'e' && cmd[4] == 'T') {//total energy


return "e T " + std::to_string(getSysEnergy(d))  + "\n";


    }else if (cmd[0] == 'g' && cmd[2] == 'e') {//instataneous power consumption


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {


            return "e " + std::to_string(id)  + " " +std::to_string(d->energy[id])  + "\n";
                    }


    }

    else if (cmd[0] == 'g' && cmd[2] == 'c' && cmd[4] == 'T') {//current energy

return "c T " + std::to_string(getSysConfErr(d))  + "\n";



    } else if (cmd[0] == 'g' && cmd[2] == 'c') {//instataneous power consumption


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {

            return "c " + std::to_string(id)  + " " +std::to_string(d->confErr[id])  + "\n";
                    }



    }else if (cmd[0] == 'g' && cmd[2] == 'v' && cmd[4] == 'T') {//current energy

return "v T " + std::to_string(getSysConfVar(d))  + "\n";



    }else if (cmd[0] == 'g' && cmd[2] == 'v') {//instataneous power consumption


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {

            return "v " + std::to_string(id)  + " " +std::to_string(d->confVar[id])  + "\n";
                    }



    }else if (cmd[0] == 's') {//set ocuppancy


        int id = atoi(&cmd[2]);
        int val = atoi(&cmd[4]);


        //send command
        SerialSend((char *)"SET OCU");

        //acknowledge
        return "ack\n";
        //std::cout << "ack" << std::endl;


    }else if (cmd[0] == 'r') {//send system restart


        //send command
        SerialSend((char *)"RESTART SYSTEM");

        //acknowledge
        return "ack\n";


    }else if (cmd[0] == 'b' && cmd[2] == 'd') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            //get elements of list
              return "PRINT DUTY b d  1,2,3,4\n";


        }

    }else if (cmd[0] == 'b' && cmd[2] == 'l') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {
std::cout << "PRINT DA LISTA" << '\n';
          for (auto v : d->lastLux[id])
                std::cout << v << "\n";

            //get elements of list
                return "PRINT LUMS b l 1,2,3,4\n";

        }

    }else if (cmd[0] == 'c' && cmd[2] == 'l') {//stream l


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///START STREAM LUM

std::cout << "STREAM LUX SET TO"<<d->streamLux[id] << '\n';

          d->streamLux[id]=1;



  return "STREAM START luxs \n";
        }

    }else if (cmd[0] == 'c' && cmd[2] == 'd') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///START STREAM DUTY
            d->streamDuty[id]=1;


            }
              return "STREAM START duty \n";

        }

    else if (cmd[0] == 'd' && cmd[2] == 'l') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///START STREAM LUM
  d->streamLux[id]=0;
              return "ack\n";

        }

    }else if (cmd[0] == 'd' && cmd[2] == 'd') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///START STREAM DUTY
  d->streamDuty[id]=0;
            return "ack\n";
        }
    }//DEBUG STUFF
    else if (cmd[0] == 'z' && cmd[1] == 'd') {//last minute resume

std::cout << "VALOR ADICIONADODUTY" << '\n';
    d->lastDuty[0] = InsertDuty(d->lastDuty[0],1);

    }
    else if (cmd[0] == 'z' && cmd[1] == 'l') {//last minute resume

std::cout << "VALOR ADICIONADOLUX" << '\n';
    d->lastLux[0] = InsertLux(d->lastLux[0],1);

    }


    //
    return "";
}

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


 std::cout << "Message Read in Server: "<<data_ << "\n";

 //check if command is valid
msg_ =CheckCommand(data_,d);

if(msg_.empty()){
//std::cout << msg_ << "INVALID CMD \n";
loop(d);
}
else
{std::cout << msg_ << "Good Cmd\n";

 boost::asio::async_write(sock_,boost::asio::buffer(msg_),
    [this,self,d](const boost::system::error_code &ec, std::size_t length){

loop(d);


    });

}


  });
 }


    void start(DSys* d) {
    	auto self = shared_from_this();
    	async_write(sock_, buffer("Hello World\n"),
    	[this, self,d](boost::system::error_code ec, std::size_t length) {

       std::cout << "Server Started" << '\n';
loop(d);
  timerLoop(d);
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


  //  io_service io;
  //  boost::asio::steady_timer tim(io);

  //  tim.expires_from_now(std::chrono::milliseconds(1000));
  //  tim.async_wait(&deadline_handler);
    tcp_server server(io,&dsystem);
    io.run();
} catch(std::exception &e) {std::cout << e.what();}
