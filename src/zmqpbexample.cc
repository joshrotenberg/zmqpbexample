#include "zmqpbexample.h"

#include <iostream>

zmqpbexample::zmqpbexample() {

}

zmqpbexample::~zmqpbexample() {

}

void zmqpbexample::run() {
  
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REP);
  socket.bind("tcp://*:5555");
    
  while(true) {
    zmq::message_t request;
    socket.recv(&request);
    std::cerr << "received " << (char *)request.data();

    zmq::message_t reply(2);
    memcpy((void *) reply.data(), "OK", 2);
    socket.send(reply);
  }
  
}
