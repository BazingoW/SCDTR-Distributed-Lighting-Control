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




int main()
{
int d1=37;
int d2=200;
std::string s1 = std::to_string(d1);
s1+="." +std::to_string(d2);



float value = ::atof(s1.c_str());
std::cout << value << '\n';
}
