#include "zmqpbexample.h"

#include <iostream>

int main(int argc, char** argv) 
{
  
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  zmqpbexample z;
  try {
    z.run();
  } catch(...) {
    std::cerr << "woops!\n";
    return 1;
  }

  return 0;
}
