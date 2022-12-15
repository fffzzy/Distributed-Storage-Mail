#ifndef KVSTORE_CLIENT_H
#define KVSTORE_CLIENT_H

#include "common.h"
#include "kvstore.grpc.pb.h"

namespace KVStore {

class KVStoreClient {
 public:
  KVStoreClient();

 public:
  /**
   * @brief Find value at (row, col) in KVStore.
   *
   * @param row
   * @param col
   * @return absl::StatusOr<std::string>
   */
  absl::StatusOr<std::string> Get(const std::string& row,
                                  const std::string& col);

  /**
   * @brief Put value at (row, col) in KVStore.
   *
   * @param row
   * @param col
   * @param value
   * @return absl::Status
   */
  absl::Status Put(const std::string& row, const std::string& col,
                   const std::string& value);

  /**
   * @brief Put new_value at (row, col) in KVStore if current value is
   * cur_value.
   *
   * @param row
   * @param col
   * @param cur_value
   * @param new_value
   * @return absl::Status
   */
  absl::Status CPut(const std::string& row, const std::string& col,
                    const std::string& cur_value, const std::string& new_value);

  /**
   * @brief Delete value at (row, col) in KVStore.
   *
   * @param row
   * @param col
   * @return absl::Status
   */
  absl::Status Delete(const std::string& row, const std::string& col);

 private:
  absl::StatusOr<std::string> GetPrimaryNodeAddr(const std::string& row,
                                                 const std::string& col);

 private:
  std::unique_ptr<KVStoreMaster::Stub> kvstore_master_;
};
}  // namespace KVStore

#endif