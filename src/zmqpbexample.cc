#include "zmqpbexample.h"

#include <iostream>

zmqpbexample::zmqpbexample() {

}

zmqpbexample::~zmqpbexample() {

}

void zmqpbexample::run() {

  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind ("tcp://*:5555");

  while (true) {
    zmq::message_t request;
    ZmqPBExampleRequest pb_request;

    // receive the request, and parse the protocol buffer from it
    socket.recv (&request);
    pb_request.ParseFromArray(request.data(), request.size());
    std::cout << "server: Received " << pb_request.request_number() << ": " <<
      pb_request.request_string() << std::endl;

    // do something interesting with the request here
    // ...

    // create a response. we just echo the request
    ZmqPBExampleResponse pb_response;
    pb_response.set_response_string(pb_request.request_string());
    pb_response.set_response_number(pb_request.request_number());
    std::string pb_serialized;
    pb_response.SerializeToString(&pb_serialized);

    //  create the reply
    zmq::message_t reply (pb_serialized.size());
    memcpy ((void *) reply.data (), pb_serialized.c_str(), 
	    pb_serialized.size());
    socket.send (reply);
  }
  
}
