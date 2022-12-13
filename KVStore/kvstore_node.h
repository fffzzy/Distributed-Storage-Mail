#ifndef KVSTORE_NODE_H
#define KVSTORE_NODE_H

#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "common.h"
#include "kvstore.grpc.pb.h"
#include "kvstore_node_utils.h"

namespace KVStore {

class KVStoreNodeImpl final : public KVStoreNode::Service {
 public:
  KVStoreNodeImpl();

  ::grpc::Status Execute(::grpc::ServerContext* context,
                         const ::KVRequest* request,
                         ::KVResponse* response) override;

  ::grpc::Status CheckHealth(::grpc::ServerContext* context,
                             const ::google::protobuf::Empty* request,
                             ::google::protobuf::Empty* response) override;

 public:
  std::string addr;  // ip+port of this node
  bool verbose = false;
  int node_idx = -1;
  std::string master_addr;
  std::vector<std::string> peer_addr_vec;

 private:
  // Stub communicates to master node.
  std::unique_ptr<KVStoreMaster::Stub> master_stub;
  // Stub communicates to replicas in the same cluster.
  std::vector<std::unique_ptr<KVStoreNode::Stub>> peer_stub_vec;
  // tablet mem vector
  std::vector<std::unique_ptr<Tablet>> tablet_mem_vec;

 public:
  void ReadConfig();
  void InitEnv();  // set up tablet file, log, stub with master and peers

 private:
  void KVGet(const KVRequest_KVGetRequest* request, KVResponse* response);

  // get tablet from memory, if not found, return NULL
  Tablet* GetTabletFromMem(int tablet_idx);
};

}  // namespace KVStore

#endif