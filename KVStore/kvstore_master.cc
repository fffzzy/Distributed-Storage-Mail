#include "kvstore_master.h"

namespace KVStore {
namespace {
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
}  // namespace

KVStoreMasterImpl::KVStoreMasterImpl() {
  // TODO: add initialization later.
}

Status KVStoreMasterImpl::FetchNodeAddr(ServerContext* context,
                                        const FetchNodeRequest* request,
                                        FetchNodeResponse* response) {
  // Use "{row}-{col}" as key for hashing.
  std::string key = absl::StrCat(request->row(), "-", request->col());
  std::cout << "Receive request with key: " << key << std::endl;
  // TODO: sha-256, return the address of primary node in corresponding cluster.
  return Status::OK;
}

}  // namespace KVStore

int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:10001");
  KVStore::KVStoreMasterImpl service;

  ::grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return 0;
}