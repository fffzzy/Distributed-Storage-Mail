#include "kvstore_node.h"

namespace KVStore {
const char* serverlist_path = "../../Config/serverlist.txt";
int num_replicas = 3;
int num_tablet_total = 5;
int num_tablet_mem = 3;

namespace {
using ::google::protobuf::Empty;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Request Queue

}  // namespace

KVStoreNodeImpl::KVStoreNodeImpl() {}

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
  Tablet* tablet_to_unload = tablet_mem_vec.front();
  tablet_mem_vec.erase(tablet_mem_vec.begin());

  // write tablet to file
  WriteTabletToFile(tablet_to_unload);

  // clear log for this tablet
  std::fstream logFile;
  logFile.open(GetLogFilePath(node_idx, tablet_to_unload->tablet_idx),
               std::fstream::in | std::fstream::out | std::fstream::trunc);
  logFile.close();

  // delete this tablet
  delete tablet_to_unload;
}

Tablet* KVStoreNodeImpl::LoadTablet(int tablet_idx) {
  // load tablet with idx tablet_idx
  Tablet* tablet = LoadTabletFromFile(node_idx, tablet_idx);

  // insert the new tablet to tablet_mem_vec
  assert(tablet_mem_vec.size() < num_tablet_mem);
  tablet_mem_vec.push_back(tablet);

  return tablet;
}

Status KVStoreNodeImpl::CheckHealth(ServerContext* context,
                                    const Empty* request, Empty* response) {
  fprintf(stderr, "[Health Check] responding back to master ... \n");
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
    std::fstream tablet_file;
    tablet_file.open(GetTabletFilePath(node_idx, i), std::fstream::in |
                                                         std::fstream::out |
                                                         std::fstream::trunc);
    tablet_file.close();

    std::fstream log_file;
    log_file.open(GetLogFilePath(node_idx, i),
                  std::fstream::in | std::fstream::out | std::fstream::trunc);
    log_file.close();
  }

  // create stub for master and peers
  master_stub = KVStoreMaster::NewStub(
      grpc::CreateChannel(master_addr, grpc::InsecureChannelCredentials()));
  for (int i = 0; i < peer_addr_vec.size(); i++) {
    peer_stub_vec.push_back(KVStoreNode::NewStub(grpc::CreateChannel(
        peer_addr_vec[i], grpc::InsecureChannelCredentials())));
  }
}

Status KVStoreNodeImpl::Execute(ServerContext* context,
                                const KVRequest* request, KVResponse* respone) {
  switch (request->request_case()) {
    case KVRequest::RequestCase::kGetRequest: {
      KVGet(&request->get_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kSgetRequest: {
      KVSget(&request->sget_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kPutRequest: {
      KVPut(&request->put_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kSputRequest: {
      KVSput(&request->sput_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kCputRequest: {
      // KVCPut(&request->cput_request(), respone);
      break;
    }
    case KVRequest::RequestCase::kDeleteRequest: {
      // KVDelete(&request->delete_request(), respone);
      break;
    }
    default: {
      respone->set_status(KVStatusCode::FAILURE);
      respone->set_message("-ERR unsupported mthods.");
    }
  }
  return Status::OK;
}

void KVStoreNodeImpl::KVGet(const KVRequest_KVGetRequest* request,
                            KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  VerboseLog("Receive Get <" + row + "><" + col + ">");

  int digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  // check if tablet is in memory
  Tablet* tablet = GetTabletFromMem(tablet_idx);

  // if tablet is not in mem
  if (tablet == NULL) {
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
      VerboseLog("Send Sget <" + row + "><" + col + "> to " + peer_addr_vec[i]);
    }

    /* load target tablet to mem */
    // if vec is full, unload the first one
    if (tablet_mem_vec.size() == num_tablet_mem) {
      UnloadTablet();
    }
    // load target tablet
    tablet = LoadTablet(tablet_idx);
  }

  // target tablet should not be null
  assert(tablet != NULL);

  // set response
  response->set_status(KVStatusCode::SUCCESS);
  response->set_message(tablet->map[row][col]);

  return;
}

void KVStoreNodeImpl::KVSget(const KVRequest_KVSgetRequest* request,
                             KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  VerboseLog("Receive Sget <" + row + "><" + col + ">");

  int digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  // assert tablet is not in mem
  Tablet* tablet = GetTabletFromMem(tablet_idx);
  assert(tablet == NULL);

  /* load target tablet to mem */
  // if vec is full, unload the first one
  if (tablet_mem_vec.size() == num_tablet_mem) {
    UnloadTablet();
  }
  // load target tablet
  tablet = LoadTablet(tablet_idx);

  // target tablet should not be null
  assert(tablet != NULL);

  // set response
  response->set_status(KVStatusCode::SUCCESS);
  // do not need to actually get

  return;
}

void KVStoreNodeImpl::KVPut(const KVRequest_KVPutRequest* request,
                            KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  std::string value = request->value();
  VerboseLog("Receive Put <" + row + "><" + col + ">");

  // send sput request to secondary nodes
  // TODO: fault tolerance
  for (int i = 0; i < peer_stub_vec.size(); i++) {
    KVRequest secondary_request;
    secondary_request.mutable_sput_request()->set_row(row);
    secondary_request.mutable_sput_request()->set_col(col);
    secondary_request.mutable_sput_request()->set_value(value);

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
    VerboseLog("Send Sput <" + row + "><" + col + "> to " + peer_addr_vec[i]);
  }

  /* write to local tablet */
  int digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  // check if tablet is in memory
  Tablet* tablet = GetTabletFromMem(tablet_idx);

  // if tablet is not in mem
  if (tablet == NULL) {
    /* load target tablet to mem */
    // if vec is full, unload the first one
    if (tablet_mem_vec.size() == num_tablet_mem) {
      UnloadTablet();
    }
    // load target tablet
    tablet = LoadTablet(tablet_idx);
  }

  // target tablet should not be null
  assert(tablet != NULL);

  // append put record to log
  // format: put <row> <col> <value_size> <value>
  std::fstream log_file;
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  log_file.open(log_file_path, std::fstream::out | std::fstream::app);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
  } else {
    log_file << "put " << row << " " << col << " " << value.length() << " "
             << value << "\n";
  }
  log_file.close();

  // put value to tablet
  if (tablet->map.find(row) == tablet->map.end()) {
    tablet->map[row] = std::unordered_map<std::string, std::string>();
  }
  tablet->map[row][col] = value;

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVSput(const KVRequest_KVSputRequest* request,
                             KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  std::string value = request->value();
  VerboseLog("Receive Sput <" + row + "><" + col + ">");

  /* write to local tablet */
  int digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  // check if tablet is in memory
  Tablet* tablet = GetTabletFromMem(tablet_idx);

  // if tablet is not in mem
  if (tablet == NULL) {
    /* load target tablet to mem */
    // if vec is full, unload the first one
    if (tablet_mem_vec.size() == num_tablet_mem) {
      UnloadTablet();
    }
    // load target tablet
    tablet = LoadTablet(tablet_idx);
  }

  // target tablet should not be null
  assert(tablet != NULL);

  // append put record to log
  // format: put <row> <col> <value_size> <value>
  std::fstream log_file;
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  log_file.open(log_file_path, std::fstream::out | std::fstream::app);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
  } else {
    log_file << "put " << row << " " << col << " " << value.length() << " "
             << value << "\n";
  }
  log_file.close();

  // put value to tablet
  if (tablet->map.find(row) == tablet->map.end()) {
    tablet->map[row] = std::unordered_map<std::string, std::string>();
  }
  tablet->map[row][col] = value;

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
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