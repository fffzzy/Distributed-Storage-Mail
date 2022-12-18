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
#include <ext/stdio_filebuf.h>
#include <sys/file.h>
#include <nlohmann/json.hpp>

#include "local_kvstore.hpp"

using json = nlohmann::json;

using namespace std;

enum Status
{
    Begin = 0,
    Helo,
    Mail,
    Rcpt,
    Data,
    Done,
};

class MailService
{
private:

public:
    MailService() {};

    void sendOut(json data) {};

    void startAccepting();
};

void *messageWorker(void *);