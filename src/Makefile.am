AM_CXXFLAGS = -g $(PROTOBUF_CFLAGS) $(ZMQ_CFLAGS) 

bin_PROGRAMS = zmqpbexample
zmqpbexample_SOURCES = zmqpbexample.cc
zmqpbexample_LDADD = $(PROTOBUF_LIBS) $(ZMQ_LIBS)

zmqpbexample.pb.cc: ../protobuf/zmqpbexample.proto
	${PROTOC} --cpp_out=. --proto_path=../protobuf \
	../protobuf/zmqpbexample.proto
