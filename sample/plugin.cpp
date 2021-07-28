#include <iostream>
#include <libguile.h>

void onStart() asm ("onStart");
void onStart() { 
  std::cout << "hello world 2 from on start!" << std::endl;
}

void onFrame() asm ("onFrame");
void onFrame() { 
  std::cout << "hello world 2 from on start!" << std::endl;
}

SCM scmHelloWorld(){
  std::cout << "hello world" << std::endl;
  return SCM_UNSPECIFIED;
}

void registerGuileFns() asm ("registerGuileFns");
void registerGuileFns() { 
  scm_c_define_gsubr("hello-world", 0, 0, 0, (void *)scmHelloWorld);
}

#ifdef BINARY_MODE

int main(){
  scmHelloWorld();
}

#endif