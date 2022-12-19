#include "kvstore_node.h"

::grpc::Server* global_server;
bool shutdown_flag = false;

namespace KVStore {
const char* serverlist_path = "../../Config/serverlist.txt";
int num_replicas = 3;
int num_tablet_total = 5;
int num_tablet_mem = 3;

std::string kw_put = "put";
std::string kw_delete = "delete";

pthread_mutex_t execute_lock;

namespace {
using ::google::protobuf::Empty;
using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

const int kMessageSizeLimit = 1024 * 1024 * 1024;
const int kBackOffLimitMSec = 500;
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

  VerboseLog("Unload node " + std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_to_unload->tablet_idx));

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

  VerboseLog("Load node " + std::to_string(node_idx) + " tablet " +
             std::to_string(tablet->tablet_idx));

  // insert the new tablet to tablet_mem_vec
  assert(tablet_mem_vec.size() < num_tablet_mem);
  tablet_mem_vec.push_back(tablet);

  return tablet;
}

Status KVStoreNodeImpl::CheckHealth(ServerContext* context,
                                    const Empty* request,
                                    KVResponse* response) {
  if (node_status == KVStoreNodeStatus::RUNNING) {
    response->set_status(KVStatusCode::SUCCESS);
  } else if (node_status == KVStoreNodeStatus::SUSPENDED) {
    response->set_status(KVStatusCode::SUSPENDED);
  } else if (node_status == KVStoreNodeStatus::RECOVERING) {
    response->set_status(KVStatusCode::RECOVERING);
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
    std::fstream tablet_file;
    tablet_file.open(GetTabletFilePath(node_idx, i), std::fstream::in |
                                                         std::fstream::out |
                                                         std::fstream::trunc);

    tablet_file.close();

    std::ofstream tablet_file_init(GetTabletFilePath(node_idx, i));
    tablet_file_init << 0 << "\n";
    tablet_file_init.close();

    std::fstream log_file;
    log_file.open(GetLogFilePath(node_idx, i),
                  std::fstream::in | std::fstream::out | std::fstream::trunc);
    log_file.close();
  }

  // create stub for master and peers
  // master_stub = KVStoreMaster::NewStub(
  //     grpc::CreateChannel(master_addr, grpc::InsecureChannelCredentials()));
  ChannelArguments master_args;  // 1GB incoming message size.
  master_args.SetMaxReceiveMessageSize(kMessageSizeLimit);
  master_args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, kBackOffLimitMSec);
  master_stub = KVStoreMaster::NewStub(grpc::CreateCustomChannel(
      master_addr, grpc::InsecureChannelCredentials(), master_args));
  for (int i = 0; i < peer_addr_vec.size(); i++) {
    // peer_stub_vec.push_back(KVStoreNode::NewStub(grpc::CreateChannel(
    //     peer_addr_vec[i], grpc::InsecureChannelCredentials())));
    ChannelArguments peer_args;  // 1GB incoming message size.
    peer_args.SetMaxReceiveMessageSize(kMessageSizeLimit);
    peer_args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, kBackOffLimitMSec);
    peer_stub_vec.push_back(KVStoreNode::NewStub(grpc::CreateCustomChannel(
        peer_addr_vec[i], grpc::InsecureChannelCredentials(), peer_args)));
  }
}

