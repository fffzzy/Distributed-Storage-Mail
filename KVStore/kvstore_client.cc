#include "common.h"
#include "kvstore.grpc.pb.h"
#include "regex"

namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

const std::regex regex_get("get (.+) (.+)");
const std::regex regex_put("put (.+) (.+) (.+)");
const std::regex regex_cput("cput (.+) (.+) (.+) (.+)");
const std::regex regex_delete("delete (.+) (.+)");

class KVStoreClient {
 public:
  KVStoreClient(std::string kvstore_addr);

  // Read commands from STDIN.
  void Run();

 private:
  std::string GetNodeAddr(const std::string& row, const std::string& col);
  std::string Get(const std::string& row, const std::string& col);
  void Put(const std::string& row, const std::string& col,
           const std::string& value);
  void CPut(const std::string& row, const std::string& col,
            const std::string& cur_value, const std::string& new_value);
  void Delete(const std::string& row, const std::string& col);

 private:
  std::unique_ptr<KVStoreMaster::Stub> kvstore_master_;
};

KVStoreClient::KVStoreClient(std::string kvstore_addr) {
  // Initialize stubs to communicate with kvstore_master node.
  kvstore_master_ = KVStoreMaster::NewStub(
      grpc::CreateChannel(kvstore_addr, grpc::InsecureChannelCredentials()));
}

std::string KVStoreClient::GetNodeAddr(const std::string& row,
                                       const std::string& col) {
  ClientContext context;
  FetchNodeRequest req;
  FetchNodeResponse res;

  req.set_row(row);
  req.set_col(col);
  Status status = kvstore_master_->FetchNodeAddr(&context, req, &res);
  if (status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      return res.addr();
    }
  }
  return "";
}

std::string KVStoreClient::Get(const std::string& row, const std::string& col) {
  std::string addr = GetNodeAddr(row, col);
  ClientContext context;
  KVRequest req;
  KVResponse res;

  auto stub = KVStoreNode::NewStub(
      grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

  req.mutable_get_request()->set_row(row);
  req.mutable_get_request()->set_col(col);

  Status status = stub->Execute(&context, req, &res);
  if (status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      fprintf(stderr, "[Get] value: %s\n", res.message().c_str());
      return res.message();
    } else {
      fprintf(stderr, "[Get] operation failed: %s\n", res.message().c_str());
      return "";
    }
  }

  fprintf(stderr, "[Get] grpc failed.\n");
  return "";
}

void KVStoreClient::Put(const std::string& row, const std::string& col,
                        const std::string& value) {
  std::string addr = GetNodeAddr(row, col);
  ClientContext context;
  KVRequest req;
  KVResponse res;

  auto stub = KVStoreNode::NewStub(
      grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

  req.mutable_put_request()->set_row(row);
  req.mutable_put_request()->set_col(col);
  req.mutable_put_request()->set_value(value);

  Status status = stub->Execute(&context, req, &res);
  if (status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      fprintf(stderr, "[Put] Succeeds: %s\n", res.message().c_str());
      return;
    } else {
      fprintf(stderr, "[Put] operation failed: %s\n", res.message().c_str());
      return;
    }
  }

  fprintf(stderr, "[Put] grpc failed.\n");
  return;
}

void KVStoreClient::CPut(const std::string& row, const std::string& col,
                         const std::string& cur_value,
                         const std::string& new_value) {
  std::string addr = GetNodeAddr(row, col);
  ClientContext context;
  KVRequest req;
  KVResponse res;

  auto stub = KVStoreNode::NewStub(
      grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

  req.mutable_cput_request()->set_row(row);
  req.mutable_cput_request()->set_col(col);
  req.mutable_cput_request()->set_cur_value(cur_value);
  req.mutable_cput_request()->set_new_value(new_value);

  Status status = stub->Execute(&context, req, &res);
  if (status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      fprintf(stderr, "[Put] Succeeds: %s\n", res.message().c_str());
      return;
    } else {
      fprintf(stderr, "[Put] operation failed: %s\n", res.message().c_str());
      return;
    }
  }

  fprintf(stderr, "[Put] grpc failed.\n");
  return;
}

void KVStoreClient::Delete(const std::string& row, const std::string& col) {
  std::string addr = GetNodeAddr(row, col);
  ClientContext context;
  KVRequest req;
  KVResponse res;

  auto stub = KVStoreNode::NewStub(
      grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

  req.mutable_delete_request()->set_row(row);
  req.mutable_delete_request()->set_col(col);

  Status status = stub->Execute(&context, req, &res);
  if (status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
      fprintf(stderr, "[Delete] Succeeds: %s\n", res.message().c_str());
      return;
    } else {
      fprintf(stderr, "[Delete] operation failed: %s\n", res.message().c_str());
      return;
    }
  }

  fprintf(stderr, "[Delete] grpc failed.\n");
  return;
}

void KVStoreClient::Run() {
  std::smatch sm;
  for (std::string line; std::getline(std::cin, line);) {
    if (std::regex_match(line, sm, regex_get)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      std::string value = Get(row, col);
    } else if (std::regex_match(line, sm, regex_put)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      std::string value = sm[3].str();
      Put(row, col, value);
    } else if (std::regex_match(line, sm, regex_cput)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      std::string curr_value = sm[3].str();
      std::string new_value = sm[4].str();
      CPut(row, col, curr_value, new_value);
    } else if (std::regex_match(line, sm, regex_delete)) {
      std::string row = sm[1].str();
      std::string col = sm[2].str();
      Delete(row, col);
    } else {
      fprintf(stderr, "Unpported commands: %s\n", line.c_str());
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "[Command Line Format] ./kvstore_client <master_addr>\n");
    exit(-1);
  }

  KVStoreClient client(argv[1]);
  client.Run();
}
