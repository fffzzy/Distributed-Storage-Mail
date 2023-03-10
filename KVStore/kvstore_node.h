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
  std::string addr;  // ip+port of this node
  bool verbose = false;
  int node_idx = -1;
  std::string master_addr;
  std::vector<std::string> peer_addr_vec;
  KVStoreNodeStatus node_status = KVStoreNodeStatus::RUNNING;

  // Stub communicates to master node.
  std::unique_ptr<KVStoreMaster::Stub> master_stub;
  // Stub communicates to replicas in the same cluster.
  std::vector<std::unique_ptr<KVStoreNode::Stub>> peer_stub_vec;
  // tablet mem vector
  std::vector<Tablet*> tablet_mem_vec;

  std::string target_addr_to_recover;

 public:
  KVStoreNodeImpl();

  void VerboseLog(char* msg);

  void VerboseLog(std::string msg);
  // get tablet from memory, if not found, return NULL
  Tablet* GetTabletFromMem(int tablet_idx);

  // unload the first tablet in tablet_mem_vec to file
  void UnloadTablet();

  // load tablet from file, and insert it to tablet_mem_vec
  Tablet* LoadTablet(int tablet_idx);

  ::grpc::Status CheckHealth(::grpc::ServerContext* context,
                             const ::google::protobuf::Empty* request,
                             ::KVResponse* response) override;

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

  void KVSuspend(const KVRequest_KVSuspendRequest* request,
                 KVResponse* response);

  // when primary node receives recovery request from master
  static void* KVPrimaryRecoveryThreadFunc(void* args);

  void KVPrimaryRecovery(const KVRequest_KVRecoveryRequest* request,
                         KVResponse* response);

  // when secondary node receives recovery request from primary
  void KVSecondaryRecovery(const KVRequest_KVSrecoveryRequest* request,
                           KVResponse* response);

  // secondary compares checksum for file to transfer, respond SUCCESS
  // represents checksum is different, need file transfer
  void KVChecksum(const KVRequest_KVChecksumRequest* request,
                  KVResponse* response);

  // secondary receives file transfer from primary
  void KVFiletransfer(const KVRequest_KVFiletransferRequest* request,
                      KVResponse* response);

  Tablet* ReplayTablet(int tablet_idx);

  // secondary receives replay request from primary
  void KVReplay(const KVRequest_KVReplayRequest* request, KVResponse* response);

  void KVKeyvalueRequest(const KVRequest_KVKeyvalueRequest* request,
                         KVResponse* response);

  void KVSkeyvalueRequest(const KVRequest_KVSkeyvalueRequest* request,
                          KVResponse* response);
};

void* KVPrimaryRecoveryThreadFunc(void* args);

void computeDigest(char* data, int dataLengthBytes,
                   unsigned char* digestBuffer);

std::string getDigestStr(std::string target);

}  // namespace KVStore

#endif