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

std::string GetNodeDirPath(int node_idx);

std::string GetTabletFilePath(int node_idx, int tablet_idx);

std::string GetLogFilePath(int node_idx);

}  // namespace KVStore

#endif