#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

#include "local_kvstore.hpp"

using namespace std;

enum ReqType
{
    GET = 0,
    POST,
    DELETE
};

enum CookieType
{
    MISSING = 0,
    WRONG,
    CORRECT
};

class APIHandler{
public:
  string header;
  json data;
  int fd;
  bool is_verbose;
  KVStoreClient& kvstore;

  APIHandler(string buf, int f, bool is_verb, KVStoreClient& kvstore) : fd(f), is_verbose(is_verb), kvstore(kvstore) {
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
};

string urlEncode(string str);