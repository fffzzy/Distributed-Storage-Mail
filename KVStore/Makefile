proto:
	protoc -I ../Protos --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../Protos/kv_store.proto
	protoc -I ../Protos --cpp_out=. ../Protos/kv_store.proto

clean:
	rm -rf *.pb.cc *.pb.h