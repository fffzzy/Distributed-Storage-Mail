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
  // string header;
  // json data;
  // int fd;
  bool is_verbose;
  KVStore::KVStoreClient kvstore_;
  KVStore::KVStoreConsole console_;
  // string chunk;

  APIHandler(bool is_verb)
      : is_verbose(is_verb){
            // if (is_verbose)
            // {
            //     cout << buf << endl;
            //     cout << data << endl;
            // }
        };
  void parsePost(string buf, int fd);
  void parseGet(string buf, int fd);
  void parseDelete(string buf, int fd);

  void signup(int fd, json& data);
  void login(int fd, json& data);
  void logout(int fd, string& header);
  void changePassword(int fd, json& data);

  void sendEmail(int fd, string& header, json& data);
  void getEmailList(int fd, string& header);
  void deleteEmail(int fd, string& header);

  void getFiles(int fd, string& header);
  void changeFiles(int fd, string& header, json& data);

  void uploadFile(int fd, string& header, string& chunk);
  void downloadFile(int fd, string& header);
  void deleteFile(int fd, string& header);

  void checkBackend(int fd);
  void checkFrontend(int fd);
  void suspendNode(int fd, string& header);
  void reviveNode(int fd, string& header);
  void showKeyValue(int fd, string& header);

  string extractValueFromHeader(string key, string& header);
  string checkCookie(string& header);
  void parseEmail(string email, string& user, string& host);
};

string urlEncode(string str);