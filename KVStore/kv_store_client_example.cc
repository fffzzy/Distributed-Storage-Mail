#include <grpcpp/grpcpp.h>

#include "common.h"
#include "kv_store.grpc.pb.h"

namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class KVStoreClientExample {
 public:
  KVStoreClientExample(std::shared_ptr<Channel> channel)
      : stub_(KVStoreService::NewStub(channel)){};

  void Get(std::string row, std::string col) {
    KVGetRequest request;
    request.set_row(row);
    request.set_col(col);

    KVGetResponse response;
    ClientContext context;

    Status status = stub_->Get(&context, request, &response);
    if (status.ok()) {
      if (response.status() == KVResStatus::SUCCESS) {
        std::cout << "[Get-OK]: " << response.message() << std::endl;
      } else {
        std::cout << "[Get-ERR]: " << response.error_msg() << std::endl;
      }
    } else {
      std::cout << "[Get] Failure: " << response.error_msg() << std::endl;
    }
  }

  void Put(std::string row, std::string col, std::string value) {
    KVPutRequest request;
    request.set_row(row);
    request.set_col(col);
    request.set_value(value);

    KVPutResponse response;
    ClientContext context;

    Status status = stub_->Put(&context, request, &response);
    if (status.ok()) {
      if (response.status() == KVResStatus::SUCCESS) {
        std::cout << "[Put-OK]: " << response.message() << std::endl;
      } else {
        std::cout << "[Put-ERR]: " << response.error_msg() << std::endl;
      }
    } else {
      std::cout << "[Put] Failure: " << response.error_msg() << std::endl;
    }
  }

  void CPut(std::string row, std::string col, std::string prev_value,
            std::string value) {
    KVCPutRequest request;
    request.set_row(row);
    request.set_col(col);
    request.set_prev_value(prev_value);
    request.set_value(value);

    KVCPutResponse response;
    ClientContext context;

    Status status = stub_->CPut(&context, request, &response);
    if (status.ok()) {
      if (response.status() == KVResStatus::SUCCESS) {
        std::cout << "[CPut-OK]: " << response.message() << std::endl;
      } else {
        std::cout << "[CPut-ERR]: " << response.error_msg() << std::endl;
      }
    } else {
      std::cout << "[CPut] Failure: " << response.error_msg() << std::endl;
    }
  }

  void Delete(std::string row, std::string col) {
    KVDeleteRequest request;
    request.set_row(row);
    request.set_col(col);

    KVDeleteResponse response;
    ClientContext context;

    Status status = stub_->Delete(&context, request, &response);
    if (status.ok()) {
      if (response.status() == KVResStatus::SUCCESS) {
        std::cout << "[Delete-OK]: " << response.message() << std::endl;
      } else {
        std::cout << "[Delete-ERR]: " << response.error_msg() << std::endl;
      }
    } else {
      std::cout << "[Delete] Failure: " << response.error_msg() << std::endl;
    }
  }

  void Test() {
    Put("Alice", "Alice-Col1", "Hello, I'm Alice\n");
    Put("Alice", "Alice-Col2",
        "This is the second col for Alice(Before delete)!\n");
    Put("Bob", "Bob-Col1", "This is the first col for Bob (old)\n");
    Put("Bob", "Bob-Col2", "Test splited\n content\n");
    Put("Bob", "Bob-Col3", "\tThis is the third col for Bob\n");
    Delete("Alice", "Alice-Col2");
    Delete("Alice", "Alice-Col1");
    CPut("Bob", "Bob-Col1", "This is the first col for Bob (old)\n",
         "This is the first col for Bob (new)\n");
    Put("Bob", "Bob-Col3", "This is the third col for Bob(no indentation)\n");
    Put("Cindy", "Cindy-Col1", "Hello World, I'm Cindy.");
    Put("Cindy", "Cindy-Col2", "This is Cindy's second col");
    // -------------------------------------
    Get("Alice", "Alice-Col1");
    Get("Alice", "Alice-Col2");
    Get("Bob", "Bob-Col1");
    Get("Bob", "Bob-Col2");
    Get("Bob", "Bob-Col3");
    Get("Cindy", "Cindy-Col1");
    Get("Cindy", "Cindy-Col2");
    Get("Cindy", "Cindy-Col3");
  }

 private:
  // stub used to connect to the KVStoreService
  std::unique_ptr<KVStoreService::Stub> stub_;
};
}  // namespace

int main(int argc, char** argv) {
  std::string target_addr = "localhost:10000";
  KVStoreClientExample client(
      grpc::CreateChannel(target_addr, grpc::InsecureChannelCredentials()));

  client.Test();
};