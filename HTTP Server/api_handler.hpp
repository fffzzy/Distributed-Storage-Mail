#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

#include "../KVStore/kvstore_client.h"

using json = nlohmann::json;

#include "local_kvstore.hpp"

using namespace std;

enum ReqType { GET = 0, POST, DELETE };

enum CookieType { MISSING = 0, WRONG, CORRECT };

class APIHandler {
 public:
  string header;
  json data;
  int fd;
  bool is_verbose;
  KVStoreClient& kvstore;

  APIHandler(string buf, int f, bool is_verb, KVStoreClient& kvstore)
      : fd(f), is_verbose(is_verb), kvstore(kvstore) {
    size_t division = buf.find("\r\n\r\n");
    header = buf.substr(0, division + 2);
    string body = buf.substr(division + 4);
    if (body.find("{") == string::npos) {
      data = nullptr;
    } else {
      data = json::parse(body);
    }
    if (is_verbose) {
      cout << buf << endl;
      cout << data << endl;
    }
  };
  void parsePost();
  void parseGet();
  void parseDelete();
  void signup();
  void login();
  void logout();
  string checkCookie();

 private:
  KVStore::KVStoreClient kvstore_client_;
};

string urlEncode(string str);