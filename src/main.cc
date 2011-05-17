#include "zmqpbexample.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include <iostream>

void simple();
void rpc();

// our generic rpc service function.
bool service(zmq::socket_t* socket,
	     const std::string& method, 
	     const google::protobuf::Message& request,
	     google::protobuf::Message* response);

int main(int argc, char** argv) 
{
  
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // run the simple example. this uses a single protobuf messages as a request
  // and returns a simple protobuf message in the response, copying the data
  // from the request to the response.
  simple();

  rpc();

  return 0;
}

static void* start_simple(void* arg) {
  static_cast<zmqpbexample*>(arg)->run_simple();
}

void simple()
{
  std::string simple_endpoint = "tcp://127.0.0.1:5555";
  
  zmqpbexample z(simple_endpoint);

  // run the server in a thread
  pthread_t thread;
  pthread_create(&thread, NULL, start_simple,  &z);
  
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REQ);

  socket.connect (simple_endpoint.c_str());
    
  for (int request_nbr = 0; request_nbr != 5; request_nbr++) {
    
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
    
    assert(pb_response.response_number() == pb_request.request_number());
    assert(pb_response.response_string() == pb_request.request_string());
    std::cout << "client: Received " << pb_response.response_number() << ": "
	      << pb_response.response_string() << std::endl;
    
  }

}

static void* start_rpc(void* arg) {
  static_cast<zmqpbexample*>(arg)->run_rpc();
}

void rpc() 
{
  std::string rpc_endpoint = "tcp://127.0.0.1:5556";
  
  zmqpbexample z(rpc_endpoint);
  
  pthread_t thread;
  pthread_create(&thread, NULL, start_rpc,  &z);

  // the caller would probably be better off as a class and would handle the
  // connection stuff itself, but for brevity ...
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REQ);
  socket.connect (rpc_endpoint.c_str());
  
  // create an add request 
  RPCAddRequest add_request;
  add_request.set_term1(4);
  add_request.set_term2(6);
  
  // now call the service with our request and response that will be 
  // populated by our 'service' handler 
  RPCAddResponse add_response;    
  if(!service(&socket, "add", add_request, &add_response)) {
    // handle errors
  }
  assert(add_response.sum() == 10);

  // create a reverse request
  RPCReverseRequest reverse_request;
  reverse_request.set_to_reverse("reverse me");
  
  RPCReverseResponse reverse_response;
  if(!service(&socket, "reverse", reverse_request, &reverse_response)) {
    // handle errors
  }
  assert(reverse_response.reversed() == "em esrever");
  
}

bool service(zmq::socket_t* socket, 
	     const std::string& method, 
	     const google::protobuf::Message& request,
	     google::protobuf::Message* response) {

  // we've got three levels of requests here:
  // 1) the specifc rpc call message (RPCAddRequest or RPCReverseRequest)
  // 2) the rpc wrapper to transport the call
  // 3) the zmq::message_t to hand it over the wire.

  ZmqPBExampleRPCRequest rpc_request;
  ZmqPBExampleRPCResponse rpc_response;

  // not really doing anything with the 'service' param, but in general it 
  // might indicate a higher level namespace that the function belongs to
  rpc_request.set_service("example"); 

  // set the method to call on the server side
  rpc_request.set_method(method);
  
  // now serialize our call specific message
  std::string request_serialized;
  request.SerializeToString(&request_serialized);

  // and add it to the rpc wrapper
  rpc_request.set_protobuf(request_serialized);

  // now serialize the wrapper and send it over
  std::string wrapper_serialized;
  rpc_request.SerializeToString(&wrapper_serialized);

  // send the request
  zmq::message_t zmq_request (wrapper_serialized.size());
  memcpy ((void *) zmq_request.data (), wrapper_serialized.c_str(), 
	  wrapper_serialized.size());
  socket->send (zmq_request);

  // get the reply
  zmq::message_t reply;
  socket->recv (&reply);

  // parse the rpc wrapper
  rpc_response.ParseFromArray(reply.data(), reply.size());

  if(rpc_response.has_error() ) {
    // something went wrong
    return false;
  }

  // otherwise parse the response and return true
  response->ParseFromString(rpc_response.protobuf());
  return true;
}
