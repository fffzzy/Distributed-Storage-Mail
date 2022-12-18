#include "common.h"
#include "kvstore.grpc.pb.h"
#include "kvstore_console.h"
#include "regex"

namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

const std::regex regex_poll("poll");
const std::regex regex_suspend("suspend (.+)");
const std::regex regex_revive("revive (.+)");

class KVStoreTestConsole : public KVStore::KVStoreConsole {
 public:
  KVStoreTestConsole() : KVStoreConsole() {}

  void Run();
};

void KVStoreTestConsole::Run() {
  std::smatch sm;
  for (std::string line; std::getline(std::cin, line);) {
    if (std::regex_match(line, sm, regex_poll)) {
      auto res = PollStatus();
      if (res.ok()) {
        std::cout << "[Poll] succeed: " << std::endl;
        for (int i = 0; i < res->size(); i++) {
          std::cout << "Cluster-" << i << std::endl;
          for (auto& node : (*res)[i]) {
            std::cout << node.addr << ": (" << node.is_primary << " - "
                      << node.status << std::endl;
          }
        }
      } else {
        std::cout << "[Poll] failed: " << res.status().ToString() << std::endl;
      }
    } else if (std::regex_match(line, sm, regex_suspend)) {
      auto res = Suspend(sm[1].str());
      if (res.ok()) {
        std::cout << "[Suspend] succeed: " << std::endl;
      } else {
        std::cout << "[Suspend] failed: " << res.ToString() << std::endl;
      }
    } else if (std::regex_match(line, sm, regex_revive)) {
      auto res = Revive(sm[1].str());
      if (res.ok()) {
        std::cout << "[Revive] succeed: " << std::endl;
      } else {
        std::cout << "[Revive] failed: " << res.ToString() << std::endl;
      }
    } else {
      fprintf(stderr, "Unpported commands: %s\n", line.c_str());
    }
  }
}
};  // namespace

int main(int argc, char** argv) {
  KVStoreTestConsole client;
  client.Run();
}