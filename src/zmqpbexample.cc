#include "zmqpbexample.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include <iostream>

zmqpbexample::zmqpbexample(const std::string& endpoint) 
  : endpoint_(endpoint) {

}

zmqpbexample::~zmqpbexample() {

}

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

void zmqpbexample::run_rpc() {

  zmq::context_t context (1);
  zmq::socket_t socket (context, ZMQ_REP);

  socket.bind (endpoint_.c_str());
  while (true) {

    zmq::message_t zmq_request;
    ZmqPBExampleRPCRequest rpc_request;

    socket.recv (&zmq_request);
    rpc_request.ParseFromArray(zmq_request.data(), zmq_request.size());

    ZmqPBExampleRPCResponse rpc_response;        
    std::string serialized_response;
    rpc_handler(rpc_request.service(), rpc_request.method(), 
		rpc_request.protobuf(), &serialized_response);
     
    rpc_response.set_protobuf(serialized_response);

    std::string rpc_response_serialized;
    rpc_response.SerializeToString(&rpc_response_serialized);

    zmq::message_t reply (rpc_response_serialized.size());
    memcpy ((void *) reply.data (), rpc_response_serialized.c_str(), 
	    rpc_response_serialized.size());
    socket.send (reply);
  }
}

void zmqpbexample::rpc_handler(const std::string& service,
			       const std::string& method,
			       const std::string& serialized_request,
			       std::string* serialized_response) {
  
  // in real life, this would probably dispatch registered callbacks to handle 
  // different methods or something fancy like that.


  if(method.compare("add") == 0) {
    google::protobuf::Message *request = new RPCAddRequest;
    RPCAddResponse response;

    // get the descriptor
    const google::protobuf::Descriptor* descriptor = request->GetDescriptor();

    // verify thisour fields
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
    response.set_sum(sum);
    response.SerializeToString(serialized_response);
    /*
    try {
      google::protobuf::DynamicMessageFactory factory;
      
    std::cout << rpc_request.method().compare("add") << std::endl;
    const google::protobuf::Message* prototype_msg =
      factory.GetPrototype(descriptor);
    google::protobuf::Message* mutable_msg = prototype_msg->New();
    
    mutable_msg->ParseFromString(rpc_request.protobuf());
    request.CheckTypeAndMergeFrom(*mutable_msg);
    
    std::cerr << request.GetTypeName() << std::endl;

    delete mutable_msg;
    
    } catch(...) {

    }
    */
  }
  else if(method.compare("reverse") == 0) {
    
  }
  
}
