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
  if (node_status == KVStoreNodeStatus::ALIVE) {
    response->set_status(KVStatusCode::SUCCESS);
  } else if (node_status == KVStoreNodeStatus::SUSPEND) {
    response->set_status(KVStatusCode::SUSPEND);
  } else if (node_status == KVStoreNodeStatus::RECOVERYING) {
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
  master_stub = KVStoreMaster::NewStub(
      grpc::CreateChannel(master_addr, grpc::InsecureChannelCredentials()));
  for (int i = 0; i < peer_addr_vec.size(); i++) {
    peer_stub_vec.push_back(KVStoreNode::NewStub(grpc::CreateChannel(
        peer_addr_vec[i], grpc::InsecureChannelCredentials())));
  }
}

Status KVStoreNodeImpl::Execute(ServerContext* context,
                                const KVRequest* request,
                                KVResponse* response) {
  if (node_status == KVStoreNodeStatus::ALIVE) {
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
      default: {
        response->set_status(KVStatusCode::FAILURE);
        response->set_message("-ERR unsupported mthods when node is alive");
      }
    }
  } else if (node_status == KVStoreNodeStatus::SUSPEND) {
  } else if (node_status == KVStoreNodeStatus::RECOVERYING) {
  }
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
    log_file << "put " << row << " " << col << " " << value.length() << " "
             << value << "\n";
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
    log_file << "put " << row << " " << col << " " << value.length() << " "
             << value << "\n";
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
    log_file << "put " << row << " " << col << " " << new_value.length() << " "
             << new_value << "\n";
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
    log_file << "put " << row << " " << col << " " << new_value.length() << " "
             << new_value << "\n";
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
    log_file << "delete " << row << " " << col << "\n";
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
    log_file << "delete " << row << " " << col << "\n";
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