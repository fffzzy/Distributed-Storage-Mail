#ifndef COMMON_H
#define COMMON_H

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace KVStore {
// it's a deterministic hash function, whether the node recovers from a crash or
// not.
inline unsigned long GetDigest(const std::string row, const std::string col) {
  std::string s = absl::StrCat(row, col);

  const int p = 31;
  const int n = 1e9 + 7;
  unsigned long digest = 0;
  unsigned long pow = 1;

  for (int i = 0; i < s.length(); i++) {
    digest = (digest + s.at(i) * pow) % n;
    pow = (pow * p) % n;
  }

  return digest;
}

enum KVStoreNodeStatus {
  RUNNING,     // Node is running normally.
  SUSPENDED,   // Node is suspended and rejects all requests.
  RECOVERING,  // Node is recoverying and rejects all requests.
  CRASHED      // Node is crashed, all rpc failed.
};

}  // namespace KVStore
#endif