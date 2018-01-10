#include <iostream>
#include <fstream>
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


#ifdef __arm__
#include "pigpio.h"
#endif

#include <mutex>
#include <condition_variable>

std::mutex mut;
std::condition_variable cvi2c;
std::condition_variable cvstream;
std::string data;
int flag=0;

using namespace boost;
using namespace boost::asio;
using byte = unsigned char;
using ip::tcp;


boost::asio::io_service io;
boost::asio::steady_timer tim(io);
boost::asio::serial_port sp(io);
int sflag=0;


#define NLUMS 2
#define LISTSIZE 1000
#define SAVEINTERVAL 5 // every 5 * (ARDUINODELAY) ms it saves on the list

class DSys
{
public:


  int ocu[NLUMS]={0};
  float lowLum[NLUMS];
  float external[NLUMS];
  float refLum[NLUMS]={20};
  float  energy[NLUMS]={0};
  float  confErr[NLUMS]={0};
  std::ofstream lums1;
  std::ofstream lums2;

  std::ofstream duts1;
  std::ofstream duts2;


int dutyCounter[NLUMS]={0};
int luxCounter[NLUMS]={0};

  int N[NLUMS]={1};
  int Nl[NLUMS]={1};
  float  confVar[NLUMS]={0};

  //flag if stream is on or not
  int streamLux[NLUMS]={ 0 };
  int streamDuty[NLUMS]={ 0 };


bool isreading = false;
bool iswriting = false;

  //flag new information for the stream
  int streamflagLux[NLUMS]={ 0 };
  int streamflagDuty[NLUMS]={ 0 };

  //flag new info external
  int flagExternal[NLUMS]={ 0 };
  int flagLowLum[NLUMS]={ 0 };
  int flagRefLum[NLUMS]={ 0 };
  int flagOcu[NLUMS]={ 0 };

  std::list<float> lastLux [NLUMS];
  std::list<int> lastDuty [NLUMS];


  //this are list arrays
  std::list<std::chrono::steady_clock::time_point> luxTime [NLUMS];
  std::list<std::chrono::steady_clock::time_point> dutyTime [NLUMS];

  //this is a normal array
  std::chrono::steady_clock::time_point lastTimeRecv [NLUMS];

  std::chrono::steady_clock::time_point lastTimeRecvLux [NLUMS];




  std::chrono::steady_clock::time_point start_time;


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
        // printf("%c", byte);//x02
        // if (SDA) printf("-"); else printf("+");
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

      //  printf("]\n"); // stop
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

      //  printf("["); // start
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

std::list<std::chrono::steady_clock::time_point> InsertTime(std::list<std::chrono::steady_clock::time_point> list)
{
  list.push_back (std::chrono::steady_clock::now());

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
  float aux=0;

  for (int i = 0; i < NLUMS; i++) {

    aux+=d->lastDuty[i].back() / (float) 255;
  }


  return aux;
}


float getSysEnergy(DSys* d)
{
  float aux=0;

  for (int i = 0; i < NLUMS; i++) {

    aux+=d->energy[i];
  }

  return aux;
}

float getSysConfErr(DSys* d)
{
  float aux=0;

  for (int i = 0; i < NLUMS; i++) {

    aux+=d->confErr[i]/d->N[i];
  }

  return aux;
}

float getSysConfVar(DSys* d)
{
  float aux=0;

  for (int i = 0; i < NLUMS; i++) {

    aux+=d->confVar[i]/d->Nl[i];
  }

  return aux;
}

void SerialSend(std::string cmd)
{
  if(sflag==0)   return;

std::cout << "MANDOU:" << cmd;

  async_write(sp, buffer(cmd),
  [&](const boost::system::error_code &ec, std::size_t length){

    std::cout << "Message successfully sent to arduino" << '\n';

  });



}



float doubleInt2Float(int i1,int i2)
{
  if(i1==200) i1=0;
  if(i2==200) i2=0;

  std::string s1 = std::to_string(i1);
  s1+="." +std::to_string(i2);

  return ::atof(s1.c_str());
}






void serialLoop(DSys*d)
{

}

#ifdef __arm__


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

