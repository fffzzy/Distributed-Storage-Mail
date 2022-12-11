#include "kvstore_node.h"

namespace KVStore {
const char* serverlist_path = "../../Config/serverlist.txt";
int num_replicas = 3;
int num_tablet_total = 5;
int num_tablet_mem = 3;

namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Request Queue

void KVGet(const KVRequest_KVGetRequest* request, KVResponse* response,
           KVStoreNode::Stub* stub) {
  // TODO: implement Get methods.

  // Example: Primary node creates Get method and sends to secondary nodes.
  // [protobuf oneof
  // reference](https://developers.google.com/protocol-buffers/docs/proto3#oneof)
  KVRequest req;
  req.mutable_get_request()->set_row(request->row());
  req.mutable_get_request()->set_col(request->col());
  KVResponse res;
  ClientContext context;
  Status status = stub->Execute(&context, req, &res);
  if (status.ok()) {
    if (res.status() == KVStatusCode::SUCCESS) {
    } else if (res.status() == KVStatusCode::FAILURE) {
    }
  } else {
  }

  response->set_status(KVStatusCode::SUCCESS);
  response->set_message("sah");
}

void KVPut(const KVRequest_KVPutRequest* request, KVResponse* response) {
  // TODO: implement Put methods.
}

void KVCPut(const KVRequest_KVCPutRequest* request, KVResponse* response) {
  // TODO: implement CPut methods.
}

void KVDelete(const KVRequest_KVDeleteRequest* request, KVResponse* response) {
  // TODO: implement Delete methods.
}

}  // namespace

KVStoreNodeImpl::KVStoreNodeImpl() {
  // Load tablet into memory.
  tablet_ = std::make_unique<Tablet>();
  tablet_->path = GetTabletPath();
  tablet_->map =
      std::unordered_map<std::string,
                         std::unordered_map<std::string, std::string>>();

  // Initialize master stubs.
  std::string master_addr = "0.0.0.0:10001";
  master_stub_ = KVStoreMaster::NewStub(
      grpc::CreateChannel(master_addr, grpc::InsecureChannelCredentials()));

  // TODO: initialize stubs to communicate with other replicas.
  std::string replica1_addr = "0.0.0.0:10003";
  node_stub1_ = KVStoreNode::NewStub(
      grpc::CreateChannel(replica1_addr, grpc::InsecureChannelCredentials()));
  // std::string replica2_addr = "0.0.0.0:10004";
  // node_stub2_ = KVStoreNode::NewStub(
  //     grpc::CreateChannel(replica2_addr,
  //     grpc::InsecureChannelCredentials()));
}

Status KVStoreNodeImpl::Execute(ServerContext* context,
                                const KVRequest* request, KVResponse* respone) {
  switch (request->request_case()) {
    case KVRequest::RequestCase::kGetRequest: {
      KVGet(&request->get_request(), respone, node_stub1_.get());
      break;
    }
    case KVRequest::RequestCase::kPutRequest: {
      KVPut(&request->put_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kCputRequest: {
      KVCPut(&request->cput_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kDeleteRequest: {
      KVDelete(&request->delete_request(), respone);
      break;
    }
    default: {
      respone->set_status(KVStatusCode::FAILURE);
      respone->set_message("-ERR unsupported mthods.");
    }
  }
  return Status::OK;
}

void KVStoreNodeImpl::ReadConfig() {
  FILE* file = fopen(serverlist_path, "r");
  if (file == NULL) {
    fprintf(stderr, "Cannot open  %s\n", serverlist_path);
    exit(-1);
  }
  // calculate idx range of cluster, inclusive
  int start = (node_idx - 1) / 3 * 3 + 1;
  int end = (node_idx - 1) / 3 * 3 + 3;

  char line[1000];
  int count = 0;
  while (fgets(line, sizeof(line), file)) {
    char* addr_line = strtok(line, ",\r\n");
    if (count == 0) {
      master_addr = std::string(addr_line, strlen(addr_line));
    } else if (count >= start && count <= end) {
      std::string address = std::string(addr_line, strlen(addr_line));
      if (count == node_idx) {
        addr = address;
      } else {
        peer_addr_vec.push_back(address);
      }
    }

    count++;
  }

  if (addr.empty()) {
    fprintf(stderr, "Invalid node idx\n");
    exit(-1);
  }
}

}  // namespace KVStore

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "[Command Line Format] ./kvstore_node <num> [-v]\n");
    exit(-1);
  }

  /* declare a node class */
  KVStore::KVStoreNodeImpl node;

  /* read from command line */
  int opt;
  while ((opt = getopt(argc, argv, "v")) != -1) {
    switch (opt) {
      case 'v':
        node.verbose = true;
        break;
      default:
        fprintf(stderr, "[Command Line Format] ./kvstore_node <num> [-v]\n");
        exit(-1);
        break;
    }
  }

  if (optind + 1 != argc) {
    fprintf(stderr, "[Command Line Format] ./kvstore_node <num> [-v]\n");
    exit(-1);
  }

  node.node_idx = atoi(argv[optind++]);
  if (node.node_idx < 1) {
    fprintf(stderr, "Invalid node idx\n");
    exit(-1);
  }

  node.ReadConfig();

  // check config
  std::cout << "self addr: " << node.addr << std::endl;
  std::cout << "master addr: " << node.master_addr << std::endl;
  std::cout << "peer addr: " << std::endl;
  for (int i = 0; i < node.peer_addr_vec.size(); i++) {
    std::cout << node.peer_addr_vec[i] << std::endl;
  }

  // std::string server_address("0.0.0.0:10002");

  // ::grpc::ServerBuilder builder;
  // builder.AddListeningPort(server_address,
  // grpc::InsecureServerCredentials()); builder.RegisterService(&node);
  // std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
  // std::cout << "Server listening on " << server_address << std::endl;
  // server->Wait();
}