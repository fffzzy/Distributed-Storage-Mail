#include "mail_service.hpp"

#include "../KVStore/kvstore_client.h"

static bool is_verbose = true;
static vector<int> fds;
static vector<pthread_t> threads;
static int listen_fd;
static int port = 2500;
static string path;
static vector<string> users;
pthread_mutex_t lock_;

static KVStore::KVStoreClient kvstore_;

void MailService::startAccepting() {
  listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(port);
  bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  listen(listen_fd, 100);
  while (true) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    int comm_fd =
        accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddrlen);

    fds.push_back(comm_fd);
    // printf("Connection from %s\n", inet_ntoa(clientaddr.sin_addr));
    if (is_verbose) fprintf(stderr, "[%d] New connection\n", comm_fd);
    pthread_t p;
    threads.push_back(p);
    // Pram pra = {comm_fd, kvstore};
    // pthread_create(&threads.back(), NULL, mailWorker, (void *)&pra);
    pthread_create(&threads.back(), NULL, mailWorker, (void *)&comm_fd);
  }
}

void *mailWorker(void *comm_fd) {
  char buffer[1020] = {};
  char *head = buffer;
  char *tail;
  char *text_head;
  char *text_tail;
  char command[20];
  char message[1001];
  Status stage = Begin;
  string from;
  string to;
  vector<string> tos;
  string mail;
  //   bool is_quited = false;

  // all reply messages are listed here
  const char *OK = "250 OK\r\n";
  const char *READY = "220 localhost Service ready\r\n";
  const char *START = "354 Start mail input; end with <CRLF>.<CRLF>\r\n";
  const char *CLOSE = "221 localhost Service closing transmission channel\r\n";
  const char *SERVICE_ERR =
      "421 localhost Service not available, closing transmission channel\r\n";
  const char *UNKNOWN =
      "500 Syntax error, command unrecognized [This may include errors such as "
      "command line too long]\r\n";
  const char *PARAM_ERR = "501 Syntax error in parameters or arguments\r\n";
  const char *BAD_SEQ = "503 Bad sequence of commands\r\n";
  const char *HELO = "250 localhost\r\n";
  const char *MAILBOX_ERR =
      "550 Requested action not taken: mailbox unavailable [E.g., mailbox not "
      "found, no access]\r\n";
  const char *INPUT = "354 Start mail input; end with <CRLF>.<CRLF>\r\n";

  // Pram *pr = (Pram *)p;
  // int fd = pr->fd;
  // KVStoreClient kvstore = pr->kvstore;

  int fd = *(int *)comm_fd;
  write(fd, READY, strlen(READY));

  while (true) {
    read(fd, head, 1020 - strlen(buffer));
    while ((tail = strstr(buffer, "\r\n")) != NULL) {
      tail += 2 * sizeof(char);

      if (is_verbose)
        fprintf(stderr, "[%d] C: %.*s", fd, int(tail - buffer), buffer);

      memcpy(command, buffer, 5 * sizeof(char));
      command[5] = '\0';

      if (!strcasecmp(command, "HELO ")) {
        if (stage == Begin || Helo) {
          memcpy(message, HELO, strlen(HELO) + 1);
          stage = Helo;
        } else {
          memcpy(message, BAD_SEQ, strlen(BAD_SEQ) + 1);
        }
      } else if (!strcasecmp(command, "MAIL ")) {
        // double check whether the command is strictly `MAIL FROM:<>`
        memcpy(command, buffer, 10 * sizeof(char));
        command[10] = '\0';
        if (!strcasecmp(command, "MAIL FROM:")) {
          // check stage
          if (stage == Helo) {
            // save sender information to `from`
            text_head = strstr(buffer, "<");
            text_tail = strstr(buffer, ">");
            // check whether the sender address is in `<>`
            if (text_tail && text_head) {
              memcpy(message, OK, strlen(OK) + 1);
              // saved with `<>`
              text_tail++;
              char tmp[int(text_tail - text_head) + 1];
              memcpy(tmp, text_head, int(text_tail - text_head));
              tmp[int(text_tail - text_head)] = '\0';
              from = string(tmp);
              from = from.substr(1, from.size() - 2);
              stage = Mail;
            } else {
              memcpy(message, PARAM_ERR, strlen(PARAM_ERR) + 1);
            }
          } else {
            memcpy(message, BAD_SEQ, strlen(BAD_SEQ) + 1);
          }
        }
        // start with `MAIL` but not `MAIL FROM:`, still illegal.
        else {
          memcpy(message, UNKNOWN, strlen(UNKNOWN) + 1);
        }
      } else if (!strcasecmp(command, "RCPT ")) {
        // double check whether the command is strictly `MAIL FROM:<>`
        memcpy(command, buffer, 8 * sizeof(char));
        command[8] = '\0';
        if (!strcasecmp(command, "RCPT TO:")) {
          // check stage
          if (stage == Mail || Rcpt) {
            // get receiver's email host
            text_head = strstr(buffer, "@");
            text_tail = strstr(buffer, ">");

            if (text_tail && text_head) {
              text_head++;
              char tmp[200];
              memcpy(tmp, text_head, int(text_tail - text_head));
              tmp[int(text_tail - text_head)] = '\0';
              if (!strcmp(tmp, "localhost")) {
                // get receiver's name
                text_head = strstr(buffer, "<");
                text_tail = strstr(buffer, ">");
                text_head++;
                memcpy(tmp, text_head, int(text_tail - text_head));
                tmp[int(text_tail - text_head)] = '\0';
                to = string(tmp);
                tos.push_back(to);
                stage = Rcpt;
                memcpy(message, OK, strlen(OK) + 1);
              }
              // reject emails to other locations
              else {
                memcpy(message, MAILBOX_ERR, strlen(MAILBOX_ERR) + 1);
              }
            }
            // email doesn't contain `@` or `>`
            else {
              memcpy(message, PARAM_ERR, strlen(PARAM_ERR) + 1);
            }
          } else {
            memcpy(message, BAD_SEQ, strlen(BAD_SEQ) + 1);
          }
        }
        // start with `RCPT` but not `RCPT TO:`, still illegal.
        else {
          memcpy(message, UNKNOWN, strlen(UNKNOWN) + 1);
        }
      } else if (!strcasecmp(command, "DATA\r")) {
        if (stage == Rcpt) {
          memcpy(message, INPUT, strlen(INPUT) + 1);
          stage = Data;
        } else {
          memcpy(message, BAD_SEQ, strlen(BAD_SEQ) + 1);
        }
      } else if (stage == Data) {
        if (strcmp(buffer, ".\r\n") == 0) {
          for (auto r : tos) {
            // grab mutexes to control thread access first
            pthread_mutex_lock(&lock_);
            time_t t =
                chrono::system_clock::to_time_t(chrono::system_clock::now());

            string username = r.substr(0, r.find("@"));
            json new_mail;

            new_mail["sender"] = from;
            for (auto sender : tos) {
              new_mail["recipients"].push_back(sender);
            }
            string matcher = "Subject: ";
            size_t title_begin = mail.find(matcher);
            size_t sep = mail.find("\r\n", title_begin);
            new_mail["subject"] =
                mail.substr(title_begin + matcher.size(),
                            sep - title_begin - matcher.size());
            new_mail["content"] = mail.substr(sep + 2);
            string time = ctime(&t);
            new_mail["time"] = time.substr(0, time.size() - 1);

            cout << "To save the new email to mailbox" << endl;
            auto get_res = kvstore_.Get(username, "mails");
            string mail_string = get_res.ok() ? *get_res : "";
            if (mail_string.empty()) {
              cout << username << " has no email in the mailbox currently!"
                   << endl;
              mail_string = "[]";
            }
            json mailList = json::parse(mail_string);

            if (mailList.empty()) {
              new_mail["mailId"] = 1;
            } else {
              int mailId = mailList[mailList.size() - 1]["mailId"];
              new_mail["mailId"] = mailId + 1;
            }

            mailList.push_back(new_mail);
            auto put_res = kvstore_.Put(username, "mails", mailList.dump());
            if (!put_res.ok()) {
              fprintf(stderr,
                      "failed to put username & mails into kvstore: %s\n",
                      put_res.ToString().c_str());
            }

            pthread_mutex_unlock(&lock_);
          }
          memcpy(message, OK, strlen(OK) + 1);
          stage = Done;
          // clean up
          from.clear();
          tos.clear();
          mail.clear();
        } else {
          mail += string(buffer, tail - buffer);
          message[0] = '\0';
        }
      } else if (!strcasecmp(command, "QUIT\r")) {
        // reply to client immediately and close the file descriptor.
        write(fd, CLOSE, strlen(CLOSE));
        if (is_verbose) {
          fprintf(stderr, "[%d] S: %s", fd, message);
          fprintf(stderr, "[%d] Connection closed\n", fd);
        }
        close(fd);
        return NULL;
      } else if (!strcasecmp(command, "RSET\r")) {
        memcpy(message, OK, strlen(OK) + 1);
        from.clear();
        tos.clear();
        mail.clear();
        stage = Helo;
      } else if (!strcasecmp(command, "NOOP\r")) {
        memcpy(message, OK, strlen(OK) + 1);
      } else {
        memcpy(message, UNKNOWN, strlen(UNKNOWN) + 1);
      }

      write(fd, message, strlen(message));

      if (is_verbose) fprintf(stderr, "[%d] S: %s", fd, message);

      head = buffer;
      while (head != tail) {
        *head = '\0';
        head++;
      }
      head = buffer;

      /* move the rest characters to the beginning. */
      while (*tail != '\0') {
        *head = *tail;
        *tail = '\0';
        head++;
        tail++;
      }
    }
    while (*head != '\0') head++;

    // fprintf(stderr, "%d\n", *head);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  MailService mail_service;
  mail_service.startAccepting();
}