#ifndef KVSTORE_NODE_UTILS_H
#define KVSTORE_NODE_UTILS_H

#include "common.h"

namespace KVStore {

struct Tablet {
  int tablet_idx = -1;  // starts from 0 to num_tablet_total - 1;
  std::string path;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      map;
};

std::string GetTabletPath();

std::string GetNodeDirPath(int node_idx);

std::string GetTabletFilePath(int node_idx, int tablet_idx);

std::string GetLogFilePath(int node_idx, int tablet_idx);

int WriteTabletToFile(Tablet* tablet);

Tablet* LoadTabletFromFile(int node_idx, int tablet_idx);

// index ranges from 0 to num_tablet_total - 1 (inclusive)
int Digest2TabletIdx(int digest, int num_tablet_total);

}  // namespace KVStore

#endif