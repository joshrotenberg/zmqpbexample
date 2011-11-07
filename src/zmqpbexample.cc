#include "zmqpbexample.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>

#include <string>
#include <algorithm>
#include <iostream>

using namespace google::protobuf::io;

zmqpbexample::zmqpbexample(const std::string& endpoint) 
  : endpoint_(endpoint) {
}

zmqpbexample::zmqpbexample(const std::string& frontend,
                           const std::string& backend) 
    : frontend_(frontend), backend_(backend) {
}


zmqpbexample::~zmqpbexample() {
}

// this is a simple case. we have a known request and response type. works well
// but isn't particularly flexible if we want to start supporting multiple
// message types over the same req/rep socket.
void zmqpbexample::run_simple() {

  //  Prepare our context and socket
  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);
  socket.bind (endpoint_.c_str());

  while (true) {
    zmq::message_t request;
    ZmqPBExampleRequest pb_request;

    // receive the request, and parse the protocol buffer from it
    socket.recv (&request);
    pb_request.ParseFromArray(request.data(), request.size());
    
    std::cout << "server: Receivedd " << pb_request.request_number() << ": " <<
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

// a simple rpc server. this builds on the previous example by using a 
// wrapper protobuf, so that as long as our remote procedures know what type to
// expect and return, we can wrap up any message and function name and hand 
// them over the wire.
void zmqpbexample::run_rpc() {

  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);

  socket.bind (endpoint_.c_str());
  while (true) {

    // receive and parse the rpc wrapper
    zmq::message_t zmq_request;
    ZmqPBExampleRPCRequest rpc_request;
    socket.recv (&zmq_request);
    rpc_request.ParseFromArray(zmq_request.data(), zmq_request.size());
    
    // now hand off the contents to our handler. we'll also pass in a 
    // string which will get the serialized, method specific response
    std::string serialized_response;
    rpc_handler(rpc_request.service(), rpc_request.method(), 
		rpc_request.protobuf(), &serialized_response);

    // now set up the response wrapper
    ZmqPBExampleRPCResponse rpc_response;        
    rpc_response.set_protobuf(serialized_response);

    // serialize the wrapper
    std::string rpc_response_serialized;
    rpc_response.SerializeToString(&rpc_response_serialized);

    // and respond to the client
    zmq::message_t reply (rpc_response_serialized.size());
    memcpy ((void *) reply.data (), rpc_response_serialized.c_str(), 
	    rpc_response_serialized.size());
    socket.send (reply);
  }
}

  
// in real life, this would probably dispatch registered callbacks to handle 
// different methods or something fancy like that, but for simplicity,
// this one handler knows how to run our two functions.
void zmqpbexample::rpc_handler(const std::string& service,
			       const std::string& method,
			       const std::string& serialized_request,
			       std::string* serialized_response) {

  if(method.compare("add") == 0) {
    // since we know the type of the request here, we don't have to
    // use the reflection api, but just as an example ...
    google::protobuf::Message *request = new RPCAddRequest;


    // get the descriptor
    const google::protobuf::Descriptor* descriptor = request->GetDescriptor();

    // find the fields
    const google::protobuf::FieldDescriptor* term1_field = 
      descriptor->FindFieldByName("term1");
    const google::protobuf::FieldDescriptor* term2_field = 
      descriptor->FindFieldByName("term2");

    // now parse 
    request->ParseFromString(serialized_request);
    const google::protobuf::Reflection* reflection = request->GetReflection();

    // perform our actual rpc method function
    uint32_t sum = reflection->GetUInt32(*request, term1_field) +
      reflection->GetUInt32(*request, term2_field);
    
    // set up the response
    RPCAddResponse response;
    response.set_sum(sum);
    response.SerializeToString(serialized_response);
  }
  else if(method.compare("reverse") == 0) {

    RPCReverseRequest request;

    request.ParseFromString(serialized_request);

    std::string to_reverse = request.to_reverse();
    reverse(to_reverse.begin(), to_reverse.end());

    RPCReverseResponse response;
    response.set_reversed(to_reverse);
    response.SerializeToString(serialized_response);
  }
}

// this is like the wuserver example, only instead of sending out a single
// update per message, we send out one or more. this example is somewhat 
// contrived, but other applications may want the ability to, say, send
// out a 'group' of items in a single 'publish', perhaps three or four
// separate protobuf types that are related. zeromq multipart is another 
// option here as well, as is a single wrapper protobuf messages with a
// repeated field of embedded messages.

#define within(num) (int) ((float) num * random () / (RAND_MAX + 1.0))
void zmqpbexample::run_weather() {
  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  
  publisher.bind(endpoint_.c_str());
  srandom ((unsigned) time (NULL));
  while(1) {
    
    std::string encoded_message;
    ZeroCopyOutputStream* raw_output = 
      new StringOutputStream(&encoded_message);
    CodedOutputStream* coded_output = new CodedOutputStream(raw_output);
    
    int num_updates = within(9) + 1;

    // prefix the stream with the number of updates
    coded_output->WriteLittleEndian32(num_updates);
    int update_nbr;

    // create a bunch of updates, serialize them and add them
    // to the stream
    for(update_nbr = 0; update_nbr < num_updates; update_nbr++) {
      
      ZmqPBExampleWeather update;
      update.set_zipcode( within (100000));
      update.set_temperature(within (110));
      update.set_relhumidity(within (50) + 10);

      std::string serialized_update;
      update.SerializeToString(&serialized_update);

      coded_output->WriteVarint32(serialized_update.size());
      coded_output->WriteString(serialized_update);
    }
    // clean up
    delete coded_output;
    delete raw_output;

    zmq::message_t message(encoded_message.size());
    memcpy ((void *) message.data(), encoded_message.c_str(), 
	    encoded_message.size());

    publisher.send(message);
  }
}

void zmqpbexample::run_worker() {

  zmq::context_t context (1);
  zmq::socket_t responder(context, ZMQ_REP);

  responder.connect(backend_.c_str());

  while(true) {
    zmq::message_t request;
    responder.recv(&request);
        
    std::string string(static_cast<char*>(request.data()), request.size());
    
    std::cout << "Received request: " << string << std::endl;
    //sleep(1);
    zmq::message_t reply(string.size());
    memcpy(reply.data(), string.data(), string.size());
    responder.send(reply);
  }
  
}

void zmqpbexample::run_broker() {

  zmq::context_t context (1);
  zmq::socket_t frontend(context, ZMQ_XREP);
  zmq::socket_t backend(context, ZMQ_XREQ);
  
  frontend.bind(frontend_.c_str());
  backend.bind(backend_.c_str());

  zmq::pollitem_t items [] = {
    { frontend, 0, ZMQ_POLLIN, 0 },
    { backend,  0, ZMQ_POLLIN, 0 }
  };

  while(true) {
    zmq::message_t message;
    int64_t more;

    zmq::poll(&items[0], 2, -1);

    if(items [0].revents & ZMQ_POLLIN) {
      while(true) {
        frontend.recv(&message);
        size_t more_size = sizeof(more);
        frontend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        backend.send(message, more? ZMQ_SNDMORE: 0);

        if(!more)
          break;
      }
    }
    if(items [1].revents & ZMQ_POLLIN) {
      while(true) {
        backend.recv(&message);
        size_t more_size = sizeof(more);
        backend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        frontend.send(message, more? ZMQ_SNDMORE: 0);

        if(!more)
          break;
      }
    }
    
  }
}
