#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>

using namespace std::chrono;



int main()
{

   std::chrono::steady_clock::time_point start_time =  std::chrono::steady_clock::now();


  usleep(1000000);

 std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
   duration<double,std::milli> elapsed(end_time - start_time);
  std::cout << "Elapsed Time in Main Thread # "   << elapsed.count() << std::endl;

}