        std::unique_lock<std::mutex> lk(mut);
        while (d->isreading==true)   cvi2c.wait(lk);
        d->iswriting=true;

        int id = buffer[3]-1;
        int id2= buffer[3]-'b';//converte em 0 ou 1
        float aux=0;
        if(id>=0 && id<NLUMS)
        {

          if(buffer[1]=='$')
          {
            if(buffer[2]=='d')//adiciona ao duty cycle
            {
              //converte percentagem em duty
              aux =doubleInt2Float(buffer[4],buffer[5]);

            //  std::cout << "VALOR DO MIRAGAIA: " <<aux<< '\n';
              int v= aux;


              if(d->dutyCounter[id]==SAVEINTERVAL)
              {
                //insere novo duty cycle
                d->lastDuty[id] = InsertDuty(d->lastDuty[id],v);

                //insere novo time
                d->dutyTime[id] = InsertTime(d->dutyTime[id]);

                if(id==0)
                {

                  if (d->duts1.is_open())
                  {
                    d->duts1 << d->lastDuty[id].back()<<";";

                  }

                }
                else
                {
                  if (d->duts2.is_open())
                  {
                    d->duts2 << d->lastDuty[id].back()<<";";

                  }
                }


                std::chrono::duration<double,std::milli> elapsed3(std::chrono::steady_clock::now()-d->start_time);

                                if(id==0)
                                {

                                  if (d->duts1.is_open())
                                  {

                                    d->duts1 << elapsed3.count()<<"\n";



                                  }

                                }
                                else
                                {
                                  if (d->duts2.is_open())
                                  {
                                    d->duts2 << elapsed3.count()<<"\n";

                                  }
                                }


                std::chrono::duration<double,std::milli> elapsed1(std::chrono::steady_clock::now() - d->dutyTime[id].front());
              //  std::cout << "TEMPO ULTIME" <<elapsed1.count() << '\n';

                while(elapsed1.count()>60000)
                {
                  //std::cout << "apaga" << '\n';

                  d->lastDuty[id].pop_front ();
                  d->dutyTime[id].pop_front();

                  elapsed1=std::chrono::steady_clock::now() - d->dutyTime[id].front();
                }



                d->dutyCounter[id]=0;
              }
              d->dutyCounter[id]++;


              //define como available para streamar
              d->streamflagDuty[id] = 1;

              //initializes times in the first time
              if(d->energy[id]==0)
              d->lastTimeRecv[id] = std::chrono::steady_clock::now();


              //gets time interval
              std::chrono::duration<double,std::milli> elapsed(std::chrono::steady_clock::now() - d->lastTimeRecv[id]);

              //sets new last time
              d->lastTimeRecv[id] = std::chrono::steady_clock::now();



              //std::cout << v << "\n";
              std::string s = std::to_string(elapsed.count());

              //std::cout << "TIME PRINTS" << s << "ms\n";

              //std::cout << (v/(float)255) <<"VERY NICE"<<(elapsed.count()) <<'\n';
              //setEnergu
              d->energy[id]+=(v/(float)255)*(elapsed.count()/1000);


            }else if(buffer[2]=='1')//adiciona ao lux
            {
              //converte em float
              aux =doubleInt2Float(buffer[4],buffer[5]);

              if(d->luxCounter[id]==SAVEINTERVAL)
              {
                d->lastLux[id] = InsertLux(d->lastLux[id],aux);

if(id==0)
{

  if (d->lums1.is_open())
  {
    d->lums1 << d->lastLux[id].back()<<";";

  }

}
else
{
  if (d->lums2.is_open())
  {
    d->lums2 << d->lastLux[id].back()<<";";

  }
}


                //insere novo time
                d->luxTime[id] = InsertTime(d->luxTime[id]);



std::chrono::duration<double,std::milli> elapsed3(std::chrono::steady_clock::now()-d->start_time);

                if(id==0)
                {

                  if (d->lums1.is_open())
                  {

                    d->lums1 << elapsed3.count()<<"\n";



                  }

                }
                else
                {
                  if (d->lums2.is_open())
                  {
                    d->lums2 << elapsed3.count()<<"\n";

                  }
                }


                std::chrono::duration<double,std::milli> elapsed1(std::chrono::steady_clock::now() - d->luxTime[id].front());
                //  std::cout << "TEMPO ULTIME" <<elapsed1.count() << '\n';

                while(elapsed1.count()>60000)
                {
                  //std::cout << "apaga" << '\n';

                  d->lastLux[id].pop_front ();
                  d->luxTime[id].pop_front();

                  elapsed1=std::chrono::steady_clock::now() - d->luxTime[id].front();
                }

                d->luxCounter[id]=0;
              }
            d->  luxCounter[id]++;



              d->streamflagLux[id] = 1;
              //calcs confErr
              float confort= d->refLum[id]-aux;

              if(confort<0)confort=0;



              d->confErr[id]=d->confErr[id] +confort;
              d->N[id]++;




              //DA PARA CALCULAR VARIANCIA
              if(d->lastLux[id].size()>5)
              {


                //std::cout << d->lastLux[id].back() << '\n';

                //std::cout << "PRINT LISTA" << '\n';
                float auxer=0;
                int n=0;
                //iterador na posicao zero da estranho
                for (std::list<float>::iterator it =  d->lastLux[id].end(); n<4; --it){

                  //  std::cout << n << '\n';
                  float ola = *it;
                  //faz cena dentro do modulo
                //  std::cout << "VALAOR EQ" << ola << '\n';
                  if(n==2)
                  auxer-=2*ola;
                  else if(n>0)
                  auxer+=ola;


                  n++;
                }

                if(auxer<0)
                {
                  auxer*=-1;
                }

//std::cout << "VAR " <<auxer<< '\n';

                //initializes times in the first time



                //gets time interval
                std::chrono::duration<double,std::milli> elapsed(std::chrono::steady_clock::now() - d->lastTimeRecvLux[id]);

//std::cout << "TEMPO:" <<elapsed.count() << '\n';

                //sets new last time
                d->lastTimeRecvLux[id] = std::chrono::steady_clock::now();


//std::cout << "A SOMAR" << auxer << '\n';
//std::cout << "TEMPO " << elapsed.count()<< '\n';

      auxer/=((elapsed.count()/1000)*(elapsed.count()/1000));
//std::cout << "RESULT " << auxer << '\n';
                d->Nl[id]++;
                d->confVar[id]=(d->confVar[id]  +auxer);
  //              std::cout << "LOLLOL " <<   d->confVar[id] << '\n';
    //            std::cout << "LALAL " <<   d->Nl[id] << '\n';

//std::cout << "RESULTADO  " <<   d->confVar[id] / d->Nl[id]  << '\n';
                //std::cout << auxer << '\n';
              }
              if(d->confVar[id]==0)
              d->lastTimeRecvLux[id] = std::chrono::steady_clock::now();





              //std::cout << "CONF ERR IS " <<d->confErr[id] << '\n';

            }
            else if(buffer[2]=='L')
            {
              //std::cout << "RECEBI O LOW LUMMMMMMMMMMMM" << '\n';
              d->flagLowLum[id]=1;
              d-> lowLum[id]=doubleInt2Float(buffer[4],buffer[5]);


            }
            else if(buffer[2]=='o')
            {
              //std::cout << "RECEBI O OCUPANCIA" << '\n';

              d->flagOcu[id]=1;
              //std::cout << d->flagOcu[id] << '\n';
              if(buffer[4]==1)
              d->ocu[id]=1;
              else
              d->ocu[id]=0;



            }else if(buffer[2]=='O')
            {
              //std::cout << "RECEBI O EXTERNAL" << '\n';
              //flag new info external


              //std::cout << doubleInt2Float(buffer[4],buffer[5]) << '\n';
              d->flagExternal[id]=1;
              d-> external[id]=doubleInt2Float(buffer[4],buffer[5]);

            }else if(buffer[2]=='r')
            {
              //std::cout << "Reference Received" << '\n';
              d->flagRefLum[id]=1;
              d-> refLum[id]=doubleInt2Float(buffer[4],buffer[5]);
            }

          }else if(buffer[1]=='R')
          {
            std::cout << "System Started" << '\n';
            d->start_time =  std::chrono::steady_clock::now();


            for (int i = 0; i < NLUMS; i++) {

               d->energy[i]=0;
               d->confErr[i]=0;
               d->N[i]=1;
               d->Nl[i]=1;
              d->confVar[i]=0;
            }








          }

          /*
          std::cout << "ADDED VALUE TO" << id<< '\n';
          d->lastDuty[id] = InsertDuty(d->lastDuty[id],1);
          d->dutyTime[id] = InsertTime(d->dutyTime[id]);

          */

        }
        d->iswriting = false;

