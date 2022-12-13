#ifndef KVSTORE_MASTER_H
#define KVSTORE_MASTER_H

#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "common.h"
#include "kvstore.grpc.pb.h"
namespace KVStore {

const char* kServerlistPath = "../../Config/serverlist.txt";
const int kNumOfReplicas = 3;

struct Cluster {
  int id;
  // Addr of primary node.
  std::string primary;
  // [addr(str), alive(bool)]
  std::unordered_map<std::string, bool> isAlive;
  // [addr(str), stub]
  std::unordered_map<std::string, std::unique_ptr<KVStoreNode::Stub>> stubs;
};

class KVStoreMasterImpl final : public KVStoreMaster::Service {
 public:
  KVStoreMasterImpl();

  ::grpc::Status FetchNodeAddr(::grpc::ServerContext* context,
                               const ::FetchNodeRequest* request,
                               ::FetchNodeResponse* response) override;

 public:
  void ReadConfig();
  void CheckReplicaHealth();
  std::string GetAddr();

 public:
  std::string master_addr_;
  std::vector<Cluster> clusters_;
};

}  // namespace KVStore

#endif