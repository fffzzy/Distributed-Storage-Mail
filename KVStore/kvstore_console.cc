#include "kvstore_console.h"

namespace KVStore {
namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

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

KVStoreConsole::KVStoreConsole() {
  absl::StatusOr<std::string> addr = GetMasterAddrFromConfig();
  if (addr.ok()) {
    kvstore_master_ = KVStoreMaster::NewStub(
        grpc::CreateChannel(addr->data(), grpc::InsecureChannelCredentials()));
  } else {
    std::cout << addr.status().ToString().c_str() << std::endl;
    exit(-1);
  }
}

int KVStoreConsole::FindClusterId(const std::string& node) {
  if (node_to_cluster_.find(node) == node_to_cluster_.end()) {
    return -1;
  }
  return node_to_cluster_[node];
}

absl::StatusOr<std::vector<std::vector<NodeInfo>>>
KVStoreConsole::PollStatus() {
  ClientContext context;
  PollStatusResponse res;
  Status grpc_status =
      kvstore_master_->PollStatus(&context, PollStatusRequest(), &res);
  if (grpc_status.ok()) {
    std::vector<std::vector<NodeInfo>> status;
    for (int i = 0; i < res.clusters_size(); i++) {
      std::vector<NodeInfo> cluster;
      for (int j = 0; j < res.clusters(i).nodes_size(); j++) {
        NodeInfo node_info{.addr = res.clusters(i).nodes(j).address(),
                           .is_primary = res.clusters(i).nodes(j).is_primary(),
                           .status = res.clusters(i).nodes(j).state()};
        cluster.push_back(node_info);
      }
      status.push_back(cluster);
    }

    if (node_to_cluster_.empty()) {
      for (int i = 0; i < status.size(); i++) {
        for (auto& node : status[i]) {
          node_to_cluster_[node.addr] = i;
        }
      }
    }

    return status;
  } else {
    return absl::UnavailableError("grpc failed, service unavailable.");
  }
}

absl::Status KVStoreConsole::Suspend(const std::string& node) {
  int cluster_id = FindClusterId(node);
  if (cluster_id < 0) {
    return absl::NotFoundError(node + " not found.");
  }

  ClientContext context;
  SuspendRequest req;
  KVResponse res;
  req.set_cluster_id(cluster_id);
  req.set_node_addr(node);
  Status grpc_status = kvstore_master_->Suspend(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return absl::OkStatus();
    } else {
      return absl::CancelledError(res.message());
    }
  }
  return absl::UnavailableError("grpc failed, service unavailable.");
}

absl::Status KVStoreConsole::Revive(const std::string& node) {
  int cluster_id = FindClusterId(node);
  if (cluster_id < 0) {
    return absl::NotFoundError(node + " not found.");
  }

  ClientContext context;
  ReviveRequest req;
  KVResponse res;
  req.set_cluster_id(cluster_id);
  req.set_node_addr(node);
  Status grpc_status = kvstore_master_->Revive(&context, req, &res);
  if (grpc_status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return absl::OkStatus();
    } else {
      return absl::CancelledError(res.message());
    }
  }

  return absl::UnavailableError("grpc failed, service unavailable.");
}

}  // namespace KVStore