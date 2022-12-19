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

static bool is_verbose = false;
static bool is_shut_down = false;
static vector<int> fds;
static vector<pthread_t> threads;
static int listen_fd;
static int port = 8003;
static sockaddr_in backend_coordinator_addr;
static sockaddr_in self_addr;

int main(int argc, char *argv[])
{
    // signal(SIGINT, sigHandler);
    parseInput(argc, argv);

    listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);
    while (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        servaddr.sin_port = htons(--port);
    }
    cout << "Server start a port at: " << port << endl;
    listen(listen_fd, 100);
    while (!is_shut_down)
    {
        struct sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        int comm_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (!is_shut_down)
        {
            fds.push_back(comm_fd);
            // printf("Connection from %s\n", inet_ntoa(clientaddr.sin_addr));
            if (is_verbose)
                fprintf(stderr, "[%d] New connection\n", comm_fd);
            pthread_t p;
            threads.push_back(p);
            pthread_create(&threads.back(), NULL, messageWorker, (void *)&comm_fd);
        }
    }

    return 0;
}

void parseInput(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "v")) != -1)
    {
        switch (opt)
        {
        case 'v':
            is_verbose = true;
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s [-v]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // int index = atoi(argv[optind]);

    // ifstream server_list("../servers.config");

    // int line_num = 1;
    // string line;
    // ServerType type;
    // while (getline(server_list, line))
    // {
    //     if (line == "HTTP Servers")
    //     {
    //         type = HTTP_SERVER;
    //     }
    //     else if (line == "Backend Coordinator")
    //     {
    //         type = BACKEND_COORDINATOR;
    //     }
    //     else if (line == "Backend Servers")
    //     {
    //         type = OTHERS;
    //     }
    //     else
    //     {
    //         switch (type)
    //         {
    //         case HTTP_SERVER:
    //             if (line_num == index)
    //             {
    //                 self_addr = parseSockaddr(line);
    //             }
    //             line_num++;
    //             break;

    //         case BACKEND_COORDINATOR:
    //             if (!line.empty())
    //                 backend_coordinator_addr = parseSockaddr(line);

    //         default:
    //             break;
    //         }
    //     }
    // }
}

void sigHandler(int num)
{
    char SHUTDOWN[] = "-ERR Server shutting down\r\n";

    is_shut_down = true;
    close(listen_fd);
    for (auto fd : fds)
    {
        // write(fd, SHUTDOWN, strlen(SHUTDOWN));
        close(fd);
        if (is_verbose)
        {
            // fprintf(stderr, "fd: %d\n", fd);
            fprintf(stderr, "[%d] Connection closed\n", fd);
        }
    }
    for (auto thread : threads)
    {
        pthread_kill(thread, 0);
    }
}

sockaddr_in parseSockaddr(string s)
{
    int idx = s.find(":");
    string ip = s.substr(0, idx);
    int port = stoi(s.substr(idx + 1));
    if (is_verbose)
        cout << port << endl;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
    return addr;
}

void *messageWorker(void *comm_fd)
{
    char buffer[4280] = {};

    int fd = *(int *)comm_fd;
    read(fd, buffer, 4279);
    if (is_verbose)
        printf("%s\n", buffer);

    string page = "HTTP/1.1 200 OK\r\n";
    write(fd, page.c_str(), page.length());

    close(fd);
    return comm_fd;
}