Status KVStoreNodeImpl::Execute(ServerContext* context,
                                const KVRequest* request,
                                KVResponse* response) {
  pthread_mutex_lock(&execute_lock);
  if (node_status == KVStoreNodeStatus::RUNNING) {
    switch (request->request_case()) {
      case KVRequest::RequestCase::kGetRequest: {
        KVGet(&request->get_request(), response);
        break;
      }
      case KVRequest::RequestCase::kSgetRequest: {
        KVSget(&request->sget_request(), response);
        break;
      }
      case KVRequest::RequestCase::kPutRequest: {
        KVPut(&request->put_request(), response);
        break;
      }
      case KVRequest::RequestCase::kSputRequest: {
        KVSput(&request->sput_request(), response);
        break;
      }
      case KVRequest::RequestCase::kCputRequest: {
        KVCput(&request->cput_request(), response);
        break;
      }
      case KVRequest::RequestCase::kScputRequest: {
        KVScput(&request->scput_request(), response);
        break;
      }
      case KVRequest::RequestCase::kDeleteRequest: {
        KVDelete(&request->delete_request(), response);
        break;
      }
      case KVRequest::RequestCase::kSdeleteRequest: {
        KVSdelete(&request->sdelete_request(), response);
        break;
      }
      // node can accept suspend request from master
      case KVRequest::RequestCase::kSuspendRequest: {
        KVSuspend(&request->suspend_request(), response);
        break;
      }
      // primary can accept recovery request from master (in alive status)
      case KVRequest::RequestCase::kRecoveryRequest: {
        KVPrimaryRecovery(&request->recovery_request(), response);
        break;
      }
      case KVRequest::RequestCase::kSrecoveryRequest: {
        KVSecondaryRecovery(&request->srecovery_request(), response);
        break;
      }
      default: {
        VerboseLog("-ERR unsupported mthods when node is alive");
        response->set_status(KVStatusCode::FAILURE);
        response->set_message("-ERR unsupported mthods when node is alive");
      }
    }
  } else if (node_status == KVStoreNodeStatus::SUSPENDED) {
    switch (request->request_case()) {
      // only allow recover request from primary (in suspend status)
      case KVRequest::RequestCase::kSrecoveryRequest: {
        KVSecondaryRecovery(&request->srecovery_request(), response);
        break;
      }
      default: {
        VerboseLog("-ERR unsupported mthods when node is suspend");
        response->set_status(KVStatusCode::FAILURE);
        response->set_message("-ERR unsupported mthods when node is suspend");
      }
    }
  } else if (node_status == KVStoreNodeStatus::RECOVERING) {
    // only allow recovery communication request from primary
    switch (request->request_case()) {
      // only allow recover request from primary (in suspend status)
      case KVRequest::RequestCase::kChecksumRequest: {
        KVChecksum(&request->checksum_request(), response);
        break;
      }
      case KVRequest::RequestCase::kFiletransferRequest: {
        KVFiletransfer(&request->filetransfer_request(), response);
        break;
      }
      case KVRequest::RequestCase::kReplayRequest: {
        KVReplay(&request->replay_request(), response);
        break;
      }
      case KVRequest::RequestCase::kRecoveryRequest: {
        VerboseLog("-ERR Primary is busy with recovering other nodes");
        response->set_status(KVStatusCode::FAILURE);
        response->set_message("Primary is busy with recovering other nodes");
        break;
      }
      default: {
        VerboseLog("-ERR unsupported mthods when node is recovering");
        response->set_status(KVStatusCode::FAILURE);
        response->set_message(
            "-ERR unsupported mthods when node is recovering");
      }
    }
  }

  pthread_mutex_unlock(&execute_lock);
  return Status::OK;
}

