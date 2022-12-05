CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++14
# CXXFLAGS += -I/home/cis5050/grpc/third_party/protobuf/src
# CXXFLAGS += -I/home/cis5050/grpc/include
# CXXFLAGS += -I/home/cis5050/grpc/third_party/abseil-cpp
# CXXFLAGS += -I/home/cis5050/grpc/build/libs/opt/pkgconfig
# CXXFLAGS += -I/home/cis5050/grpc/build/lib/pkgconfig

LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl\

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ../protos

vpath %.proto $(PROTOS_PATH)

all: server client_example

server: kv_store.pb.o kv_store.grpc.pb.o kv_store_server.o
	$(CXX) $^ $(LDFLAGS) -o $@

client_example: kv_store.pb.o kv_store.grpc.pb.o kvstore_client_example.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

proto:
	protoc -I ../protos --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../protos/kv_store.proto
	protoc -I ../protos --cpp_out=. ../protos/kv_store.proto

clean:
	rm -f *.o *.pb.cc *.pb.h server client_example