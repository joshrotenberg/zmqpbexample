#ifndef __ZMQPBEXAMPLE_H__
#define __ZMQPBEXAMPLE_H__

#include <zmq.hpp>
#include "zmqpbexample.pb.h"

class zmqpbexample;

class zmqpbexample {

 public:
  explicit zmqpbexample(const std::string& endpoint);
  explicit zmqpbexample(const std::string& frontend,
                        const std::string& backend);
  virtual ~zmqpbexample();

  void run_simple();
  void run_rpc();
  void run_weather();
  void run_worker();
  void run_broker();

  // from zhelpers.cpp
  static bool s_send (zmq::socket_t & socket, const std::string & string) {
    
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());
    
    bool rc = socket.send(message);
    return (rc);
  }
  
  static bool s_sendmore (zmq::socket_t & socket, const std::string & string) {
  
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());
    
    bool rc = socket.send(message, ZMQ_SNDMORE);
    return (rc);
  }

  static std::string s_recv (zmq::socket_t & socket) {
    
    zmq::message_t message;
    socket.recv(&message);
    
    return std::string(static_cast<char*>(message.data()), message.size());
  }
  
 private:

  std::string endpoint_;
  std::string frontend_;
  std::string backend_;
  void rpc_handler(const std::string& service,
		   const std::string& method,
		   const std::string& serialized_request,
		   std::string* serialized_response);
  

};



#endif
