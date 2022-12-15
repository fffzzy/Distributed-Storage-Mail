#include "server.hpp"

static bool is_verbose = false;
static bool is_shut_down = false;
static vector<int> fds;
static vector<pthread_t> threads;
static int listen_fd;
static int port;
static sockaddr_in backend_coordinator_addr;
static sockaddr_in self_addr;
static string page_root = "./React/build";

int main(int argc, char *argv[])
{
    // signal(SIGINT, sigHandler);
    parseInput(argc, argv);

    listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    bind(listen_fd, (struct sockaddr *)&self_addr, sizeof(self_addr));
    listen(listen_fd, 100);
    while (!is_shut_down)
    {
        cout << "Server start" << endl;
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int comm_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
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

    int index = atoi(argv[optind]);

    ifstream server_list("../servers.config");

    int line_num = 1;
    string line;
    ServerType type;
    while (getline(server_list, line))
    {
        if (line == "HTTP Servers")
        {
            type = HTTP_SERVER;
        }
        else if (line == "Backend Coordinator")
        {
            type = BACKEND_COORDINATOR;
        }
        else if (line == "Backend Servers")
        {
            type = OTHERS;
        }
        else
        {
            switch (type)
            {
            case HTTP_SERVER:
                if (line_num == index)
                {
                    self_addr = parseSockaddr(line);
                }
                line_num++;
                break;

            case BACKEND_COORDINATOR:
                if (!line.empty())
                    backend_coordinator_addr = parseSockaddr(line);

            default:
                break;
            }
        }
    }
}

sockaddr_in parseSockaddr(string s)
{
    int idx = s.find(":");
    string ip = s.substr(0, idx);
    int port = stoi(s.substr(idx + 1));
    if (is_verbose) cout << port << endl;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
    return addr;
}

void *messageWorker(void *comm_fd)
{
    char buffer[128000] = {};

    int fd = *(int *)comm_fd;
    read(fd, buffer, 127999);
    if (is_verbose)
        printf("%s\n", buffer);

    if (!strncmp(buffer, "GET / ", 6))
    {
        string page = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

        ifstream infile(page_root + "/index.html");
        string line;
        while (getline(infile, line))
        {
            page += line;
        }
        write(fd, page.c_str(), page.length());
    }
    else if (!strncmp(buffer, "GET /api/", 9))
    {
        string buf(buffer);
        buf = buf.substr(9);
        APIHandler handler(buf, fd, is_verbose);
        handler.parseGet();
    }
    else if (!strncmp(buffer, "POST /api/", 10))
    {
        string buf(buffer);
        buf = buf.substr(10);
        APIHandler handler(buf, fd, is_verbose);
        handler.parsePost();
    }
    else if (!strncmp(buffer, "DELETE /api/", 12))
    {
        string buf(buffer);
        buf = buf.substr(12);
        APIHandler handler(buf, fd, is_verbose);
        handler.parseDelete();
    }
    else if (!strncmp(buffer, "GET", 3))
    {
        string buf(buffer);
        int head = buf.find(" ");
        string tmp = buf.substr(head + 1);
        int tail = tmp.find(" ");
        string file = tmp.substr(0, tail);
        string page = "HTTP/1.1 200 OK\r\n";
        string file_type = file.substr(file.find_last_of(".") + 1);
        string path = page_root + file;
        ios_base::openmode mode = ios_base::in;

        if (file_type == "jpg" | file_type == "png")
        {
            sendBinary(path, file_type, fd, page);
        }
        else
        {
            page += "\r\n";
            ifstream infile(path, mode);
            string line;
            while (getline(infile, line))
            {
                page += line;
            }
            write(fd, page.c_str(), page.length());
        }
    }

    close(fd);
    return comm_fd;
}

void sendBinary(string file, string image_type, int fd, string page)
{
    vector<char> buffer;
    page += "Content-Type: image/" + image_type + "\r\n\r\n";
    write(fd, page.c_str(), page.length());
    FILE *file_stream = fopen(file.c_str(), "rb");
    size_t file_size;
    if (file_stream != nullptr)
    {
        fseek(file_stream, 0, SEEK_END);
        long file_length = ftell(file_stream);
        rewind(file_stream);

        buffer.resize(file_length);

        file_size = fread(&buffer[0], 1, file_length, file_stream);
    }
    FILE *fp = fdopen(fd, "wb");
    fwrite(buffer.data(), 1, buffer.size(), fp);
}
