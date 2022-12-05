#include "kv_store_server.h"

namespace KVStore {
namespace {
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

}  // namespace

Status KVStoreServiceImpl::Put(ServerContext* context,
                               const KVPutRequest* request,
                               KVPutResponse* response) {
  std::string row = request->row();
  std::string col = request->col();
  std::string value = request->value();

  std::string msg =
      absl::StrCat("Put value: ", value, " into (", row, ",", col, ").");

  response->set_status(KVResStatus::SUCCESS);
  response->set_message(msg);
  return Status::OK;
}

Status KVStoreServiceImpl::Get(ServerContext* context,
                               const KVGetRequest* request,
                               KVGetResponse* response) {
  std::string row = request->row();
  std::string col = request->col();

  std::string msg = absl::StrCat("Get value from (", row, ",", col, ").");

  response->set_status(KVResStatus::SUCCESS);
  response->set_message(msg);
  return Status::OK;
}

Status KVStoreServiceImpl::CPut(ServerContext* context,
                                const KVCPutRequest* request,
                                KVCPutResponse* response) {
  std::string row = request->row();
  std::string col = request->col();
  std::string prev_value = request->prev_value();
  std::string value = request->value();

  std::string msg = absl::StrCat("CPut value: ", value, " into (", row, ",",
                                 col, ") if previous value is: ", prev_value);

  response->set_status(KVResStatus::SUCCESS);
  response->set_message(msg);
  return Status::OK;
}

Status KVStoreServiceImpl::Delete(ServerContext* context,
                                  const KVDeleteRequest* request,
                                  KVDeleteResponse* response) {
  std::string row = request->row();
  std::string col = request->col();

  std::string msg = absl::StrCat("Delete value in (", row, ",", col, ").");

  response->set_status(KVResStatus::SUCCESS);
  response->set_message(msg);
  return Status::OK;
}

}  // namespace KVStore

int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:10000");
  KVStore::KVStoreServiceImpl service;

  KVStore::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();

  return 0;
}