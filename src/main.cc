#include "zmqpbexample.h"

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <iostream>

int main(int argc, char** argv) 
{
  
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // run the server
  zmqpbexample z;
  pid_t pid;

  pid = fork();

  std::cerr << pid << std::endl;
  if(pid == 0) {
    try {
      z.run();
    } catch(...) {
      std::cerr << "woops!\n";
      return 1;
    }
  } else {

    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect ("tcp://localhost:5555");

    //  Do 10 requests, waiting each time for a response
    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
      zmq::message_t request (6);
      memcpy ((void *) request.data (), "Hello", 5);
      std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
      socket.send (request);

      //  Get the reply.
      zmq::message_t reply;
      socket.recv (&reply);
      std::cout << "Received World " << request_nbr << std::endl;
    }
    // kill the server
    kill(pid, SIGTERM);
  }
  return 0;
}
