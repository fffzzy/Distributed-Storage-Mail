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
#include <vector>
// #include <sys/sendfile.h>
#include "api_handler.hpp"
using namespace std;

enum ServerType {
  HTTP_SERVER = 0,
  BACKEND_COORDINATOR,
  BACKEND_SERVER,
  OTHERS,
};

void sigHandler(int num);
void parseInput(int argc, char *argv[]);
sockaddr_in parseSockaddr(string s);
void *messageWorker(void *comm_fd);
void sendBinary(string file, string image_type, int fd, string page);
void homepage(int fd);