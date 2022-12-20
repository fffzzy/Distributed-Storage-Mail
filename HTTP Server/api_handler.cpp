#include "api_handler.hpp"

void APIHandler::parseGet(string buf, int fd) {
  size_t division = buf.find("\r\n\r\n");
  string header = buf.substr(0, division + 2);
  string body = buf.substr(division + 4);

  if (header.substr(0, 4) == "mail") {
    getEmailList(fd, header);
  } else if (header.substr(0, 14) == "drive/download") {
    downloadFile(fd, header);
  } else if (header.substr(0, 5) == "drive") {
    getFiles(fd, header);
  } else if (header.substr(0, 7) == "backend") {
    checkBackend(fd);
  } else if (header.substr(0, 8) == "frontend") {
    checkFrontend(fd);
  } else {
  }
}

void APIHandler::parsePost(string buf, int fd) {
  size_t division = buf.find("\r\n\r\n");
  string header = buf.substr(0, division + 2);
  string body = buf.substr(division + 4);
  json data;
  string chunk;
  if (body.find("{") == string::npos) {
    data = nullptr;
    chunk = body;
  } else if (body.size() < 10000) {
    data = json::parse(body);
    chunk = body;
  } else {
    data = nullptr;
    chunk = body;
  }
  if (header.substr(0, 6) == "signup") {
    signup(fd, data);
  } else if (header.substr(0, 5) == "login") {
    login(fd, data);
  } else if (header.substr(0, 14) == "changepassword") {
    changePassword(fd, data);
  } else if (header.substr(0, 12) == "mail/compose") {
    sendEmail(fd, header, data);
  } else if (header.substr(0, 12) == "drive/upload") {
    uploadFile(fd, header, chunk);
  } else if (header.substr(0, 5) == "drive") {
    changeFiles(fd, header, data);
  } else if (header.substr(0, 7) == "suspend") {
    suspendNode(fd, header);
  } else if (header.substr(0, 6) == "revive") {
    reviveNode(fd, header);
  } else if (header.substr(0, 12) == "showkeyvalue") {
    showKeyValue(fd, header);
  } else {
  }
}

void APIHandler::parseDelete(string buf, int fd) {
  size_t division = buf.find("\r\n\r\n");
  string header = buf.substr(0, division + 2);
  string body = buf.substr(division + 4);
  json data;
  string chunk;
  if (body.find("{") == string::npos) {
    data = nullptr;
    chunk = body;
  } else if (body.size() < 10000) {
    data = json::parse(body);
    chunk = body;
  } else {
    data = nullptr;
    chunk = body;
  }
  if (header.substr(0, 6) == "logout") {
    logout(fd, header);
  } else if (header.substr(0, 11) == "mail/delete") {
    deleteEmail(fd, header);
  } else if (header.substr(0, 12) == "drive/delete") {
    deleteFile(fd, header);
  } else {
  }
}

