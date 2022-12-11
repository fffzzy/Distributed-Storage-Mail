#ifndef KVSTORE_NODE_UTILS_H
#define KVSTORE_NODE_UTILS_H

#include "common.h"

namespace KVStore {

struct Tablet {
  std::string path;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      map;
};

std::string GetTabletPath();

}  // namespace KVStore

#endif