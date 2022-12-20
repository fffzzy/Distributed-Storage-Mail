#include "server.hpp"

static bool is_verbose = false;
static bool is_shut_down = false;
static vector<int> fds;
static vector<pthread_t> threads;
static int listen_fd;
static int port;
static sockaddr_in backend_coordinator_addr;
static sockaddr_in self_addr;
static string page_root = "../react";

struct args {
  APIHandler *handler;
  int comm_fd;
};

int main(int argc, char *argv[]) {
  APIHandler api_handler(is_verbose);
  // signal(SIGINT, sigHandler);
  parseInput(argc, argv);

  listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(port);
  while (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    servaddr.sin_port = htons(--port);
  }
  cout << "Server start a port at: " << port << endl;
  listen(listen_fd, 100);
  while (!is_shut_down) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    int comm_fd =
        accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (!is_shut_down) {
      fds.push_back(comm_fd);
      // printf("Connection from %s\n", inet_ntoa(clientaddr.sin_addr));
      if (is_verbose) fprintf(stderr, "[%d] New connection\n", comm_fd);
      pthread_t p;
      threads.push_back(p);
      args thread_arg;
      thread_arg.comm_fd = comm_fd;
      thread_arg.handler = &api_handler;
      pthread_create(&threads.back(), NULL, messageWorker, (void *)&thread_arg);
    }
  }

  return 0;
}

void parseInput(int argc, char *argv[]) {
  int opt;

  while ((opt = getopt(argc, argv, "p:v")) != -1) {
    switch (opt) {
      case 'v':
        is_verbose = true;
        break;
      case 'p':
        port = atoi(optarg);
        break;

      default: /* '?' */
        fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
}

void sigHandler(int num) {
  char SHUTDOWN[] = "-ERR Server shutting down\r\n";

  is_shut_down = true;
  close(listen_fd);
  for (auto fd : fds) {
    // write(fd, SHUTDOWN, strlen(SHUTDOWN));
    close(fd);
    if (is_verbose) {
      // fprintf(stderr, "fd: %d\n", fd);
      fprintf(stderr, "[%d] Connection closed\n", fd);
    }
  }
  for (auto thread : threads) {
    pthread_kill(thread, 0);
  }
}

sockaddr_in parseSockaddr(string s) {
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

void *messageWorker(void *thread_args) {
  APIHandler *handler = ((args *)thread_args)->handler;
  // while (true)
  // {
  //     vector<char> buffer(BUFF_SIZE);

  //     int read_len = recvfrom(sock, buffer.data(), BUFF_SIZE, 0,
  //                             (struct sockaddr *)&source_address,
  //                             &source_address_len);
  //     string str;
  //     str.append(buffer.begin(), buffer.begin() + read_len);
  // }
  size_t buffer_size = 50000;
  char buffer[buffer_size] = {};

  int fd = ((args *)thread_args)->comm_fd;
  read(fd, buffer, buffer_size - 1);
  // if (strlen(buffer) != buffer_size - 1)
  size_t len = strlen(buffer);
  if (is_verbose) {
    if (len > 1000) {
      cout << "Http request size: " << len << endl;
    } else {
      printf("%s\n", buffer);
    }
  }

  if (!strncmp(buffer, "GET / ", 6)) {
    homepage(fd);
  } else if (!strncmp(buffer, "GET /api/", 9)) {
    string buf(buffer);
    buf = buf.substr(9);
    handler->parseGet(buf, fd);
  } else if (!strncmp(buffer, "POST /api/", 10)) {
    string buf(buffer);
    buf = buf.substr(10);
    handler->parsePost(buf, fd);
  } else if (!strncmp(buffer, "DELETE /api/", 12)) {
    string buf(buffer);
    buf = buf.substr(12);
    handler->parseDelete(buf, fd);
  } else if (!strncmp(buffer, "GET", 3)) {
    string buf(buffer);
    int head = buf.find(" ");
    string tmp = buf.substr(head + 1);
    int tail = tmp.find(" ");
    string file = tmp.substr(0, tail);
    string page = "HTTP/1.1 200 OK\r\n";
    string file_type = file.substr(file.find_last_of(".") + 1);
    string path = page_root + file;
    ios_base::openmode mode = ios_base::in;

    ifstream checkExists(path);
    if (!checkExists) {
      homepage(fd);
    } else {
      if (file_type == "jpg" | file_type == "png") {
        sendBinary(path, file_type, fd, page);
      } else {
        page += "\r\n";
        ifstream infile(path, mode);
        string line;
        while (getline(infile, line)) {
          page += line;
        }
        write(fd, page.c_str(), page.length());
      }
    }
  }

  close(fd);
  // return comm_fd;
  return NULL;
}

void sendBinary(string file, string image_type, int fd, string page) {
  vector<char> buffer;
  page += "Content-Type: image/" + image_type + "\r\n\r\n";
  write(fd, page.c_str(), page.length());
  FILE *file_stream = fopen(file.c_str(), "rb");
  size_t file_size;
  if (file_stream != nullptr) {
    fseek(file_stream, 0, SEEK_END);
    long file_length = ftell(file_stream);
    rewind(file_stream);

    buffer.resize(file_length);

    file_size = fread(&buffer[0], 1, file_length, file_stream);
  }
  FILE *fp = fdopen(fd, "wb");
  fwrite(buffer.data(), 1, buffer.size(), fp);
}

void homepage(int fd) {
  string page = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

  ifstream infile(page_root + "/index.html");
  string line;
  while (getline(infile, line)) {
    page += line;
  }
  write(fd, page.c_str(), page.length());
}