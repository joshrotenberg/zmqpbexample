#include "zmqpbexample.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>

#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include <iostream>

using namespace google::protobuf::io;

void simple();
void rpc();
void weather();
void workers();

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

  // a slightly more complex example that implements a barebones rpc server,
  // using a single wrapper request/response protobuf that can accomodate 
  // any type of method specifc payload as an embedded message
  rpc();

  // a version of the weather update pub/sub example that uses protocol buffers
  // and the underlying coded stream api to publish updates with 1 or more 
  // messages at a time
  weather();

  //
  workers();
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
    std::cout << "foo bar!\n";
  }
  assert(reverse_response.reversed() == "em esrever");
  
}

// the client's 'service' handler takes care of the wrapping and unwrapping
// of the rpc request/responses, but doesn't need to know anything about
// the underlying operation of the specific method.
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

static void* start_weather(void* arg) {
  static_cast<zmqpbexample*>(arg)->run_weather();
}

void weather() 
{
  std::string weather_endpoint = "tcp://127.0.0.1:5557";

  zmqpbexample z(weather_endpoint);
  
  pthread_t thread;
  pthread_create(&thread, NULL, start_weather,  &z);

  zmq::context_t context (1);
  zmq::socket_t subscriber (context, ZMQ_SUB);
  subscriber.connect(weather_endpoint.c_str());

  subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0);

  int update_nbr;
  for (update_nbr = 0; update_nbr < 10; update_nbr++) {
    zmq::message_t update;
    subscriber.recv(&update);

    // use protocol buffer's io stuff to package up a bunch of stuff into 
    // one stream of serialized messages. the first part of the stream 
    // is the number of messages. next, each message is preceeded by its
    // size in bytes.
    ZeroCopyInputStream* raw_input = 
      new ArrayInputStream(update.data(), update.size());
    CodedInputStream* coded_input = new CodedInputStream(raw_input);

    // find out how many updates are in this message
    uint32_t num_updates;
    coded_input->ReadLittleEndian32(&num_updates);
    std::cout << "received update " << update_nbr << std::endl;

    // now for each message in the stream, find the size and parse it ...
    for(int i = 0; i < num_updates; i++) {

      std::cout << "\titem: " << i << std::endl;

      std::string serialized_update;
      uint32_t serialized_size;
      ZmqPBExampleWeather weather_update;
      
      // the message size
      coded_input->ReadVarint32(&serialized_size);
      // the serialized message data
      coded_input->ReadString(&serialized_update, serialized_size);
      
      // parse it
      weather_update.ParseFromString(serialized_update);
      std::cout << "\t\tzip: " << weather_update.zipcode() << std::endl;
      std::cout << "\t\ttemperature: " << weather_update.temperature() << 
	std::endl;
      std::cout << "\t\trelative humidity: " << 
	weather_update.relhumidity() << std::endl;
    }

    delete coded_input;
    delete raw_input;
  }
  
}

static void* start_worker(void* arg) {
  static_cast<zmqpbexample*>(arg)->run_worker();
}

static void* start_broker(void* arg) {
  static_cast<zmqpbexample*>(arg)->run_broker();
}

void workers()
{

  std::string worker_frontend = "tcp://127.0.0.1:5559";
  std::string worker_backend = "tcp://127.0.0.1:5560";

  zmqpbexample z(worker_frontend, worker_backend);
  int nthreads = 3;

  for(int j = 0; j < nthreads; j++) {
    pthread_t worker_thread;
    pthread_create(&worker_thread, NULL, start_worker, &z);
  }

  pthread_t broker_thread;
  pthread_create(&broker_thread, NULL, start_broker, &z);

  zmq::context_t context (1);
  zmq::socket_t requester (context, ZMQ_REQ);

  requester.connect(worker_frontend.c_str());
  for( int j = 0; j < 10; j++) {

    // create a new workload
    ZmqPBExampleWorkerRequest worker_request;
    worker_request.set_string_in("reverse me");

    std::string pb_serialized;
    worker_request.SerializeToString(&pb_serialized);

    // send the job type string
    zmqpbexample::s_sendmore(requester, "one");
    
    // create and send the zmq message
    zmq::message_t request (pb_serialized.size());
    memcpy ((void *) request.data (), pb_serialized.c_str(), 
	    pb_serialized.size());
    
    requester.send (request);

    std::string response_string = zmqpbexample::s_recv(requester);
    //std::cout << "Received reply " << response_string << std::endl;
  }
  
}
