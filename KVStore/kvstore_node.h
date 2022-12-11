#ifndef KVSTORE_NODE_H
#define KVSTORE_NODE_H

#include <grpc/grpc.h>
<<<<<<< HEAD
=======
#include <grpcpp/grpcpp.h>
>>>>>>> kvstore-lsd
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "common.h"
#include "kvstore.grpc.pb.h"

    namespace KVStore {

  struct Tablet {
    std::string path;
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string> >
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
    // Stub communicates to master node.
    std::unique_ptr<KVStoreMaster::Stub> master_stub_;
    // Stub communicates to another replica in the same cluster.
    std::unique_ptr<KVStoreNode::Stub> node_stub1_;
    std::unique_ptr<KVStoreNode::Stub> node_stub2_;
  };
}  // namespace KVStore

#endif