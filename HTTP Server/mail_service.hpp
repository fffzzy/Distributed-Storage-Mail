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
#include <chrono>
#include <nlohmann/json.hpp>

#include "local_kvstore.hpp"


using json = nlohmann::json;


using namespace std;

class Mail{
private:
  json data;
public:
  Mail(json data) : data(data) {};
  void sendOut();
};