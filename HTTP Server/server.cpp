#include "server.hpp"

static bool is_verbose = false;
static bool is_shut_down = false;
static vector<int> fds;
static vector<pthread_t> threads;
static int listen_fd;

int main(int argc, char *argv[])
{
    int port = 10000;
    int opt;

    signal(SIGINT, sigHandler);

    while ((opt = getopt(argc, argv, "p:av")) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;

        case 'a':
            fprintf(stderr, "Zhouyang Fang (fangzhy)");
            exit(EXIT_FAILURE);

        case 'v':
            is_verbose = true;
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s [-v] [-a] [-p portno]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
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

void sigHandler(int num)
{
    char SHUTDOWN[] = "-ERR Server shutting down\r\n";

    is_shut_down = true;
    close(listen_fd);
    for (auto fd : fds)
    {
        write(fd, SHUTDOWN, strlen(SHUTDOWN));
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

void *messageWorker(void *comm_fd)
{
    char buffer[1020] = {};
    char *head = buffer;
    char *tail;
    char *text_head;
    char command[6];
    char message[1001];
    int size;

    const char *GREETING = "+OK Server ready (Author: Zhouyang Fang / fangzhy)\r\n";
    const char *OK = "+OK ";
    const char *BYE = "+OK Goodbye!\r\n";
    const char *UNKNOWN = "-ERR Unknown command\r\n";
    const char *CLOSE = "Connection closed\r\n";

    int fd = *(int *)comm_fd;
    write(fd, GREETING, strlen(GREETING));

    while (true)
    {
        read(fd, head, 1020 - strlen(buffer));
        while ((tail = strstr(buffer, "\r\n")) != NULL)
        {
            tail += 2 * sizeof(char);
            memcpy(command, buffer, 5 * sizeof(char));
            command[5] = '\0';
            text_head = buffer + 5;

            if (!strcasecmp(command, "ECHO "))
            {
                memcpy(message, OK, strlen(OK));
                size = tail - text_head;
                memcpy(message + 4, text_head, size);
                message[size + 4] = '\0';
                write(fd, message, strlen(message));
                if (is_verbose)
                {
                    // fprintf(stderr, "char size: %d\n", int(sizeof(char)));
                    fprintf(stderr, "[%d] C: %.*s", fd, int(tail - buffer), buffer);
                    fprintf(stderr, "[%d] S: %s", fd, message);
                }
            }
            else if (!strcasecmp(command, "QUIT\r"))
            {
                memcpy(message, BYE, strlen(BYE) + 1);
                write(fd, message, strlen(message));
                if (is_verbose)
                {
                    fprintf(stderr, "[%d] C: QUIT\n", fd);
                    fprintf(stderr, "[%d] S: %s", fd, message);
                    fprintf(stderr, "[%d] Connection closed\n", fd);
                }
                return NULL;
            }
            else
            {
                memcpy(message, UNKNOWN, strlen(UNKNOWN) + 1);
                write(fd, message, strlen(message));
                if (is_verbose)
                {
                    // fprintf(stderr, "%s\n", command);
                    fprintf(stderr, "%s", buffer);
                }
            }
            head = buffer;
            while (head != tail)
            {
                *head = '\0';
                head++;
            }
            head = buffer;
            while (*tail != '\0')
            {
                *head = *tail;
                *tail = '\0';
                head++;
                tail++;
            }
        }
        while (*head != '\0')
            head++;

        // fprintf(stderr, "%d\n", *head);
    }
    close(fd);
}
