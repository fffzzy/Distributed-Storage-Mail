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

using namespace std;

class SocketReader
{
private:
    int fd;
    string buffer;

public:
    SocketReader(int fd) : fd(fd) {};

    bool readLine(string &);
    bool readData(string &header, string &data);
    int hasLine();
    bool fillBuffer(int);
    void extractHeader(string &);
    bool readBulk(int size, string &data);
};