void KVStoreNodeImpl::KVGet(const KVRequest_KVGetRequest* request,
                            KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();

  unsigned long digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  VerboseLog("Receive Get <" + row + "><" + col + "> from node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

  // check if tablet is in memory
  Tablet* tablet = GetTabletFromMem(tablet_idx);

  // if tablet is not in mem
  if (tablet == NULL) {
    // firstly notify peers to switch tablet
    for (int i = 0; i < peer_stub_vec.size(); i++) {
      // check if secondary is alive
      ClientContext healthcheck_context;
      Empty healthcheck_request;
      KVResponse healthcheck_response;
      Status healthcheck_status = peer_stub_vec[i].get()->CheckHealth(
          &healthcheck_context, healthcheck_request, &healthcheck_response);
      if (healthcheck_status.ok() &&
          healthcheck_response.status() == KVStatusCode::SUCCESS) {
        // only if alive, send secondary request
        KVRequest secondary_request;
        secondary_request.mutable_sget_request()->set_row(row);
        secondary_request.mutable_sget_request()->set_col(col);

        KVResponse secondary_response;
        ClientContext secondary_context;
        Status status = peer_stub_vec[i].get()->Execute(
            &secondary_context, secondary_request, &secondary_response);
        VerboseLog("Send Sget <" + row + "><" + col + "> to " +
                   peer_addr_vec[i]);
      } else {
        VerboseLog("Cannot send sget to " + peer_addr_vec[i] +
                   " as it's not alive");
      }
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

  // check if row col exists
  if (tablet->map.find(row) == tablet->map.end() ||
      tablet->map[row].find(col) == tablet->map[row].end()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("value not found");
  } else {
    response->set_status(KVStatusCode::SUCCESS);
    response->set_message(tablet->map[row][col]);
  }

  return;
}

void KVStoreNodeImpl::KVSget(const KVRequest_KVSgetRequest* request,
                             KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();

  unsigned long digest = GetDigest(row, col);
  int tablet_idx = Digest2TabletIdx(digest, num_tablet_total);

  VerboseLog("Receive Sget <" + row + "><" + col + "> from node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

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
  for (int i = 0; i < peer_stub_vec.size(); i++) {
    // check if secondary is alive
    ClientContext healthcheck_context;
    Empty healthcheck_request;
    KVResponse healthcheck_response;
    Status healthcheck_status = peer_stub_vec[i].get()->CheckHealth(
        &healthcheck_context, healthcheck_request, &healthcheck_response);
    if (healthcheck_status.ok() &&
        healthcheck_response.status() == KVStatusCode::SUCCESS) {
      // only if alive, send secondary request
      KVRequest secondary_request;
      secondary_request.mutable_sput_request()->set_row(row);
      secondary_request.mutable_sput_request()->set_col(col);
      secondary_request.mutable_sput_request()->set_value(value);

      KVResponse secondary_response;
      ClientContext secondary_context;
      Status status = peer_stub_vec[i].get()->Execute(
          &secondary_context, secondary_request, &secondary_response);
      VerboseLog("Send Sput <" + row + "><" + col + "> to " + peer_addr_vec[i]);
    } else {
      VerboseLog("Cannot send sput to " + peer_addr_vec[i] +
                 " as it's not alive");
    }
  }

  /* write to local tablet */
  unsigned long digest = GetDigest(row, col);
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
    log_file << kw_put << " " << row << " " << col << " " << value.length()
             << " " << value << "\n";
  }
  log_file.close();

  // put value to tablet
  if (tablet->map.find(row) == tablet->map.end()) {
    tablet->map[row] = std::unordered_map<std::string, std::string>();
  }
  tablet->map[row][col] = value;

  VerboseLog("Put <" + row + "><" + col + "> to node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

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
  unsigned long digest = GetDigest(row, col);
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
    log_file << kw_put << " " << row << " " << col << " " << value.length()
             << " " << value << "\n";
  }
  log_file.close();

  // put value to tablet
  if (tablet->map.find(row) == tablet->map.end()) {
    tablet->map[row] = std::unordered_map<std::string, std::string>();
  }
  tablet->map[row][col] = value;

  VerboseLog("Sput <" + row + "><" + col + "> to node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVCput(const KVRequest_KVCPutRequest* request,
                             KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  std::string cur_value = request->cur_value();
  std::string new_value = request->new_value();
  VerboseLog("Receive CPut <" + row + "><" + col + ">");

  // send scput request to secondary nodes
  // TODO: fault tolerance
  for (int i = 0; i < peer_stub_vec.size(); i++) {
    // check if secondary is alive
    ClientContext healthcheck_context;
    Empty healthcheck_request;
    KVResponse healthcheck_response;
    Status healthcheck_status = peer_stub_vec[i].get()->CheckHealth(
        &healthcheck_context, healthcheck_request, &healthcheck_response);
    if (healthcheck_status.ok() &&
        healthcheck_response.status() == KVStatusCode::SUCCESS) {
      // only if alive, send secondary request
      KVRequest secondary_request;
      secondary_request.mutable_scput_request()->set_row(row);
      secondary_request.mutable_scput_request()->set_col(col);
      secondary_request.mutable_scput_request()->set_cur_value(cur_value);
      secondary_request.mutable_scput_request()->set_new_value(new_value);

      KVResponse secondary_response;
      ClientContext secondary_context;
      Status status = peer_stub_vec[i].get()->Execute(
          &secondary_context, secondary_request, &secondary_response);
      VerboseLog("Send Scput <" + row + "><" + col + "> to " +
                 peer_addr_vec[i]);
    } else {
      VerboseLog("Cannot send scput to " + peer_addr_vec[i] +
                 " as it's not alive");
    }
  }

  /* cput to local tablet */
  unsigned long digest = GetDigest(row, col);
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

  // check if row key exists and cur_value matches
  if (tablet->map.find(row) == tablet->map.end() ||
      tablet->map[row].find(col) == tablet->map[row].end()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("CPut target cell not found");
    return;
  }

  if (tablet->map[row][col].compare(cur_value) != 0) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("CPut condition doesn't match");
    return;
  }

  // if matches, log and write to tablet
  // append put record to log
  // format: put <row> <col> <value_size> <value>
  std::fstream log_file;
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  log_file.open(log_file_path, std::fstream::out | std::fstream::app);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
  } else {
    log_file << kw_put << " " << row << " " << col << " " << new_value.length()
             << " " << new_value << "\n";
  }
  log_file.close();

  // put new value to tablet
  assert(tablet->map.find(row) != tablet->map.end() &&
         tablet->map[row].find(col) != tablet->map[row].end());
  tablet->map[row][col] = new_value;

  VerboseLog("CPut <" + row + "><" + col + "> to node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVScput(const KVRequest_KVScputRequest* request,
                              KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  std::string cur_value = request->cur_value();
  std::string new_value = request->new_value();
  VerboseLog("Receive Scput <" + row + "><" + col + ">");

  unsigned long digest = GetDigest(row, col);
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

  // check if row key exists and cur_value matches
  if (tablet->map.find(row) == tablet->map.end() ||
      tablet->map[row].find(col) == tablet->map[row].end()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("Scput target cell not found");
    return;
  }

  if (tablet->map[row][col].compare(cur_value) != 0) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("Scput condition doesn't match");
    return;
  }

  // if matches, log and write to tablet
  // append put record to log
  // format: put <row> <col> <value_size> <value>
  std::fstream log_file;
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  log_file.open(log_file_path, std::fstream::out | std::fstream::app);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
  } else {
    log_file << kw_put << " " << row << " " << col << " " << new_value.length()
             << " " << new_value << "\n";
  }
  log_file.close();

  // put new value to tablet
  assert(tablet->map.find(row) != tablet->map.end() &&
         tablet->map[row].find(col) != tablet->map[row].end());
  tablet->map[row][col] = new_value;

  VerboseLog("Scput <" + row + "><" + col + "> to node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVDelete(const KVRequest_KVDeleteRequest* request,
                               KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  VerboseLog("Receive Delete <" + row + "><" + col + ">");

  // send sdelete request to secondary nodes
  for (int i = 0; i < peer_stub_vec.size(); i++) {
    ClientContext healthcheck_context;
    Empty healthcheck_request;
    KVResponse healthcheck_response;
    Status healthcheck_status = peer_stub_vec[i].get()->CheckHealth(
        &healthcheck_context, healthcheck_request, &healthcheck_response);
    if (healthcheck_status.ok() &&
        healthcheck_response.status() == KVStatusCode::SUCCESS) {
      // only if alive, send secondary request
      KVRequest secondary_request;
      secondary_request.mutable_sdelete_request()->set_row(row);
      secondary_request.mutable_sdelete_request()->set_col(col);
      KVResponse secondary_response;
      ClientContext secondary_context;
      Status status = peer_stub_vec[i].get()->Execute(
          &secondary_context, secondary_request, &secondary_response);
      VerboseLog("Send Sdelete <" + row + "><" + col + "> to " +
                 peer_addr_vec[i]);
    } else {
      VerboseLog("Cannot send sdelete to " + peer_addr_vec[i] +
                 " as it's not alive");
    }
  }

  /* delete from local tablet */
  unsigned long digest = GetDigest(row, col);
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

  // check if target cell exists
  if (tablet->map.find(row) == tablet->map.end() ||
      tablet->map[row].find(col) == tablet->map[row].end()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("Delete target cell not found");
    return;
  }

  // append delete log
  std::fstream log_file;
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  log_file.open(log_file_path, std::fstream::out | std::fstream::app);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
  } else {
    log_file << kw_delete << " " << row << " " << col << "\n";
  }
  log_file.close();

  // delete entry
  tablet->map[row].erase(col);
  // check if row is empty, if empty, delete row
  if (tablet->map[row].size() == 0) {
    tablet->map.erase(row);
  }

  VerboseLog("Delete <" + row + "><" + col + "> from node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVSdelete(const KVRequest_KVSdeleteRequest* request,
                                KVResponse* response) {
  // retrieve row and col key
  std::string row = request->row();
  std::string col = request->col();
  VerboseLog("Receive Delete <" + row + "><" + col + ">");

  /* delete from local tablet */
  unsigned long digest = GetDigest(row, col);
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

  // check if target cell exists
  if (tablet->map.find(row) == tablet->map.end() ||
      tablet->map[row].find(col) == tablet->map[row].end()) {
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("Sdelete target cell not found");
    return;
  }

  // append delete log
  std::fstream log_file;
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  log_file.open(log_file_path, std::fstream::out | std::fstream::app);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
  } else {
    log_file << kw_delete << " " << row << " " << col << "\n";
  }
  log_file.close();

  // delete entry
  tablet->map[row].erase(col);
  // check if row is empty, if empty, delete row
  if (tablet->map[row].size() == 0) {
    tablet->map.erase(row);
  }

  VerboseLog("Sdelete <" + row + "><" + col + "> from node " +
             std::to_string(node_idx) + " tablet " +
             std::to_string(tablet_idx));

  // set response
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVSuspend(const KVRequest_KVSuspendRequest* request,
                                KVResponse* response) {
  std::string target_addr = request->target_addr();
  // no actual usage, just to assert addr is correct
  assert(addr.compare(target_addr) == 0);

  VerboseLog("Receive suspend request for myself");

  // update status to suspend
  node_status = KVStoreNodeStatus::SUSPENDED;

  // status only represents this request is successfully accepted
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void* KVStoreNodeImpl::KVPrimaryRecoveryThreadFunc(void* args) {
  KVStoreNodeImpl* primary_node = (KVStoreNodeImpl*)args;
  std::string target_addr = primary_node->target_addr_to_recover;

  primary_node->VerboseLog("primary node starts to recover secondary node: " +
                           target_addr);

  // locate the idx of target secondary node to be recovered
  int target_idx = -1;
  for (int i = 0; i < primary_node->peer_addr_vec.size(); i++) {
    if (primary_node->peer_addr_vec[i].compare(target_addr) == 0) {
      target_idx = i;
      break;
    }
  }
  assert(target_idx != -1);

  // primary node send srecovery notification to secondary nodes
  primary_node->VerboseLog("Send recovery notification to secondary node: " +
                           target_addr);
  KVRequest secondary_recovery_request;
  secondary_recovery_request.mutable_srecovery_request()->set_target_addr(
      target_addr);
  KVResponse secondary_recovery_response;
  ClientContext secondary_recovery_context;
  Status secondary_recovery_status =
      primary_node->peer_stub_vec[target_idx].get()->Execute(
          &secondary_recovery_context, secondary_recovery_request,
          &secondary_recovery_response);

  if (!secondary_recovery_status.ok()) {
    std::cerr << "-ERR secondary recovery request to " << target_addr
              << " fails, grpc fails" << std::endl;
  } else if (secondary_recovery_response.status() != KVStatusCode::SUCCESS) {
    std::cerr << "-ERR secondary recovery request to " << target_addr
              << " fails, request rejected" << std::endl;
  }

  // primary node tranfer files to secondary nodes
  for (int i = 0; i < num_tablet_total; i++) {
    // process tablet with idx = i

    /* transfer checkpoint */
    // open checkpoint file and read to string
    std::ifstream checkpoint_file(GetTabletFilePath(primary_node->node_idx, i));
    std::stringstream checkpoint_file_buffer;
    checkpoint_file_buffer << checkpoint_file.rdbuf();
    checkpoint_file.close();

    // checkpoint checksum
    KVRequest checkpoint_checksum_request;
    checkpoint_checksum_request.mutable_checksum_request()->set_file_type(
        FileType::CHECKPOINT);
    checkpoint_checksum_request.mutable_checksum_request()->set_tablet_idx(i);
    checkpoint_checksum_request.mutable_checksum_request()->set_checksum(
        getDigestStr(checkpoint_file_buffer.str()));
    KVResponse checkpoint_checksum_response;
    ClientContext checkpoint_checksum_context;
    Status checkpoint_checksum_status =
        primary_node->peer_stub_vec[target_idx].get()->Execute(
            &checkpoint_checksum_context, checkpoint_checksum_request,
            &checkpoint_checksum_response);
    if (!checkpoint_checksum_status.ok()) {
      std::cerr << "-ERR checksum for checkpoint " << i << " fails, grpc fails"
                << std::endl;
    } else if (checkpoint_checksum_response.status() != KVStatusCode::SUCCESS) {
      primary_node->VerboseLog(checkpoint_checksum_response.message());
    } else {
      primary_node->VerboseLog("Sync checkpoint " + std::to_string(i) +
                               " with " + target_addr);
      // set and send file transfer request
      KVRequest checkpoint_transfer_request;
      checkpoint_transfer_request.mutable_filetransfer_request()->set_file_type(
          FileType::CHECKPOINT);
      checkpoint_transfer_request.mutable_filetransfer_request()
          ->set_tablet_idx(i);
      checkpoint_transfer_request.mutable_filetransfer_request()->set_content(
          checkpoint_file_buffer.str());
      KVResponse checkpoint_transfer_response;
      ClientContext checkpoint_transfer_context;
      Status checkpoint_transfer_status =
          primary_node->peer_stub_vec[target_idx].get()->Execute(
              &checkpoint_transfer_context, checkpoint_transfer_request,
              &checkpoint_transfer_response);

      if (!checkpoint_transfer_status.ok()) {
        std::cerr << "-ERR checkpoint " << i << " transfer fails, grpc fails"
                  << std::endl;
      } else if (checkpoint_transfer_response.status() !=
                 KVStatusCode::SUCCESS) {
        std::cerr << "-ERR checkpoint " << i
                  << " transfer fails, request rejected " << std::endl;
      }
    }

    /* transfer log */
    // open log file and read to string
    std::ifstream log_file(GetLogFilePath(primary_node->node_idx, i));
    std::stringstream log_file_buffer;
    log_file_buffer << log_file.rdbuf();
    log_file.close();

    // logfile checksum
    KVRequest log_checksum_request;
    log_checksum_request.mutable_checksum_request()->set_file_type(
        FileType::LOGFILE);
    log_checksum_request.mutable_checksum_request()->set_tablet_idx(i);
    log_checksum_request.mutable_checksum_request()->set_checksum(
        getDigestStr(log_file_buffer.str()));
    KVResponse log_checksum_response;
    ClientContext log_checksum_context;
    Status log_checksum_status =
        primary_node->peer_stub_vec[target_idx].get()->Execute(
            &log_checksum_context, log_checksum_request,
            &log_checksum_response);

    if (!log_checksum_status.ok()) {
      std::cerr << "-ERR checksum for logfile " << i << " fails, grpc fails"
                << std::endl;
    } else if (log_checksum_response.status() != KVStatusCode::SUCCESS) {
      primary_node->VerboseLog(log_checksum_response.message());
    } else {
      primary_node->VerboseLog("Sync logfile " + std::to_string(i) + " with " +
                               target_addr);
      // set and send request
      KVRequest log_transfer_request;
      log_transfer_request.mutable_filetransfer_request()->set_file_type(
          FileType::LOGFILE);
      log_transfer_request.mutable_filetransfer_request()->set_tablet_idx(i);
      log_transfer_request.mutable_filetransfer_request()->set_content(
          log_file_buffer.str());
      KVResponse log_transfer_response;
      ClientContext log_transfer_context;
      Status log_transfer_status =
          primary_node->peer_stub_vec[target_idx].get()->Execute(
              &log_transfer_context, log_transfer_request,
              &log_transfer_response);

      if (!log_transfer_status.ok()) {
        std::cerr << "-ERR logfile " << i << " transfer fails, grpc fails"
                  << std::endl;
      } else if (log_transfer_response.status() != KVStatusCode::SUCCESS) {
        std::cerr << "-ERR logfile " << i
                  << " transfer fails, request rejected " << std::endl;
      }
    }
  }

  /* send replay request */
  std::string tablet_target;
  for (int i = 0; i < primary_node->tablet_mem_vec.size(); i++) {
    tablet_target =
        tablet_target +
        std::to_string(primary_node->tablet_mem_vec[i]->tablet_idx) + ",";
  }

  primary_node->VerboseLog("Send replay request to secondary node: " +
                           target_addr);
  KVRequest replay_request;
  replay_request.mutable_replay_request()->set_tablet_num(
      primary_node->tablet_mem_vec.size());
  replay_request.mutable_replay_request()->set_tablet_target(tablet_target);
  KVResponse replay_response;
  ClientContext replay_context;
  Status replay_status = primary_node->peer_stub_vec[target_idx].get()->Execute(
      &replay_context, replay_request, &replay_response);

  if (!replay_status.ok()) {
    std::cerr << "-ERR replay " << tablet_target << " fails, grpc fails"
              << std::endl;
  } else if (replay_response.status() != KVStatusCode::SUCCESS) {
    std::cerr << "-ERR replay " << tablet_target << " fails, request rejected "
              << std::endl;
  }

  // replay finishes, recovery finishes, set primary node status to ALIVE
  primary_node->node_status = KVStoreNodeStatus::RUNNING;

  // reply to master that recovery is finished
  primary_node->VerboseLog("Acknowledge master recovery to " + target_addr +
                           " is finished");
  NotifyRecoveryFinishedRequest recovery_finish_request;
  recovery_finish_request.set_target_addr(target_addr);
  KVResponse recovery_finish_response;
  ClientContext recovery_finish_context;
  Status recovery_finish_status =
      primary_node->master_stub.get()->NotifyRecoveryFinished(
          &recovery_finish_context, recovery_finish_request,
          &recovery_finish_response);

  // reset target_addr_to_recover
  primary_node->target_addr_to_recover.clear();

  return NULL;
}

void KVStoreNodeImpl::KVPrimaryRecovery(
    const KVRequest_KVRecoveryRequest* request, KVResponse* response) {
  std::string target_addr = request->target_addr();
  // this node is primary, target_addr is the addr of the secondary node to be
  // revived
  assert(addr.compare(target_addr) != 0);

  VerboseLog("Primary receives recovery request for secondary node: " +
             target_addr);

  // locate the idx of target secondary node to be recovered
  int target_idx = -1;
  for (int i = 0; i < peer_addr_vec.size(); i++) {
    if (peer_addr_vec[i].compare(target_addr) == 0) {
      target_idx = i;
      break;
    }
  }
  assert(target_idx != -1);

  ClientContext shealth_check_context;
  KVResponse shealth_check_response;
  Status shealth_check_status = peer_stub_vec[target_idx]->CheckHealth(
      &shealth_check_context, Empty(), &shealth_check_response);
  if (!shealth_check_status.ok()) {
    VerboseLog("-------------------Primary fails to reconnect secondary " +
               target_addr + " do not recover");
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("Primary fails to reconnect secondary " +
                          target_addr);
    return;
  }

  // set primary status to RECOVERING
  node_status = KVStoreNodeStatus::RECOVERING;

  // activate the recovery thread to execute
  pthread_t recovery_thread;
  // pass self and target_addr as arguments
  target_addr_to_recover = target_addr;
  if (pthread_create(&recovery_thread, NULL, &KVPrimaryRecoveryThreadFunc,
                     this) < 0) {
    fprintf(stderr,
            "Failed to create a pthread to execute primary node's recovery "
            "work.\n");
    exit(-1);
  };

  // set response, status only represents this request is successfully
  // accepted
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

void KVStoreNodeImpl::KVSecondaryRecovery(
    const KVRequest_KVSrecoveryRequest* request, KVResponse* response) {
  std::string target_addr = request->target_addr();
  // no actual usage, just to assert addr is correct
  assert(addr.compare(target_addr) == 0);

  VerboseLog("Secondary receive recovery request for myself");

  // update status to suspend
  node_status = KVStoreNodeStatus::RECOVERING;

  // status only represents this request is successfully accepted
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

/* auxiliary functions */
void computeDigest(char* data, int dataLengthBytes,
                   unsigned char* digestBuffer) {
  /* The digest will be written to digestBuffer, which must be at least
   * MD5_DIGEST_LENGTH bytes long */

  MD5_CTX c;
  MD5_Init(&c);
  MD5_Update(&c, data, dataLengthBytes);
  MD5_Final(digestBuffer, &c);
}

std::string getDigestStr(std::string target) {
  std::string digestStr;
  // compute digest
  unsigned char digestBuffer[MD5_DIGEST_LENGTH];
  computeDigest(const_cast<char*>(target.c_str()), target.length(),
                digestBuffer);
  digestStr.reserve(32);
  for (std::size_t i = 0; i != 16; ++i) {
    digestStr += "0123456789ABCDEF"[digestBuffer[i] / 16];
    digestStr += "0123456789ABCDEF"[digestBuffer[i] % 16];
  }

  return digestStr;
}

void KVStoreNodeImpl::KVChecksum(const KVRequest_KVChecksumRequest* request,
                                 KVResponse* response) {
  FileType file_type = request->file_type();
  int tablet_idx = request->tablet_idx();
  std::string checksum_primary = request->checksum();

  std::string file_path;
  if (file_type == FileType::CHECKPOINT) {
    file_path = GetTabletFilePath(node_idx, tablet_idx);
  } else if (file_type == FileType::LOGFILE) {
    file_path = GetLogFilePath(node_idx, tablet_idx);
  }

  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "cannot open " << file_path << ", need file transfer"
              << std::endl;
    response->set_status(KVStatusCode::FAILURE);

    return;
  }
  std::stringstream file_buffer;
  file_buffer << file.rdbuf();
  file.close();

  std::string target_file;
  if (file_type == FileType::CHECKPOINT) {
    target_file = "checkpoint " + std::to_string(tablet_idx);
  } else if (file_type == FileType::LOGFILE) {
    target_file = "logfile " + std::to_string(tablet_idx);
  }

  std::string checksum_local = getDigestStr(file_buffer.str());
  if (checksum_local.compare(checksum_primary) == 0) {
    VerboseLog("Checksum for " + target_file +
               " is the same, no need to transfer");
    response->set_status(KVStatusCode::FAILURE);
    response->set_message("Checksum for " + target_file +
                          " is the same, no need to transfer");

    return;
  } else {
    VerboseLog("Checksum for " + target_file +
               " is different, await to transfer");
    response->set_status(KVStatusCode::SUCCESS);
    response->set_message("Checksum for " + target_file +
                          " is different, await to transfer");

    return;
  }
}

void KVStoreNodeImpl::KVFiletransfer(
    const KVRequest_KVFiletransferRequest* request, KVResponse* response) {
  FileType file_type = request->file_type();
  int tablet_idx = request->tablet_idx();
  std::string content = request->content();

  std::string file_path;
  if (file_type == FileType::CHECKPOINT) {
    file_path = GetTabletFilePath(node_idx, tablet_idx);
  } else if (file_type == FileType::LOGFILE) {
    file_path = GetLogFilePath(node_idx, tablet_idx);
  }

  // overwrite file
  std::fstream file_open;
  file_open.open(file_path,
                 std::fstream::in | std::fstream::out | std::fstream::trunc);
  file_open.close();

  std::ofstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "cannot open " << file_path << std::endl;
    exit(-1);
  }
  file << content;
  file.close();

  if (file_type == FileType::CHECKPOINT) {
    VerboseLog("Sync checkpoint " + std::to_string(tablet_idx) +
               " with primary node");
  } else if (file_type == FileType::LOGFILE) {
    VerboseLog("Sync logfile " + std::to_string(tablet_idx) +
               " with primary node");
  }

  // status only represents this request is successfully accepted
  response->set_status(KVStatusCode::SUCCESS);

  return;
}

Tablet* KVStoreNodeImpl::ReplayTablet(int tablet_idx) {
  // load tablet from checkpoint
  Tablet* tablet = LoadTabletFromFile(node_idx, tablet_idx);
  assert(tablet != NULL);

  /* replay from log file */
  // read from log file
  std::string log_file_path = GetLogFilePath(node_idx, tablet_idx);
  std::ifstream log_file(log_file_path);
  if (!log_file.is_open()) {
    std::cerr << "cannot open " << log_file_path << std::endl;
    exit(-1);
  }

  std::string command;
  while (log_file >> command) {
    if (command.compare(kw_put) == 0) {
      std::string row;
      std::string col;
      int value_length;
      log_file >> row >> col >> value_length;

      // skip a whitespace
      char ignorebuffer[10];
      log_file.read(ignorebuffer, 1);
      char* buffer = new char[value_length];
      log_file.read(buffer, value_length);
      std::string value = std::string(buffer, value_length);
      delete[] buffer;

      // put <row> <col> <value>
      if (tablet->map.find(row) == tablet->map.end()) {
        tablet->map[row] = std::unordered_map<std::string, std::string>();
      }
      tablet->map[row][col] = value;

    } else if (command.compare(kw_delete) == 0) {
      std::string row;
      std::string col;
      log_file >> row >> col;

      // assert cell exists
      assert(tablet->map.find(row) != tablet->map.end() &&
             tablet->map[row].find(col) != tablet->map[row].end());

      // delete <row> <col>
      tablet->map[row].erase(col);
      // check if row is empty, if empty, delete row
      if (tablet->map[row].size() == 0) {
        tablet->map.erase(row);
      }

    } else {
      std::cerr << "[replay] command " << command << " not recongnized!"
                << std::endl;
      exit(-1);
    }
  }

  return tablet;
}

void KVStoreNodeImpl::KVReplay(const KVRequest_KVReplayRequest* request,
                               KVResponse* response) {
  int tablet_num = request->tablet_num();
  std::string tablet_target = request->tablet_target();
  VerboseLog("Receive replay request for tablet " + tablet_target);

  // parse tablet target
  char* tablet_target_char = strdup(tablet_target.c_str());
  std::vector<int> tablet_idx_vec;
  if (tablet_num >= 1) {
    char* tablet_idx_char = strtok(tablet_target_char, ",");
    tablet_idx_vec.push_back(atoi(tablet_idx_char));
  }
  if (tablet_num >= 2) {
    for (int i = 1; i < tablet_num; i++) {
      char* tablet_idx_char = strtok(NULL, ",");
      tablet_idx_vec.push_back(atoi(tablet_idx_char));
    }
  }
  free(tablet_target_char);

  // clear deprecated tablet_mem_vec
  tablet_mem_vec.clear();

  // replay each tablet in mem from corresponding checkpoint and logfile
  for (int i = 0; i < tablet_idx_vec.size(); i++) {
    Tablet* tablet = ReplayTablet(tablet_idx_vec[i]);
    // push tablet to tablet_mem_vec
    tablet_mem_vec.push_back(tablet);
  }

  // replay finishes, recovery finishes, set secondary node status to ALIVE
  node_status = KVStoreNodeStatus::RUNNING;

  // respond
  response->set_status(KVStatusCode::SUCCESS);

  VerboseLog("Secondary finishes recovery");

  return;
}

}  // namespace KVStore

void CheckShutdownThreadFunc(void) {
  while (!shutdown_flag) {
    sleep(1);
  }
  global_server->Shutdown();
}

void signalHandler(int signum) {
  std::cout << "Shutdown server" << std::endl;
  shutdown_flag = true;
}

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "[Command Line Format] ./kvstore_node <num> [-v]\n");
    exit(-1);
  }

  signal(SIGINT, signalHandler);
  std::thread check_shutdown_thread(CheckShutdownThreadFunc);

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
  // 1GB incoming message size,
  builder.SetMaxReceiveMessageSize(KVStore::kMessageSizeLimit);
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&node);
  std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
  global_server = server.get();
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();

  check_shutdown_thread.join();
  return 0;
}