#include <iostream>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <list>
#include <chrono>
#include <boost/asio/steady_timer.hpp>
#include <thread>         // std::thread

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




using namespace boost;
using namespace boost::asio;
using byte = unsigned char;
using ip::tcp;


boost::asio::io_service io;
boost::asio::steady_timer tim(io);
boost::asio::serial_port sp(io);
int sflag=0;


#define NLUMS 2
#define LISTSIZE 100


class DSys
 {
   public:


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
     lastLux[0].push_back (cl1);
     lastLux[1].push_back (cl2);
     //streamLux=0;
      //streamDuty=0;
   }
};



#define RS (sizeof(gpioReport_t))

#define SCL_FALLING 0
#define SCL_RISING  1
#define SCL_STEADY  2

#define SDA_FALLING 0
#define SDA_RISING  4
#define SDA_STEADY  8

static char * timeStamp()
{
   static char buf[32];

   struct timeval now;
   struct tm tmp;

   gettimeofday(&now, NULL);

   localtime_r(&now.tv_sec, &tmp);
   strftime(buf, sizeof(buf), "%F %T", &tmp);

   return buf;
}

int parse_I2C(int SCL, int SDA)
{
    int result=0;


   static int in_data=0, byte=0, bit=0;
   static int oldSCL=1, oldSDA=1;

   int xSCL, xSDA;

   if (SCL != oldSCL)
   {
      oldSCL = SCL;
      if (SCL) xSCL = SCL_RISING;
      else     xSCL = SCL_FALLING;
   }
   else        xSCL = SCL_STEADY;

   if (SDA != oldSDA)
   {
      oldSDA = SDA;
      if (SDA) xSDA = SDA_RISING;
      else     xSDA = SDA_FALLING;
   }
   else        xSDA = SDA_STEADY;

   switch (xSCL+xSDA)
   {
      case SCL_RISING + SDA_RISING:
      case SCL_RISING + SDA_FALLING:
      case SCL_RISING + SDA_STEADY:
         if (in_data)
         {
            if (bit++ < 8)
            {
               byte <<= 1;
               byte |= SDA;
            }
            else
            {
               printf("%c", byte);//x02
               if (SDA) printf("-"); else printf("+");
               result=byte;
               bit = 0;
               byte = 0;
            }
         }
         break;

      case SCL_FALLING + SDA_RISING:
         break;

      case SCL_FALLING + SDA_FALLING:
         break;

      case SCL_FALLING + SDA_STEADY:
         break;

      case SCL_STEADY + SDA_RISING:
         if (SCL)
         {
            in_data = 0;
            byte = 0;
            bit = 0;

            printf("]\n"); // stop
            result= -2;
            fflush(NULL);
         }
         break;

      case SCL_STEADY + SDA_FALLING:
         if (SCL)
         {
            in_data = 1;
            byte = 0;
            bit = 0;

            printf("["); // start
              result= -1;
         }
         break;

      case SCL_STEADY + SDA_STEADY:
         break;

   }

   return result;
}




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

void SerialSend(byte * cmd)
{
  if(sflag==1)   return;

   std::ostringstream os;
 os << cmd;
     async_write(sp, buffer(os.str()),
        [&](const boost::system::error_code &ec, std::size_t length){

std::cout << "Message successfully sent to arduino" << '\n';

        });

std::cout << "MANDARIA COMANDO PARA O SERIAL" << '\n';

}

