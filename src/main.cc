#include "zmqpbexample.h"

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <iostream>

int main(int argc, char** argv) 
{
  
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  zmqpbexample z;
  pid_t pid;

  // we fork and run the server in the child process, and kill it once
  // we are done sending a bunch of requests and handling the responses
  pid = fork();

  if(pid == 0) {
    z.run();
  } else {

    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    socket.connect ("tcp://localhost:5555");

    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {

      // set up the request protobuf
      ZmqPBExampleRequest pb_request;
      pb_request.set_request_string("foo");
      pb_request.set_request_number(request_nbr);

      // serialize the request to a string
      std::string pb_serialized;
      pb_request.SerializeToString(&pb_serialized);

      // create and send the zmq message
      zmq::message_t request (pb_serialized.size());
      memcpy ((void *) request.data (), pb_serialized.c_str(), 
	      pb_serialized.size());

      std::cout << "client: Sending request " << request_nbr << "..." << 
	std::endl;
      socket.send (request);

      //  create and receive the reply
      zmq::message_t reply;
      socket.recv (&reply);

      // get the response object and parse it
      ZmqPBExampleResponse pb_response;
      pb_response.ParseFromArray(reply.data(), reply.size());
      std::cout << "client: Received " << pb_response.response_number() << ": "
		<< pb_response.response_string() << std::endl;
    }
    // kill the server
    kill(pid, SIGTERM);
  }
  return 0;
}
