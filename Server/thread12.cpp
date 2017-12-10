// thread example
#include <iostream>       // std::cout
#include <thread>         // std::thread

void foo()
{
  // do stuff...

  while (1) {
    std::cout << "AAAAAAAAAAAAAAAAAAAAAAA" << '\n';
    std::this_thread::yield();
  }
}

void bar(int x)
{
  while (1) {
    std::cout << "BBB" << '\n';
    std::this_thread::yield();
  }
}

int main()
{
  std::thread first (foo);     // spawn new thread that calls foo()
  std::thread second (bar,0);  // spawn new thread that calls bar(0)

  std::cout << "main, foo and bar now execute concurrently...\n";

  // synchronize threads:
  first.join();                // pauses until first finishes
  second.join();               // pauses until second finishes

  std::cout << "foo and bar completed.\n";

  return 0;
}
