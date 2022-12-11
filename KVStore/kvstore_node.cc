#include "kvstore_node.h"

namespace KVStore {
namespace {
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Request Queue

// TODO: get cluster path based on ip, get tablet path based on rowkey and
// colkey
std::string GetTabletPath() {
  // TODO: always return test tablet
  std::string tabletPath = "../Database/cluster1/node1/tablet1.txt";

  // if file does not exist, create new file
  std::fstream tabletFile;
  tabletFile.open(tabletPath,
                  std::fstream::in | std::fstream::out | std::fstream::app);
  if (!tabletFile) {
    tabletFile.open(tabletPath,
                    std::fstream::in | std::fstream::out | std::fstream::app);
  }
  tabletFile.close();

  return tabletPath;
}

int WriteTablet(Tablet* tablet) {
  // if file does not exist, create new file
  std::string path = tablet->path;
  std::fstream tabletFile;
  tabletFile.open(path,
                  std::fstream::in | std::fstream::out | std::fstream::trunc);
  if (!tabletFile) {
    tabletFile.open(path,
                    std::fstream::in | std::fstream::out | std::fstream::trunc);
  }
  // close file
  tabletFile.close();

  /* write to file */
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      map = tablet->map;
  std::ofstream file(path);
  if (!file.is_open()) {
    return -1;
  }
  // firstly write numRows, ended by \n
  file << map.size() << "\n";
  // for each row
  for (std::pair<std::string, std::unordered_map<std::string, std::string>>
           rowEntry : map) {
    std::string rowKey = rowEntry.first;
    std::unordered_map<std::string, std::string> row = rowEntry.second;
    // write row key and numCols, splited by a space and ended by \n
    file << rowKey << " " << row.size() << "\n";

    // for each col
    for (std::pair<std::string, std::string> colEntry : row) {
      std::string colKey = colEntry.first;
      std::string cell = colEntry.second;
      // write col key <sp> cell size <sp> cell , ended by \n
      file << colKey << " " << cell.length() << " " << cell << "\n";
    }
  }
  // write finishes, close file
  file.close();

  return 0;
}

Tablet* loadTablet(std::string tabletPath) {
  std::ifstream file(tabletPath);
  if (!file.is_open()) {
    return NULL;
  }

  Tablet* tablet = new Tablet();
  tablet->path = tabletPath;

  int numRows;
  file >> numRows;

  // insert each row
  for (int r = 0; r < numRows; r++) {
    std::string rowKey;
    int numCols;
    file >> rowKey >> numCols;
    tablet->map[rowKey] = std::unordered_map<std::string, std::string>();

    // insert each col
    for (int c = 0; c < numCols; c++) {
      std::string colKey;
      int cellSize;
      file >> colKey >> cellSize;

      // skip a whitespace
      char ignorebuffer[10];
      file.read(ignorebuffer, 1);
      char buffer[cellSize];
      file.read(buffer, cellSize);
      std::string cell = std::string(buffer, cellSize);

      // insert to map
      tablet->map[rowKey][colKey] = cell;
    }
  }

  return tablet;
}

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
<<<<<<< HEAD
=======
  // Load tablet into memory.
>>>>>>> kvstore-lsd
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
<<<<<<< HEAD
      KVGet(&request->get_request(), respone);
=======
      KVGet(&request->get_request(), respone, node_stub1_.get());
>>>>>>> kvstore-lsd
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
}  // namespace KVStore

int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:10002");
  KVStore::KVStoreNodeImpl service;

  ::grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}