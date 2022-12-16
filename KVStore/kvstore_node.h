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

enum KVStoreNodeStatus { ALIVE, SHUTDOWN, RECOVERYING };

class KVStoreNodeImpl final : public KVStoreNode::Service {
 public:
  std::string addr;  // ip+port of this node
  bool verbose = false;
  int node_idx = -1;
  std::string master_addr;
  std::vector<std::string> peer_addr_vec;
  KVStoreNodeStatus node_status = KVStoreNodeStatus::ALIVE;

 private:
  // Stub communicates to master node.
  std::unique_ptr<KVStoreMaster::Stub> master_stub;
  // Stub communicates to replicas in the same cluster.
  std::vector<std::unique_ptr<KVStoreNode::Stub>> peer_stub_vec;
  // tablet mem vector
  std::vector<Tablet*> tablet_mem_vec;

 public:
  KVStoreNodeImpl();

 private:
  void VerboseLog(char* msg);

  void VerboseLog(std::string msg);
  // get tablet from memory, if not found, return NULL
  Tablet* GetTabletFromMem(int tablet_idx);

  // unload the first tablet in tablet_mem_vec to file
  void UnloadTablet();

  // load tablet from file, and insert it to tablet_mem_vec
  Tablet* LoadTablet(int tablet_idx);

 public:
  ::grpc::Status CheckHealth(::grpc::ServerContext* context,
                             const ::google::protobuf::Empty* request,
                             ::google::protobuf::Empty* response) override;

  void ReadConfig();

  void InitEnv();  // set up tablet file, log, stub with master and peers

  ::grpc::Status Execute(::grpc::ServerContext* context,
                         const ::KVRequest* request,
                         ::KVResponse* response) override;

 private:
  void KVGet(const KVRequest_KVGetRequest* request, KVResponse* response);

  void KVSget(const KVRequest_KVSgetRequest* request, KVResponse* response);

  void KVPut(const KVRequest_KVPutRequest* request, KVResponse* response);

  void KVSput(const KVRequest_KVSputRequest* request, KVResponse* response);

  void KVCput(const KVRequest_KVCPutRequest* request, KVResponse* response);

  void KVScput(const KVRequest_KVScputRequest* request, KVResponse* response);

  void KVDelete(const KVRequest_KVDeleteRequest* request, KVResponse* response);

  void KVSdelete(const KVRequest_KVSdeleteRequest* request,
                 KVResponse* response);
};

}  // namespace KVStore

#endif