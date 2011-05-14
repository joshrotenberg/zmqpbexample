#ifndef __ZMQPBEXAMPLE_H__
#define __ZMQPBEXAMPLE_H__

#include <zmq.hpp>
#include "zmqpbexample.pb.h"

class zmqpbexample;

class zmqpbexample {

 public:
  explicit zmqpbexample(const std::string& endpoint);
  virtual ~zmqpbexample();

  // runs the base server
  void run();

 private:
  std::string endpoint_;
};



#endif
