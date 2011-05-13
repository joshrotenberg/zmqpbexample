#ifndef __ZMQPBEXAMPLE_H__
#define __ZMQPBEXAMPLE_H__

#include <zmq.hpp>
#include "zmqpbexample.pb.h"

class zmqpbexample;

class zmqpbexample {

 public:
  explicit zmqpbexample();
  virtual ~zmqpbexample();

  void run();

 private:

};



#endif
