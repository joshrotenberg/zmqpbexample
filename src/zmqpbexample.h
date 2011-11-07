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
