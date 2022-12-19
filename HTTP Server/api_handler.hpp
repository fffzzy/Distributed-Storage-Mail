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

#include <chrono>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

#include "../KVStore/kvstore_client.h"
#include "../KVStore/kvstore_console.h"
#include "mail_service.hpp"

using namespace std;

enum ReqType { GET = 0, POST, DELETE };

enum CookieType { MISSING = 0, WRONG, CORRECT };

class APIHandler {
 public:
  string header;
  json data;
  int fd;
  bool is_verbose;
  KVStore::KVStoreClient kvstore_;
  KVStore::KVStoreConsole console_;
  string chunk;

  APIHandler(string buf, int f, bool is_verb) : fd(f), is_verbose(is_verb) {
    size_t division = buf.find("\r\n\r\n");
    header = buf.substr(0, division + 2);
    string body = buf.substr(division + 4);
    if (body.find("{") == string::npos) {
      data = nullptr;
      chunk = body;
    } else if (body.size() < 10000) {
      data = json::parse(body);
      chunk = body;
    } else {
      data = nullptr;
      chunk = body;
    }
    // if (is_verbose)
    // {
    //     cout << buf << endl;
    //     cout << data << endl;
    // }
  };
  void parsePost();
  void parseGet();
  void parseDelete();

  void signup();
  void login();
  void logout();

  void sendEmail();
  void getEmailList();
  void deleteEmail();

  void getFiles();
  void changeFiles();

  void uploadFile();
  void downloadFile();
  void deleteFile();

  void checkBackend();
  void checkFrontend();
  void suspendNode();
  void reviveNode();

  string extractValueFromHeader(string key);
  string checkCookie();
  void parseEmail(string email, string &user, string &host);
};

string urlEncode(string str);