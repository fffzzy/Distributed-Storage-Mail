#ifndef COMMON_H
#define COMMON_H

#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

// it's a deterministic hash function, whether the node recovers from a crash or
// not.
inline int GetDigest(const std::string row, const std::string col) {
  std::string s = absl::StrCat(row, col);

  const int p = 31;
  const int n = 1e9 + 7;
  int digest = 0;
  long pow = 1;

  for (int i = 0; i < s.length(); i++) {
    digest = (digest + (s.at(i) - 'a' + 1) * pow) % n;
    pow = (pow * p) % n;
  }

  return digest;
}

#endif