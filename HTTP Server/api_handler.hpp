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
// #include <sys/sendfile.h>

#include "kvstore_client.hpp"

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
  string buffer;
  int fd;
  bool is_verbose;
  KVStoreClient kvstore;

  APIHandler(string buf, int f, bool is_verb) : buffer(buf), fd(f), is_verbose(is_verb), kvstore("127.0.0.1:8017") {
    if (is_verbose) cout << buffer << endl;
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