// list::size
#include <iostream>


#define LISTSIZE 6


std::list<int> InsertDuty(std::list<int> list, int value)
{
list.push_back (value);

if(list.size()>LISTSIZE)
list.pop_front();

return list;
}

int main ()
{

  std::list<int> myints;


myints = InsertDuty(myints,1);
myints = InsertDuty(myints,2);
myints = InsertDuty(myints,3);
myints = InsertDuty(myints,4);
myints = InsertDuty(myints,5);
myints = InsertDuty(myints,6);
myints = InsertDuty(myints,7);
myints = InsertDuty(myints,8);
myints = InsertDuty(myints,9);

  //for (int i=0; i<10; i++) myints.push_back(i);
  for (auto v : myints)
        std::cout << v << "\n";
  std::cout << "1. size: " << myints.size() << '\n';
  std::cout << "1. PRIMEIRO: " << myints.back()<< '\n';
  return 0;
}
