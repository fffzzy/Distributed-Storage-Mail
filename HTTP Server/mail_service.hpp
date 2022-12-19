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
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cctype>
#include <chrono>
#include <ext/stdio_filebuf.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

#include "local_kvstore.hpp"

using json = nlohmann::json;

using namespace std;

// struct Pram {
//     int fd;
//     KVStoreClient &kvstore;
// };

enum Status {
  Begin = 0,
  Helo,
  Mail,
  Rcpt,
  Data,
  Done,
};

class MailService {
 private:
 public:
  MailService(){};

  void sendOut(json data){};

  void startAccepting();
};

void *mailWorker(void *);