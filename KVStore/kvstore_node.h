#ifndef KVSTORE_NODE_H
#define KVSTORE_NODE_H

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "common.h"
#include "kvstore.grpc.pb.h"

namespace KVStore {

struct Tablet {
  std::string path;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      map;
};

class KVStoreNodeImpl final : public KVStoreNode::Service {
 public:
  KVStoreNodeImpl();

  ::grpc::Status Execute(::grpc::ServerContext* context,
                         const ::KVRequest* request,
                         ::KVResponse* response) override;

 private:
  std::unique_ptr<Tablet> tablet_;
};
}  // namespace KVStore

#endif