        cvstream.notify_one();
        lk.unlock();


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
#endif





std::string CheckCommand(char* cmd,DSys* d) {

std::string sendmsg;

  //esta a pedir a luminosidade
  if (cmd[0] == 'g' && cmd[2] == 'l') {


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {
      //retorna ultima luminosidade recebida
      return "l " + std::to_string(id)  + " " +std::to_string(d->lastLux[id].back())  + "\n";
    }

  }//turns the system into decouplend mode
  if (cmd[0] == 'q') {


  return "q";

  }
  else if (cmd[0] == 'g' && cmd[2] == 'd') {//esta a pedir o duty cycle


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {
      //retorna ultimo dutycylce recebido
      return "d " + std::to_string(id)  + " " +std::to_string(d->lastDuty[id].back())  + "\n";
    }


  } else if (cmd[0] == 'g' && cmd[2] == 'o') {//esta a pedir ocupacao


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      //prints ocupation
      std::cout << "o " + std::to_string(id)  + " " +std::to_string(d->ocu[id]) << '\n';

    /*  sendmsg= "go";
      sendmsg+= id+1+'a';
        sendmsg+='\n';

      SerialSend(sendmsg);*/
      return "o " + std::to_string(id)  + " " +std::to_string(d->ocu[id]) + "\n";
    }

  }else if(cmd[0] == 's'){

  int id = atoi(&cmd[2]);
  int val = atoi(&cmd[4]);

  sendmsg= "s";
  sendmsg+= val+'a';
  sendmsg+=id+1+'a';
    sendmsg+='\n';

  SerialSend(sendmsg);


  } else if (cmd[0] == 'g' && cmd[2] == 'L') {//esta a pedir lower bound L


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      //prints ocupation
      std::cout << "L " + std::to_string(id)  + " " +std::to_string(d->lowLum[id])  + "\n";

      //sends byte array via serial
      /*sendmsg= "gL";
      sendmsg+= id+1+'a';
      sendmsg+='\n';

      SerialSend(sendmsg);*/
      return  "L " + std::to_string(id)  + " " +std::to_string(d->lowLum[id])  + "\n";
    }

  } else if (cmd[0] == 'g' && cmd[2] == 'O') {//esta a pedir external lumminance


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      //prints ocupation
      std::cout << "O " + std::to_string(id)  + " " +std::to_string(d->external[id])  + "\n";

      //sends byte array via serial
      //byte cmdserial[4]={'g','O',(byte)(id+1+'a'),'\n'};
    //  SerialSend((byte*)cmdserial);
      return "O " + std::to_string(id)  + " " +std::to_string(d->external[id])  + "\n";

    }

  }else if (cmd[0] == 'g' && cmd[2] == 'r') {//esta a pedir reference lumminance


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      //prints ocupation
      std::cout << "r " + std::to_string(id)  + " " +std::to_string(d->refLum[id])  + "\n";

      //sends byte array via serial


      return "r " + std::to_string(id)  + " " +std::to_string(d->refLum[id])  + "\n";;
    }

  } else if (cmd[0] == 'g' && cmd[2] == 'p' && cmd[4] == 'T') {//total power comsumption



    return "p T " + std::to_string(getSysPower(d))  + " Watts\n";

  } else if (cmd[0] == 'g' && cmd[2] == 'p') {//instataneous power consumption


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {
      return "p " + std::to_string(id)  + " " +std::to_string(d->lastDuty[id].back() / (float) 255)  + " Watts\n";
    }




  }else if (cmd[0] == 'g' && cmd[2] == 'e' && cmd[4] == 'T') {//total energy


    return "e T " + std::to_string(getSysEnergy(d))  + " Joules\n";


  }else if (cmd[0] == 'g' && cmd[2] == 'e') {//instataneous power consumption


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {


      return "e " + std::to_string(id)  + " " +std::to_string(d->energy[id])  + " Joules\n";
    }


  }

  else if (cmd[0] == 'g' && cmd[2] == 'c' && cmd[4] == 'T') {//current energy

    return "c T " + std::to_string(getSysConfErr(d))  + " lux\n";



  } else if (cmd[0] == 'g' && cmd[2] == 'c') {//instataneous power consumption


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      return "c " + std::to_string(id)  + " " +std::to_string(d->confErr[id]/d->N[id])  + " lux\n";
    }



  }else if (cmd[0] == 'g' && cmd[2] == 'v' && cmd[4] == 'T') {//current energy

    return "v T " + std::to_string(getSysConfVar(d))  + " lux/s^2\n";



  }else if (cmd[0] == 'g' && cmd[2] == 'v') {//instataneous power consumption


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      return "v " + std::to_string(id)  + " " +std::to_string(d->confVar[id]/d->Nl[id])  + " lux/s^2\n";
    }



  }else if (cmd[0] == 'g' && cmd[2] == 't' && cmd[4] == 'T') {//current energy



    std::chrono::duration<double,std::milli> elapsed(std::chrono::steady_clock::now()-d->start_time);



    return "t T " + std::to_string(NLUMS*(elapsed.count()/1000))  + " seconds\n";


  }else if (cmd[0] == 'g' && cmd[2] == 't') {//instataneous power consumption


    int id = atoi(&cmd[4]);

    if (id < NLUMS) {

      std::chrono::duration<double,std::milli> elapsed(std::chrono::steady_clock::now()-d->start_time);

      return "t " + std::to_string(id)  + " " + std::to_string((elapsed.count()/1000))  + " seconds\n";
    }



  }else if (cmd[0] == 'r') {//send system restart


    //send command
    SerialSend("rst\n");

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

      //PRINT DO TEMPO

/*
      std::cout << "PRINT DO TEMPO" << '\n';
      resume="";
      for (auto v : d->dutyTime[id])
      {

        std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();

        std::chrono::duration<double,std::milli> elapsed(end_time - v);

        //std::cout << v << "\n";
        std::string s = std::to_string(elapsed.count()/1000);
        resume+=s+", ";

      }
      resume+="\n";
      std::cout << resume << '\n';
*/




      //get elements of list
      return resume;


    }

  }else if (cmd[0] == 'b' && cmd[2] == 'l') {//last minute resume


    int id = atoi(&cmd[4]);
    //vai buscar a uma lista estes valores

    if (id < NLUMS) {



      std::string resume="b l " + std::to_string(id)+" ";
std::cout << "Tam Luxs: "<< d->lastLux[id].size() << '\n';
      std::cout << "PRINT DA LISTA" << '\n';
      for (auto v : d->lastLux[id])
      {
        //std::cout << v << "\n";
        resume+=std::to_string(v)+", ";


      }
      resume+="\n";
      std::cout << resume << '\n';
      //get elements of list

std::cout << "Tam Luxs: "<< d->lastLux[id].size() << '\n';

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

  else if (cmd[0] == 'z' && cmd[1] == '2') {//last minute resume

    std::cout << "VALOR NOVO PARA STREAMAR" << '\n';
    d->streamflagDuty[0] = 1;

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


  void timerLoop(DSys*d)
  {

    /*FOR DEBUGGING PURPOSES*/
    tim.expires_from_now(std::chrono::milliseconds(1000));
    tim.async_wait([this,d](boost::system::error_code ec) {

    //  std::cout << "LOOOOP" << '\n';

      for (int i = 0; i < NLUMS; i++) {

        //TAMBEM E PRECISO VERIFICAR SE STREAM ESTA ATIVO ATUALMENTE

        //check if there is new lux data
        if(d->streamflagLux[i]==1 && d->streamLux[i]==1)
        {

          //obtem tempo
          std::chrono::duration<double,std::milli> elapsed(d->luxTime[i].back()-d->start_time);

          std::string sendmsg="c l ";
          sendmsg+=std::to_string(i);
          sendmsg+=" ";
          sendmsg+=std::to_string(d->lastLux[i].back());
          sendmsg+=" ";
          sendmsg+=std::to_string(elapsed.count());
          sendmsg+="\n";

          //gets time interval


          boost::asio::async_write(sock_,boost::asio::buffer(sendmsg),
          [d](const boost::system::error_code &ec, std::size_t length){
            std::cout << "MANDA LUX" << '\n';
          });

          d->streamflagLux[i]=0;
        }

        //check if there is new duty data
        if(d->streamflagDuty[i]==1 && d->streamDuty[i]==1)
        {

          //obtem tempo
          std::chrono::duration<double,std::milli> elapsed(d->dutyTime[i].back()-d->start_time);


          std::string sendmsg="c d ";
          sendmsg+=std::to_string(i);
          sendmsg+=" ";
          sendmsg+=std::to_string(d->lastDuty[i].back());
          sendmsg+=" ";
          sendmsg+=std::to_string(elapsed.count());
          sendmsg+="\n";
          //faz  push do novo valor
          boost::asio::async_write(sock_,boost::asio::buffer(sendmsg),
          [d](const boost::system::error_code &ec, std::size_t length){
            std::cout << "MANDA DUTY" << '\n';
          });

          d->streamflagDuty[i]=0;
        }


        /*
        int flagExternal[NLUMS]={ 0 };
        int flagLowLum[NLUMS]={ 0 };
        int flagRefLum[NLUMS]={ 0 };
        int flagOcu[NLUMS]={ 0 };*/

        /*
        int ocu[NLUMS];
        float lowLum[NLUMS];
        float external[NLUMS];
        float refLum[NLUMS];*/

        /*
        std::string auxStr="";

        if(d->flagExternal[i]==1)
        {
          auxStr="O "+ std::to_string(i)+" "+ std::to_string(d->external[i])  +"\n";
          //faz  push do novo valor
          boost::asio::async_write(sock_,boost::asio::buffer(auxStr),
          [d](const boost::system::error_code &ec, std::size_t length){
            std::cout << "MANDA EXTERNAL" << '\n';
          });

          d->flagExternal[i]=0;
        }

        if(d->flagLowLum[i]==1)
        {
          auxStr="L "+ std::to_string(i)+" "+ std::to_string(d->lowLum[i])  +"\n";

          //faz  push do novo valor
          boost::asio::async_write(sock_,boost::asio::buffer(auxStr),
          [d](const boost::system::error_code &ec, std::size_t length){
            std::cout << "MANDA LOWLUM" << '\n';
          });

          d->flagLowLum[i]=0;
        }

        if(d->flagRefLum[i]==1)
        {


          auxStr="r "+ std::to_string(i)+" "+ std::to_string(d->refLum[i])  +"\n";
          //faz  push do novo valor
          boost::asio::async_write(sock_,boost::asio::buffer(auxStr),
          [d](const boost::system::error_code &ec, std::size_t length){
            std::cout << "MANDA REFLUM" << '\n';
          });

          d->flagRefLum[i]=0;
        }
std::cout << d->flagOcu[i] << '\n';

        if(d->flagOcu[i]==1)
        {


          auxStr="o "+ std::to_string(i)+" "+ std::to_string(d->ocu[i])  +"\n";
          //faz  push do novo valor
          boost::asio::async_write(sock_,boost::asio::buffer(auxStr),
          [d](const boost::system::error_code &ec, std::size_t length){
            std::cout << "MANDA OCUPANCIA" << '\n';
          });

          d->flagOcu[i]=0;
        }
*/
      }


      timerLoop(d);
      //	std::cout << "Sent! Now closing connection" << std::endl;
    });

  }


  void loop(DSys *d)
  {
    auto self = shared_from_this();

    sock_.async_read_some(boost::asio::buffer(data_, max_length),
    [this,self,d](const boost::system::error_code &ec, std::size_t length){


      //std::cout << "Message Read in Server: "<<data_ << "\n";
      std::unique_lock<std::mutex> lk(mut);
      while (d->iswriting==true)   cvstream.wait(lk);
      //check if command is valid

      d->isreading=true;
      msg_ =CheckCommand(data_,d);

      d->isreading=false;
      cvi2c.notify_one();
      lk.unlock();
      //std::cout << "Message From CHECK CMD: "<<msg_ << "\n";


      if(msg_.empty()){

        boost::asio::async_write(sock_,boost::asio::buffer("Keep Alive\n"),
        [this,self,d](const boost::system::error_code &ec, std::size_t length){

          if(!ec)
          loop(d);
          else
        {std::cout << "Client Disconnected" << '\n';
        d->lums2.close();
        d->lums1.close();

        d->duts1.close();
        d->duts2.close();
}
        });
      }
      else
      {
        std::cout << msg_ << "Good Cmd\n";

        boost::asio::async_write(sock_,boost::asio::buffer(msg_),
        [this,self,d](const boost::system::error_code &ec, std::size_t length){

          if(!ec)
          loop(d);
          else{
            d->lums2.close();
            d->lums1.close();

            d->duts1.close();
            d->duts2.close();

              std::cout << "Client Disconnected" << '\n';
          }



        });

      }


    });
  }


  void start(DSys* d) {
    auto self = shared_from_this();
    async_write(sock_, buffer("Hello World\n"),
    [this, self,d](boost::system::error_code ec, std::size_t length) {




      std::cout << "Client Connected" << '\n';
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
      new_conn->timerLoop(d);
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

    dsystem.lums1.open("lums1.csv");
      dsystem.lums2.open("lums2.csv");

      dsystem.duts1.open("duts1.csv");
        dsystem.duts2.open("duts2.csv");
    if(sflag)
    {
      sp.open(serialPort);    //connect to port
      sp.set_option(serial_port_base::baud_rate(115200));
    }


    tcp_server server(io,&dsystem);

std::cout << "Starting..." << '\n';
    std::thread io_thrd (ioStuff);     // spawn new thread that calls foo()
    #ifdef __arm__

        std::thread i2c_thrd (i2cStuff,&dsystem);  // spawn new thread that calls bar(0)
        i2c_thrd.join();               // pauses until second finishes

    #endif




    io_thrd.join();                // pauses until first finishes




  } catch(std::exception &e) {std::cout << e.what();}


  return 0;
}
