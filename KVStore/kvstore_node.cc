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
  // tablet_ = std::make_unique<Tablet>();
  // tablet_->path = GetTabletPath();
  // tablet_->map =
  //     std::unordered_map<std::string,
  //                        std::unordered_map<std::string, std::string>>();
}

Status KVStoreNodeImpl::Execute(ServerContext* context,
                                const KVRequest* request, KVResponse* respone) {
  switch (request->request_case()) {
    case KVRequest::RequestCase::kGetRequest: {
      // KVGet(&request->get_request(), respone);
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

void KVStoreNodeImpl::InitEnv() {
  // create tablet file and log
  system(("mkdir -p " + GetNodeDirPath(node_idx)).c_str());
  system(("rm -rf " + GetNodeDirPath(node_idx) + "/*").c_str());

  for (int i = 0; i < num_tablet_total; i++) {
    std::fstream tabletFile;
    tabletFile.open(GetTabletFilePath(node_idx, i),
                    std::fstream::in | std::fstream::out | std::fstream::trunc);
    tabletFile.close();
  }

  std::fstream tabletFile;
  tabletFile.open(GetLogFilePath(node_idx),
                  std::fstream::in | std::fstream::out | std::fstream::trunc);
  tabletFile.close();

  // create stub for master and peers
  master_stub = KVStoreMaster::NewStub(
      grpc::CreateChannel(master_addr, grpc::InsecureChannelCredentials()));
  for (int i = 0; i < peer_addr_vec.size(); i++) {
    peer_stub_vec.push_back(KVStoreNode::NewStub(grpc::CreateChannel(
        peer_addr_vec[i], grpc::InsecureChannelCredentials())));
  }
}

void KVStoreNodeImpl::KVGet(const KVRequest_KVGetRequest* request,
                            KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  VerboseLog(absl::StrCat("Receive Get for <", row, "><", col, ">"));

  int digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  // check if tablet is in memory
  Tablet* tablet = GetTabletFromMem(tablet_idx);

  // if tablet is mem, directly return its value, do not need to notify other
  // nodes
  if (tablet != NULL) {
    response->set_status(KVStatusCode::SUCCESS);
    response->set_message(tablet->map[row][col]);

    return;
  } else {
    // tablet is not found
    // firstly notify peers to switch tablet
    // TODO: fault tolerance
    for (int i = 0; i < peer_stub_vec.size(); i++) {
      KVRequest secondary_request;
      secondary_request.mutable_sget_request()->set_row(row);
      secondary_request.mutable_sget_request()->set_col(col);

      KVResponse secondary_response;
      ClientContext secondary_context;
      Status status = peer_stub_vec[i].get()->Execute(
          &secondary_context, secondary_request, &secondary_response);
      // TODO: check status and take corresponding actions
      if (status.ok()) {
        if (secondary_response.status() == KVStatusCode::SUCCESS) {
        } else if (secondary_response.status() == KVStatusCode::FAILURE) {
        }
      } else {
      }
      VerboseLog(absl::StrCat("Send SGet <", row, "><", col, "> to ",
                              peer_addr_vec[i]));
    }

    // load target tablet to mem
    // if vec is full, unload the first one
    if (tablet_mem_vec.size() == num_tablet_mem) {
      UnloadTablet();
    }
  }
}

Tablet* KVStoreNodeImpl::GetTabletFromMem(int tablet_idx) {
  for (int i = 0; i < tablet_mem_vec.size(); i++) {
    if (tablet_mem_vec[i]->tablet_idx == tablet_idx) {
      return tablet_mem_vec[i];
    }
  }

  return NULL;
}

void KVStoreNodeImpl::UnloadTablet() {
  // erase the first tablet from vec
  Tablet* table_to_unload = tablet_mem_vec.front();
  tablet_mem_vec.erase(tablet_mem_vec.begin());

  // write tablet to file
  WriteTabletToFile(table_to_unload);

  // clear log for this tablet
}

void KVStoreNodeImpl::VerboseLog(char* msg) {
  if (verbose) {
    fprintf(stdout, "%s\n", msg);
  }
}

void KVStoreNodeImpl::VerboseLog(std::string msg) {
  if (verbose) {
    fprintf(stdout, "%s\n", msg.c_str());
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

  node.InitEnv();

  std::string server_address(node.addr);

  ::grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&node);
  std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}