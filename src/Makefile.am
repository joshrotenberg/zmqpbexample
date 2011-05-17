AM_CXXFLAGS = -g $(PROTOBUF_CFLAGS) $(ZMQ_CFLAGS) 

lib_LTLIBRARIES = libzmqpbexample.la
libzmqpbexample_la_LIBADD = $(PROTOBUF_LIBS) $(ZMQ_LIBS)
include_HEADERS = zmqpbexample.h zmqpbexample.pb.h

libzmqpbexample_la_LDFLAGS = -version-info 1:0:0 -export-dynamic -no-undefined -fPIC
libzmqpbexample_la_SOURCES = zmqpbexample.pb.cc zmqpbexample.cc

zmqpbexample.pb.cc: ../protobuf/zmqpbexample.proto
	${PROTOC} --cpp_out=. --proto_path=../protobuf ../protobuf/zmqpbexample.proto

bin_PROGRAMS = zmqpbexample
zmqpbexample_SOURCES = main.cc
zmqpbexample_LDADD = libzmqpbexample.la

