#include <string>
#include "../KVStore/kvstore.grpc.pb.h"

class KVStoreClient {
 public:
  KVStoreClient(std::string kvstore_addr) {};

  // Read commands from STDIN.
  void Run() {};
  std::string GetNodeAddr(const std::string& row, const std::string& col) {return " ";};
  std::string Get(const std::string& row, const std::string& col){return " ";};
  void Put(const std::string& row, const std::string& col,
           const std::string& value) {};
  void CPut(const std::string& row, const std::string& col,
            const std::string& cur_value, const std::string& new_value) {};
  void Delete(const std::string& row, const std::string& col) {};

 private:
  std::unique_ptr<KVStoreMaster::Stub> kvstore_master_;
};