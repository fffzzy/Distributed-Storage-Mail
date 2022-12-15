#include "common.h"
#include "kvstore.grpc.pb.h"
#include "kvstore_client.h"
#include "regex"

namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

const std::regex regex_get("get (.+) (.+)");
const std::regex regex_put("put (.+) (.+) (.+)");
const std::regex regex_cput("cput (.+) (.+) (.+) (.+)");
const std::regex regex_delete("delete (.+) (.+)");

class KVStoreTestClient : public KVStore::KVStoreClient {
 public:
  KVStoreTestClient() : KVStoreClient(){};

  // Read commands from STDIN.
  void Run();
};

void KVStoreTestClient::Run() {
  std::smatch sm;
  for (std::string line; std::getline(std::cin, line);) {
    if (std::regex_match(line, sm, regex_get)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      auto res = Get(row, col);
      if (res.ok()) {
        std::cout << "[Get] succeeded: " << res->data() << std::endl;
      } else {
        std::cout << "[Get] failed: " << res.status().ToString() << std::endl;
      }
    } else if (std::regex_match(line, sm, regex_put)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      std::string value = sm[3].str();
      auto res = Put(row, col, value);
      if (res.ok()) {
        std::cout << "[Put] succeeded." << std::endl;
      } else {
        std::cout << "[Put] failed: " << res.ToString() << std::endl;
      }
    } else if (std::regex_match(line, sm, regex_cput)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      std::string curr_value = sm[3].str();
      std::string new_value = sm[4].str();
      auto res = CPut(row, col, curr_value, new_value);
      if (res.ok()) {
        std::cout << "[CPut] succeeded." << std::endl;
      } else {
        std::cout << "[CPut] failed: " << res.ToString() << std::endl;
      }
    } else if (std::regex_match(line, sm, regex_delete)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      auto res = Delete(row, col);
      if (res.ok()) {
        std::cout << "[Delete] succeeded." << std::endl;
      } else {
        std::cout << "[Delete] failed: " << res.ToString() << std::endl;
      }
    } else {
      fprintf(stderr, "Unpported commands: %s\n", line.c_str());
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  KVStoreTestClient client;
  client.Run();
}