void APIHandler::signup(int fd, json& data) {
  if (!data.contains("username")) {
    cout << "No username found in the request body" << endl;
  } else {
    string username = data["username"];
    string password = data["password"];
    if (is_verbose) {
      cout << "username: " << username << endl;
      cout << "password: " << password << endl;
    }

    auto res = kvstore_.Put(username, "password", password);
    if (!res.ok()) {
      fprintf(stderr, "failed to put username & password into kvstore: %s\n",
              res.ToString().c_str());
    }
    string page = "HTTP/1.1 201 successfully created\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::changePassword(int fd, json& data) {
  if (!data.contains("username")) {
    cout << "No username found in the request body" << endl;
  } else {
    string username = data["username"];
    string new_password = data["new_password"];
    if (is_verbose) {
      cout << "username: " << username << endl;
      cout << "password: " << new_password << endl;
    }

    auto res = kvstore_.Put(username, "password", new_password);
    if (!res.ok()) {
      fprintf(stderr, "failed to put username & password into kvstore: %s\n",
              res.ToString().c_str());
    }
    string page = "HTTP/1.1 201 successfully created\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::login(int fd, json& data) {
  if (!data.contains("username")) {
    cout << "No username found in the request body" << endl;
  } else {
    string username = data["username"];
    string password = data["password"];
    if (is_verbose) {
      cout << "username: " << username << endl;
      cout << "password: " << password << endl;
    }

    auto res = kvstore_.Get(username, "password");
    string real_pw = res.ok() ? *res : "";
    string page;
    if (!password.empty() && real_pw == password) {
      time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
      string cookie = urlEncode(ctime(&t));
      auto res = kvstore_.Put(username, "cookie", cookie);
      if (!res.ok()) {
        fprintf(stderr, "failed to put username & cookie into kvstore: %s\n",
                res.ToString().c_str());
      }
      page = "HTTP/1.1 200 success\r\nSet-Cookie: " + username + "=" + cookie +
             "; path=/; expires=Thu, Jan 01 2023 00:00:00 UTC \r\n\r\n";
      if (is_verbose) cout << page << endl;
    } else {
      page = "HTTP/1.1 401 unauthorized if pwd incorrect\r\n\r\n";
      cout << "your pw: " + password << "\n"
           << "real pw: " + real_pw << endl;
    }
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::logout(int fd, string& header) {
  string username = checkCookie(header);
  if (!username.empty()) {
    auto res = kvstore_.Delete(username, "cookie");
    if (res.ok()) {
      fprintf(stderr, "failed to delete username & cookie: %s\n",
              res.ToString().c_str());
    }
  }
  string page =
      "HTTP/1.1 200 success\r\nSet-Cookie: " + username +
      "=deleted; path=/; expires=Thu, Jan 01 1970 00:00:00 UTC \r\n\r\n";
  write(fd, page.c_str(), page.length());
}

void APIHandler::sendEmail(int fd, string& header, json& data) {
  string username = checkCookie(header);

  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    string name, host;
    if (is_verbose) cout << data.dump(4) << endl;

    parseEmail(data["sender"], name, host);
    if (name != username) {
      cout << "Sender in Email is different from current user!" << endl;
    }

    time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    data["time"] = ctime(&t);

    cout << data << endl;

    for (const auto& recipient : data["recipients"]) {
      parseEmail(recipient, name, host);

      if (host == "localhost") {
        cout << "To send a email to localhost soon" << endl;
        auto res = kvstore_.Get(name, "mails");
        string mail_string = res.ok() ? *res : "";
        if (mail_string.empty()) {
          cout << name << " has no email in the mailbox currently!" << endl;
          mail_string = "[]";
        }
        json mailList = json::parse(mail_string);

        if (mailList.empty()) {
          data["mailId"] = 1;
        } else {
          int mailId = mailList[mailList.size() - 1]["mailId"];
          data["mailId"] = mailId + 1;
        }

        mailList.push_back(data);
        auto put_res = kvstore_.Put(name, "mails", mailList.dump());
        if (!put_res.ok()) {
          fprintf(stderr, "failed to put name & mails into kvstore: %s\n",
                  put_res.ToString().c_str());
        }
      } else {
        MailService email;
        email.sendOut(data);
      }
    }

    page = "HTTP/1.1 200 success\r\n\r\n";
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::getEmailList(int fd, string& header) {
  string username = checkCookie(header);

  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    auto res = kvstore_.Get(username, "mails");
    string mailList = res.ok() ? *res : "";
    if (mailList.empty()) {
      mailList = "[]";
    }
    page = "HTTP/1.1 200 success\r\n\r\n" + mailList;
    cout << page << endl;
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::deleteEmail(int fd, string& header) {
  string username = checkCookie(header);
  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    int mailId = stoi(extractValueFromHeader("mailId", header));

    bool matched = false;

    auto res = kvstore_.Get(username, "mails");
    string mail_string = res.ok() ? *res : "";
    if (res.ok()) {
      std::cout << "mail_string: " << *res << std::endl;
    } else {
      std::cout << res.status().ToString() << std::endl;
    }
    if (!mail_string.empty()) {
      json mailList = json::parse(mail_string);
      // find email with that mailId in the email list
      for (auto it = mailList.begin(); it != mailList.end(); it++) {
        if ((*it)["mailId"] == mailId) {
          mailList.erase(it);
          matched = true;
          break;
        }
      }

      auto put_res = kvstore_.Put(username, "mails", mailList.dump());
      if (!put_res.ok()) {
        fprintf(stderr, "failed to put username & mails inot kvstore: %s\n",
                put_res.ToString().c_str());
      }
    }
    if (matched) {
      page = "HTTP/1.1 200 success\r\n\r\n";
    } else {
      page = "HTTP/1.1 404 not found\r\n\r\n";
    }
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::getFiles(int fd, string& header) {
  string username = checkCookie(header);
  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    page = "HTTP/1.1 200 success\r\n\r\n";

    auto res = kvstore_.Get(username, "files");
    if (res.ok()) {
      std::cout << *res << std::endl;
    } else {
      std::cout << " ----- " << std::endl;
      std::cout << res.status().ToString() << std::endl;
    }
    string files = res.ok() ? *res : "";

    if (files.empty()) {
      page +=
          "{\"field\":\"0\",\"filename\":\"/"
          "\",\"filetype\":\"directory\",\"children\":[]}";
    } else {
      page += files;
    }
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::changeFiles(int fd, string& header, json& data) {
  string username = checkCookie(header);
  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    page = "HTTP/1.1 200 success\r\n\r\n";
    auto res = kvstore_.Put(username, "files", data.dump());
    if (!res.ok()) {
      fprintf(stderr, "failed to put username & files into kvstore: %s\n",
              res.ToString().c_str());
    }
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::uploadFile(int fd, string& header, string& chunk) {
  string username = checkCookie(header);
  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    string fileId = extractValueFromHeader("fileId", header);
    page = "HTTP/1.1 200 success\r\n\r\n";

    auto res = kvstore_.Put(username, fileId, chunk);
    if (!res.ok()) {
      fprintf(stderr, "failed to put username & fileId into kvstore: %s\n",
              res.ToString().c_str());
    }
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::downloadFile(int fd, string& header) {
  string username = checkCookie(header);
  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    page = "HTTP/1.1 200 success\r\n\r\n";
    string fileId = extractValueFromHeader("fileId", header);

    auto res = kvstore_.Get(username, fileId);
    string file = res.ok() ? *res : "";

    page += file;

    cout << "Current response size: " << page.size() << endl;
  }
  write(fd, page.c_str(), page.length());
}

void APIHandler::deleteFile(int fd, string& header) {
  string username = checkCookie(header);
  string page;
  if (username.empty()) {
    page = "HTTP/1.1 401 not logged in\r\n\r\n";
  } else {
    page = "HTTP/1.1 200 success\r\n\r\n";
    string fileId = extractValueFromHeader("fileId", header);

    auto res = kvstore_.Delete(username, fileId);
    if (!res.ok()) {
      fprintf(stderr, "failed to put username & fileId into kvstore: %s\n",
              res.ToString().c_str());
    }
  }
  cout << "page: " << page << endl;
  write(fd, page.c_str(), page.length());
}

void APIHandler::checkBackend(int fd) {
  auto res = console_.PollStatus();
  if (!res.ok()) {
    fprintf(stderr, "failed to poll status from console: %s\n",
            res.status().ToString().c_str());
  }
  // vector<vector<NodeInfo>> list = kvstore.PollStatus();
  json list_json;
  for (auto& cluster : *res) {
    json cluster_json;
    for (auto& node : cluster) {
      json node_json = {
          {"is_primary", node.is_primary},
          {"addr", node.addr},
          {"status", node.status},
      };
      cluster_json.push_back(node_json);
    }
    list_json.push_back(cluster_json);
  }
  string page = "HTTP/1.1 200 success\r\n\r\n";
  page += list_json.dump();
  if (is_verbose) cout << page << endl;
  write(fd, page.c_str(), page.length());
}

void APIHandler::checkFrontend(int fd) {}

void APIHandler::suspendNode(int fd, string& header) {
  string node_addr = extractValueFromHeader("node_addr", header);

  auto res = console_.Suspend(node_addr);
  if (!res.ok()) {
    fprintf(stderr, "failed to suspend %s: %s\n", node_addr.c_str(),
            res.ToString().c_str());
    string page = "HTTP/1.1 500 failure\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  } else {
    string page = "HTTP/1.1 200 success\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::reviveNode(int fd, string& header) {
  string node_addr = extractValueFromHeader("node_addr", header);
  auto res = console_.Revive(node_addr);
  if (!res.ok()) {
    fprintf(stderr, "failed to revive %s: %s\n", node_addr.c_str(),
            res.ToString().c_str());
    string page = "HTTP/1.1 500 failure\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  } else {
    string page = "HTTP/1.1 200 success\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::showKeyValue(int fd, string& header) {
  string node_addr = extractValueFromHeader("node_addr", header);
  auto res = console_.ShowKeyValue(node_addr);
  if (!res.ok()) {
    fprintf(stderr, "failed to revive %s: %s\n", node_addr.c_str(),
            res.status().ToString().c_str());
    string page = "HTTP/1.1 500 failure\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  } else {
    string page = "HTTP/1.1 200 success\r\n\r\n";
    page += *res;
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  }
}

string APIHandler::checkCookie(string& header) {
  string cookie_matcher = "Cookie: ";
  size_t cookie_start = header.find(cookie_matcher);
  if (cookie_start != string::npos) {
    cookie_start += cookie_matcher.size();
    size_t user_name_end_index = header.find("=", cookie_start);
    string username =
        header.substr(cookie_start, user_name_end_index - cookie_start);
    cout << "username in cookie:" << username << endl;
    size_t cookie_end = header.find("\r", cookie_start);
    string cookie_value = header.substr(user_name_end_index + 1,
                                        cookie_end - user_name_end_index - 1);
    auto res = kvstore_.Get(username, "cookie");

    string real_cookie = res.ok() ? *res : "";
    if (is_verbose) {
      cout << "Cookie in browser: " << cookie_value
           << "length: " << cookie_value.size() << endl;
      cout << "Cookie in database: " << real_cookie
           << "length: " << real_cookie.size() << endl;
    }
    if (!real_cookie.empty() && real_cookie == cookie_value) {
      return username;
    } else {
      if (is_verbose)
        cout << "The cookie sent by browser doesn't exist in database." << endl;
      return "";
    }
  } else {
    if (is_verbose) cout << "No cookie attached in the header." << endl;
    return "";
  }
}

string urlEncode(string str) {
  string new_str = "";
  char c;
  int ic;
  const char* chars = str.c_str();
  char bufHex[10];
  int len = strlen(chars);

  for (int i = 0; i < len; i++) {
    c = chars[i];
    ic = c;
    // uncomment this if you want to encode spaces with +
    /*if (c==' ') new_str += '+';
    else */
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
      new_str += c;
    else {
      sprintf(bufHex, "%X", c);
      if (ic < 16)
        new_str += "%0";
      else
        new_str += "%";
      new_str += bufHex;
    }
  }
  return new_str;
}

string APIHandler::extractValueFromHeader(string key, string& header) {
  string matcher = key + "=";
  size_t index = header.find(matcher);
  size_t end = header.find(" ", index);

  if (index == string::npos) {
    if (is_verbose) cout << "No " << key << " detected in the header." << endl;
    return "";
  } else {
    string value =
        header.substr(index + matcher.size(), end - index - matcher.size());
    if (is_verbose) cout << key << " detected in the header: " + value << endl;
    return value;
  }
}

void APIHandler::parseEmail(string email, string& user, string& host) {
  auto separator = email.find("@");
  user = email.substr(0, separator);
  host = email.substr(separator + 1);

  if (is_verbose) {
    cout << "Email: " + email + ", User: " + user + ", Host: " + host << endl;
  }
}