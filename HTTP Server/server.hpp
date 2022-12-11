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
#include <sys/sendfile.h>

using namespace std;

enum ServerType {
  HTTP_SERVER = 0,
  BACKEND_COORDINATOR,
  BACKEND_SERVER,
  OTHERS,
};

void parseInput(int argc, char *argv[]);
sockaddr_in parseSockaddr(string s);
void *messageWorker(void *comm_fd);
void sendBinary(string file, string image_type, int fd, string page);