void timerLoop(DSys*d)
{

/*FOR DEBUGGING PURPOSES*/
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

void serialLoop(DSys*d)
{

}

void i2cStuff(DSys*d)
{
  //i2c stuff
  int gSCL, gSDA, SCL, SDA, xSCL;
  int r;
  uint32_t level, changed, bI2C, bSCL, bSDA;

  gpioReport_t report;


  int buffer[10];
  int result;
  int k=0;

     gSCL = atoi("3");
     gSDA = atoi("2");

     bSCL = 1<<gSCL;
     bSDA = 1<<gSDA;

     bI2C = bSCL | bSDA;


  SCL = 1;
  SDA = 1;
  level = bI2C;

  while ((r=read(STDIN_FILENO, &report, RS)) == RS)
  {
     report.level &= bI2C;

     if (report.level != level)
     {
        changed = report.level ^ level;

        level = report.level;

        if (level & bSCL) SCL = 1; else SCL = 0;
        if (level & bSDA) SDA = 1; else SDA = 0;

    result=    parse_I2C(SCL, SDA);
if(result!=0)

if(result==-1)
{k=0;

}else if (result==-2)
{



  for (int i = 0; i <6; i++)
  std::cout << buffer[i]<<" ";
std::cout  << '\n';

int id = buffer[0]/2 -1;
if(id>=0 && id<NLUMS)
{
  std::cout << "ADDED VALUE TO" << id<< '\n';
  d->lastDuty[id] = InsertDuty(d->lastDuty[id],1);
}

  //


}

else
{
  buffer[k]=result;
  k++;
}


     }
  }
  return;
}






std::string CheckCommand(char* cmd,DSys* d) {


    //esta a pedir a luminosidade
    if (cmd[0] == 'g' && cmd[2] == 'l') {


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
          //retorna ultima luminosidade recebida
return "l " + std::to_string(id)  + " " +std::to_string(d->lastLux[id].back())  + "\n";
        }

    }

    else if (cmd[0] == 'g' && cmd[2] == 'd') {//esta a pedir o duty cycle


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
          //retorna ultimo dutycylce recebido
return "d " + std::to_string(id)  + " " +std::to_string((d->lastDuty[id].back()/(float)255) *100)  + "\n";
        }


    } else if (cmd[0] == 'g' && cmd[2] == 'o') {//esta a pedir ocupacao


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {

//prints ocupation
std::cout << "o " + std::to_string(id)  + " " +std::to_string(d->ocu[id]) << '\n';

//sends byte array via serial
byte cmdserial[4]={'g',(byte)id,'o','\n'};
SerialSend((byte*)cmdserial);
return "";
        }

    } else if (cmd[0] == 'g' && cmd[2] == 'L') {//esta a pedir lower bound L


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {

          //prints ocupation
          std::cout << "L " + std::to_string(id)  + " " +std::to_string(d->lowLum[id])  + "\n";

          //sends byte array via serial
          byte cmdserial[4]={'g',(byte)id,'L','\n'};
          SerialSend((byte*)cmdserial);
          return "";
        }

    } else if (cmd[0] == 'g' && cmd[2] == 'O') {//esta a pedir external lumminance


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {

//prints ocupation
std::cout << "O " + std::to_string(id)  + " " +std::to_string(d->external[id])  + "\n";

//sends byte array via serial
byte cmdserial[4]={'g',(byte)id,'O','\n'};
SerialSend((byte*)cmdserial);
return "";

        }

}else if (cmd[0] == 'g' && cmd[2] == 'r') {//esta a pedir reference lumminance


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {

//prints ocupation
std::cout << "r " + std::to_string(id)  + " " +std::to_string(d->refLum[id])  + "\n";

//sends byte array via serial
byte cmdserial[4]={'g',(byte)id,'r','\n'};
SerialSend((byte*)cmdserial);
return "";
        }

    } else if (cmd[0] == 'g' && cmd[2] == 'p' && cmd[4] == 'T') {//total power comsumption



return "p T " + std::to_string(getSysPower(d))  + "\n";

    } else if (cmd[0] == 'g' && cmd[2] == 'p') {//instataneous power consumption


        int id = atoi(&cmd[4]);

        if (id < NLUMS) {
            return "p " + std::to_string(id)  + " " +std::to_string(d->lastDuty[id].back() / (float) 255)  + "\n";
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
        SerialSend((byte*)"set\n");

        //acknowledge
        return "ack\n";
        //std::cout << "ack" << std::endl;


    }else if (cmd[0] == 'r') {//send system restart


        //send command
        SerialSend((byte *)"rst\n");

        //acknowledge
        return "ack\n";


    }else if (cmd[0] == 'b' && cmd[2] == 'd') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

          std::string resume="b d " + std::to_string(id)+" ";

          std::cout << "PRINT DA LISTA" << '\n';
                    for (auto v : d->lastDuty[id])
                    {
//std::cout << v << "\n";
resume+=std::to_string(v)+", ";

          }
          resume+="\n";
                std::cout << resume << '\n';
            //get elements of list
              return resume;


        }

    }else if (cmd[0] == 'b' && cmd[2] == 'l') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {



          std::string resume="b l " + std::to_string(id)+" ";

          std::cout << "PRINT DA LISTA" << '\n';
                    for (auto v : d->lastLux[id])
                    {
        //std::cout << v << "\n";
        resume+=std::to_string(v)+", ";


       }
       resume+="\n";
       std::cout << resume << '\n';
            //get elements of list
              return resume;
        }

    }else if (cmd[0] == 'c' && cmd[2] == 'l') {//stream l


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///START STREAM LUM

std::cout << "Stream Started"<< id << '\n';

          d->streamLux[id]=1;
          return "Stream luxs started" + std::to_string(id)+"\n";
        }

    }else if (cmd[0] == 'c' && cmd[2] == 'd') {//stop stream duty


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///START STREAM DUTY
            d->streamDuty[id]=1;
            return "Stream duty started" + std::to_string(id)+"\n";

            }
        }

    else if (cmd[0] == 'd' && cmd[2] == 'l') {//stop stream l


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///STOP STREAM LUM
            d->streamLux[id]=0;
            return "ack\n";

        }

    }else if (cmd[0] == 'd' && cmd[2] == 'd') {//last minute resume


        int id = atoi(&cmd[4]);
        //vai buscar a uma lista estes valores

        if (id < NLUMS) {

            ///STOP STREAM DUTY
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


void ioStuff()
{
  io.run();
}

int main(int argc, char* argv[])
{



  try {

std::string serialPort="/dev/tty";

    if (argc < 3)
    {
      std::cerr << "Usage: server <SerialAddress> <flagserial>\n";
      return 1;
    }
serialPort+=argv[1];

std::cout << argv[2] << '\n';
std::cout << serialPort << '\n';
if(strcmp(argv[2], "-d") == 0)
 {
   /*desativar serial port*/
   sflag=0;
   std::cout << "Serial Comunication Deactivated" << '\n';
 }
 else{
   sflag=1;
 }

DSys dsystem(10,20);

if(sflag)
{
  sp.open(serialPort);    //connect to port
  sp.set_option(serial_port_base::baud_rate(9600));
}

timerLoop(&dsystem);
    tcp_server server(io,&dsystem);


  std::thread io_thrd (ioStuff);     // spawn new thread that calls foo()
    std::thread i2c_thrd (i2cStuff,&dsystem);  // spawn new thread that calls bar(0)

    io_thrd.join();                // pauses until first finishes
    i2c_thrd.join();               // pauses until second finishes



} catch(std::exception &e) {std::cout << e.what();}


return 0;
}
