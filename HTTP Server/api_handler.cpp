#include "api_handler.hpp"

void APIHandler::parsePost() {
  if (header.substr(0, 6) == "signup") {
    signup();
  } else if (header.substr(0, 5) == "login") {
    login();
  } else if (header.substr(0, 6) == "logout") {
    logout();
  } else {
  }
}

void APIHandler::parseGet() {
  if (header.substr(0, 6) == "signup") {
    signup();
  } else if (header.substr(0, 5) == "login") {
    login();
  } else if (header.substr(0, 6) == "logout") {
    logout();
  } else {
  }
}

void APIHandler::parseDelete() {
  if (header.substr(0, 6) == "signup") {
    signup();
  } else if (header.substr(0, 5) == "login") {
    login();
  } else if (header.substr(0, 6) == "logout") {
    logout();
  } else {
  }
}

void APIHandler::signup() {
  if (!data.contains("username")) {
    cout << "No username found in the request body" << endl;
  } else {
    string username = data["username"];
    string password = data["password"];
    if (is_verbose) {
      cout << "username: " << username << endl;
      cout << "password: " << password << endl;
    }
    // kvstore.Put(username, "password", password);
    kvstore_client_.Put(username, "password", password);

    string page = "HTTP/1.1 201 successfully created\r\n\r\n";
    if (is_verbose) cout << page << endl;
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::login() {
  if (!data.contains("username")) {
    cout << "No username found in the request body" << endl;
  } else {
    string username = data["username"];
    string password = data["password"];
    if (is_verbose) {
      cout << "username: " << username << endl;
      cout << "password: " << password << endl;
    }
    // string real_pw = kvstore.Get(username, "password");

    auto res = kvstore_client_.Get(username, "password");
    string real_pw = res.ok() ? res->data() : "";

    string page;
    if (real_pw == password) {
      string cookie = username + "=" + urlEncode(username);
      //   kvstore.Put(username, "cookie", cookie);
      kvstore_client_.Put(username, "cookie", cookie);

      page =
          "HTTP/1.1 200 success\r\nSet-Cookie: " + cookie + "; path=/ \r\n\r\n";
      if (is_verbose) cout << page << endl;
    } else {
      page = "HTTP/1.1 401 unauthorized if pwd incorrect\r\n\r\n";
      cout << "your pw: " + password << "\n"
           << "real pw: " + real_pw << endl;
    }
    write(fd, page.c_str(), page.length());
  }
}

void APIHandler::logout() {
  string username = checkCookie();
  if (!username.empty()) {
    // kvstore.Delete(username, "cookie");
    kvstore_client_.Delete(username, "cookie");
  }
  string page =
      "HTTP/1.1 200 success\r\nSet-Cookie: " + username +
      "=deleted; path=/; expires=Thu, Jan 01 1970 00:00:00 UTC; \r\n\r\n";
  write(fd, page.c_str(), page.length());
}

string APIHandler::checkCookie() {
  string cookie_matcher = "Cookie: ";
  size_t cookie_start = header.find(cookie_matcher);
  if (cookie_start != string::npos) {
    cookie_start += cookie_matcher.size();
    size_t user_name_end_index = header.find("=", cookie_start);
    string username =
        header.substr(cookie_start, user_name_end_index - cookie_start);

    size_t cookie_end = header.find("\r", cookie_start);
    string cookie_value =
        header.substr(user_name_end_index + 1, cookie_end - cookie_start - 1);
    // string real_cookie = kvstore.Get(username, "cookie");
    auto res = kvstore_client_.Get(username, "cookie");
    string real_cookie = res.ok() ? res->data() : "";

    if (real_cookie == cookie_value) {
      return username;
    } else {
      return "";
    }
  } else {
    return "";
  }
}

string urlEncode(string str) {
  string new_str = "";
  char c;
  int ic;
  const char *chars = str.c_str();
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