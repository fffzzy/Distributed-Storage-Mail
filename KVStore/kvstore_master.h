#ifndef KVSTORE_MASTER_H
#define KVSTORE_MASTER_H

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "common.h"
#include "kvstore.grpc.pb.h"
namespace KVStore {

class KVStoreMasterImpl final : public KVStoreMaster::Service {
 public:
  KVStoreMasterImpl();

  ::grpc::Status FetchNodeAddr(::grpc::ServerContext* context,
                               const ::FetchNodeRequest* request,
                               ::FetchNodeResponse* response) override;
};

}  // namespace KVStore

#endif