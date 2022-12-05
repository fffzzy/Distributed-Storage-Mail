#ifndef KV_STORE_SERVER_H
#define KV_STORE_SERVER_H

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "common.h"
#include "kv_store.grpc.pb.h"

namespace KVStore {

struct TabletStruct {
  std::string path;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      map;
};

class KVStoreServiceImpl final : public KVStoreService::Service {
 public:
  KVStoreServiceImpl();

  ::grpc::Status Put(::grpc::ServerContext* context,
                     const ::KVPutRequest* request,
                     ::KVPutResponse* response) override;

  ::grpc::Status Get(::grpc::ServerContext* context,
                     const ::KVGetRequest* request,
                     ::KVGetResponse* response) override;

  ::grpc::Status CPut(::grpc::ServerContext* context,
                      const ::KVCPutRequest* request,
                      ::KVCPutResponse* response) override;

  ::grpc::Status Delete(::grpc::ServerContext* context,
                        const ::KVDeleteRequest* request,
                        ::KVDeleteResponse* response) override;

 private:
  std::unique_ptr<TabletStruct> tablet_;
};

}  // namespace KVStore

#endif