#include "kvstore_client.h"

namespace KVStore {
namespace {
using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

const int kMessageSizeLimit = 1024 * 1024 * 1024;
const char* kServerListPath = "../../Config/serverlist.txt";

absl::StatusOr<std::string> GetMasterAddrFromConfig() {
  std::ifstream configFile(kServerListPath);
  if (configFile.good()) {
    std::string master_addr;
    std::getline(configFile, master_addr);
    return master_addr;
  }

  return absl::NotFoundError(
      absl::StrCat("no master node is found in ", kServerListPath));
}

}  // namespace

KVStoreClient::KVStoreClient() {
  absl::StatusOr<std::string> addr = GetMasterAddrFromConfig();
  if (addr.ok()) {
    ChannelArguments master_args;
    master_args.SetMaxReceiveMessageSize(kMessageSizeLimit);
    kvstore_master_ = KVStoreMaster::NewStub(grpc::CreateCustomChannel(
        *addr, grpc::InsecureChannelCredentials(), master_args));
  } else {
    std::cout << addr.status().ToString().c_str() << std::endl;
    exit(-1);
  }
}

absl::StatusOr<std::string> KVStoreClient::GetPrimaryNodeAddr(
    const std::string& row, const std::string& col) {
  ClientContext context;
  FetchNodeRequest req;
  FetchNodeResponse res;

  req.set_row(row);
  req.set_col(col);
  grpc::Status grpc_status =
      kvstore_master_->FetchNodeAddr(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return res.addr();
    } else {
      return absl::UnavailableError("kvstore service is unavailable.");
    }
  }

  return absl::UnavailableError("grpc failed, service unavailable.");
}

absl::StatusOr<std::string> KVStoreClient::Get(const std::string& row,
                                               const std::string& col) {
  absl::StatusOr<std::string> addr = GetPrimaryNodeAddr(row, col);
  if (!addr.ok()) {
    return addr.status();
  }

  ClientContext context;
  KVRequest req;
  KVResponse res;
  ChannelArguments args;
  // 5GB incoming message size.
  args.SetMaxReceiveMessageSize(kMessageSizeLimit);
  auto stub = KVStoreNode::NewStub(grpc::CreateCustomChannel(
      *addr, grpc::InsecureChannelCredentials(), args));

  // auto stub = KVStoreNode::NewStub(
  //     grpc::CreateChannel(*addr, grpc::InsecureChannelCredentials()));

  req.mutable_get_request()->set_row(row);
  req.mutable_get_request()->set_col(col);

  Status grpc_status = stub->Execute(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return res.message();
    } else {
      return absl::NotFoundError(res.message());
    }
  }

  return absl::UnavailableError("grpc failed, service unavailable.");
}

absl::Status KVStoreClient::Put(const std::string& row, const std::string& col,
                                const std::string& value) {
  absl::StatusOr<std::string> addr = GetPrimaryNodeAddr(row, col);
  if (!addr.ok()) {
    return addr.status();
  }

  ClientContext context;
  KVRequest req;
  KVResponse res;
  ChannelArguments args;
  args.SetMaxReceiveMessageSize(kMessageSizeLimit);
  auto stub = KVStoreNode::NewStub(grpc::CreateCustomChannel(
      *addr, grpc::InsecureChannelCredentials(), args));

  req.mutable_put_request()->set_row(row);
  req.mutable_put_request()->set_col(col);
  req.mutable_put_request()->set_value(value);

  Status grpc_status = stub->Execute(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return absl::OkStatus();
    } else {
      return absl::NotFoundError(res.message());
    }
  }

  return absl::UnavailableError("grpc failed, service unavailable.");
}

absl::Status KVStoreClient::CPut(const std::string& row, const std::string& col,
                                 const std::string& cur_value,
                                 const std::string& new_value) {
  absl::StatusOr<std::string> addr = GetPrimaryNodeAddr(row, col);
  if (!addr.ok()) {
    return addr.status();
  }

  ClientContext context;
  KVRequest req;
  KVResponse res;
  ChannelArguments args;
  args.SetMaxReceiveMessageSize(kMessageSizeLimit);
  auto stub = KVStoreNode::NewStub(grpc::CreateCustomChannel(
      *addr, grpc::InsecureChannelCredentials(), args));

  req.mutable_cput_request()->set_row(row);
  req.mutable_cput_request()->set_col(col);
  req.mutable_cput_request()->set_cur_value(cur_value);
  req.mutable_cput_request()->set_new_value(new_value);

  Status grpc_status = stub->Execute(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return absl::OkStatus();
    } else {
      return absl::NotFoundError(res.message());
    }
  }

  return absl::UnavailableError("grpc failed, service unavailable.");
}

absl::Status KVStoreClient::Delete(const std::string& row,
                                   const std::string& col) {
  absl::StatusOr<std::string> addr = GetPrimaryNodeAddr(row, col);
  if (!addr.ok()) {
    return addr.status();
  }

  ClientContext context;
  KVRequest req;
  KVResponse res;
  ChannelArguments args;
  args.SetMaxReceiveMessageSize(kMessageSizeLimit);
  auto stub = KVStoreNode::NewStub(grpc::CreateCustomChannel(
      *addr, grpc::InsecureChannelCredentials(), args));

  req.mutable_delete_request()->set_row(row);
  req.mutable_delete_request()->set_col(col);

  Status grpc_status = stub->Execute(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return absl::OkStatus();
    } else {
      return absl::NotFoundError(res.message());
    }
  }

  return absl::UnavailableError("grpc failed, service unavailable.");
}

}  // namespace KVStore