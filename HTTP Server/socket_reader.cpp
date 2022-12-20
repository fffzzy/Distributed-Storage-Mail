#include "socket_reader.hpp"

bool SocketReader::fillBuffer(int load_size = 1100) {
  cout << "Reload buffer here" << endl;
  int curr_size = buffer.size();
  buffer.resize(curr_size + load_size);
  int size_read = recv(fd, &buffer[curr_size], load_size, 0);
  if (size_read == -1 || size_read == 0) {
    buffer.resize(curr_size);
    cout << "No data coming in but still want to read" << endl;
    return false;
  }
  buffer.resize(curr_size + size_read);
  return true;
}

int SocketReader::hasLine() {
  for (int i = 0; i != buffer.size(); ++i) {
    if (buffer[i] == '\n') {
      cout << "Line length: " << i << endl;
      return i;
    }
  }
  return -1;
}

bool SocketReader::readLine(string &line) {
  int index;
  while ((index = hasLine()) == -1) {
    if (!fillBuffer()) {
      return false;
    }
  }
  cout << line << endl;

  line = string(buffer.begin(), buffer.begin() + 1 + index);
  buffer.erase(buffer.begin(), buffer.begin() + 1 + index);
  return true;
}

bool SocketReader::readBulk(int size, string &data) {
  while (size - buffer.size() > 1100) {
    cout << "This request body is bigger than 1100 bytes, reloading once more"
         << endl;
    fillBuffer();
  }
  if (size > buffer.size()) {
    cout << "This request body is just a little larger than current buffer (0 "
            "< size < 1100 bytes)"
         << endl;
    fillBuffer(size - buffer.size());
    data = buffer;
    buffer = "";
  } else {
    cout
        << "This request body is smaller than current buffer, no refill needed."
        << endl;
    data = buffer.substr(0, size);
    cout << "Data extracted: " << data << endl;
    buffer.erase(buffer.begin(), buffer.begin() + size);
  }
  return true;
}

bool SocketReader::readData(string &header, string &data) {
  string matcher = "Content-Length: ";
  size_t index = header.find(matcher);
  if (index == -1) {
    cout << "No Content-Length detected" << endl;
    return false;
  } else {
    size_t end = header.find("\r\n", index);
    string body_size_string =
        header.substr(index + matcher.size(), end - index - matcher.size());
    cout << body_size_string << endl;
    int body_size = stoi(body_size_string);
    readBulk(body_size, data);
    return true;
  }
}

void SocketReader::extractHeader(string &header) {
  string line;
  cout << "Preparing to read lines" << endl;
  bool not_blocked = readLine(line);
  if (!not_blocked) {
    cout << "Is blocked from the first line" << endl;
  }

  while (not_blocked && line != "\r\n") {
    header += line;
    not_blocked = readLine(line);
  }
  return;
}