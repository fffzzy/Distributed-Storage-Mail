proto:
	protoc -I ../Protos --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../Protos/kvstore.proto
	protoc -I ../Protos --cpp_out=. ../Protos/kvstore.proto

clean:
	rm -rf *.pb.cc *.pb.h