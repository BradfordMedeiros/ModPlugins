
#include <iostream>

void onStart() asm ("onStart");
void onStart() { 
  std::cout << "hello world 2 from on start!" << std::endl;
}

void onFrame() asm ("onFrame");
void onFrame() { 
  std::cout << "hello world 2 from on start!" << std::endl;
}