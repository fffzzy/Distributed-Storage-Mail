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
      std::cout << "[Get] Success: " << response.message() << std::endl;
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
      std::cout << "[Put] Success: " << response.message() << std::endl;
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
      std::cout << "[CPut] Success: " << response.message() << std::endl;
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
      std::cout << "[Delete] Success: " << response.message() << std::endl;
    } else {
      std::cout << "[Delete] Failure: " << response.error_msg() << std::endl;
    }
  }

  void Test() {
    std::string row = "some_row";
    std::string col = "some_col";
    std::string prev_value = "some_prev_value";
    std::string value = "some_value";
    Get(row, col);
    Put(row, col, value);
    CPut(row, col, prev_value, value);
    Delete(row, col);
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