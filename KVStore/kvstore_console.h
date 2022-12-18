#ifndef KVSTORE_CONSOLE_H
#define KVSTORE_CONSOLE_H

#include "common.h"
#include "kvstore.grpc.pb.h"

namespace KVStore {

struct NodeInfo {
  bool is_primary;
  std::string addr;
  std::string status;
};

class KVStoreConsole {
 public:
  KVStoreConsole();

 public:
  /**
   * @brief Poll status information of all nodes.
   *
   * @return absl::StatusOr<std::vector<std::vector<NodeInfo>>>
   */
  absl::StatusOr<std::vector<std::vector<NodeInfo>>> PollStatus();

  /**
   * @brief Suspend specific node, given its address.
   *
   * @param node_addr
   * @return absl::Status
   */
  absl::Status Suspend(const std::string& node_addr);

  /**
   * @brief Revive specific node, given its address.
   *
   * @param node_addr
   * @return absl::Status
   */
  absl::Status Revive(const std::string& node_addr);

 private:
  /**
   * @brief Find which cluster the node belongs to.
   *
   * @param node
   * @return int
   */
  int FindClusterId(const std::string& node);

 private:
  std::unique_ptr<KVStoreMaster::Stub> kvstore_master_;

 private:
  std::unordered_map<std::string, int> node_to_cluster_;
};
}  // namespace KVStore

#endif