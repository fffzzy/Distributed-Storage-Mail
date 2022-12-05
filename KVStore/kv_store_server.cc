#include "kv_store_server.h"

namespace KVStore {
namespace {
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

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

int WriteTablet(TabletStruct* tablet) {
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

TabletStruct* loadTablet(std::string tabletPath) {
  std::ifstream file(tabletPath);
  if (!file.is_open()) {
    return NULL;
  }

  TabletStruct* tablet = new TabletStruct();
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

}  // namespace

KVStoreServiceImpl::KVStoreServiceImpl() {
  tablet_ = std::make_unique<TabletStruct>();
  tablet_->path = GetTabletPath();
  tablet_->map =
      std::unordered_map<std::string,
                         std::unordered_map<std::string, std::string>>();
}

Status KVStoreServiceImpl::Put(ServerContext* context,
                               const KVPutRequest* request,
                               KVPutResponse* response) {
  std::string row = request->row();
  std::string col = request->col();
  std::string value = request->value();

  if (tablet_->map.find(row) == tablet_->map.end()) {
    tablet_->map[row] = std::unordered_map<std::string, std::string>();
  }

  tablet_->map[row][col] = value;

  response->set_status(KVResStatus::SUCCESS);
  response->set_message("OK");
  return Status::OK;
}

Status KVStoreServiceImpl::Get(ServerContext* context,
                               const KVGetRequest* request,
                               KVGetResponse* response) {
  std::string row = request->row();
  std::string col = request->col();

  if (tablet_->map.find(row) == tablet_->map.end() ||
      tablet_->map[row].find(col) == tablet_->map[row].end()) {
    response->set_status(KVResStatus::FAILURE);
    response->set_error_msg("ERR entry not found.");
  } else {
    response->set_status(KVResStatus::SUCCESS);
    response->set_message(tablet_->map[row][col]);
  }

  return Status::OK;
}

Status KVStoreServiceImpl::CPut(ServerContext* context,
                                const KVCPutRequest* request,
                                KVCPutResponse* response) {
  std::string row = request->row();
  std::string col = request->col();
  std::string prev_value = request->prev_value();
  std::string value = request->value();

  if (tablet_->map.find(row) == tablet_->map.end() ||
      tablet_->map[row].find(col) == tablet_->map[row].end()) {
    response->set_status(KVResStatus::FAILURE);
    response->set_error_msg("ERR entry not found.");
  } else {
    std::string old_value = tablet_->map[row][col];
    if (old_value.compare(prev_value) != 0) {
      response->set_status(KVResStatus::FAILURE);
      response->set_error_msg("ERR previous value not match.");
    } else {
      tablet_->map[row][col] = value;
      response->set_status(KVResStatus::SUCCESS);
      response->set_message("OK");
    }
  }
  return Status::OK;
}

Status KVStoreServiceImpl::Delete(ServerContext* context,
                                  const KVDeleteRequest* request,
                                  KVDeleteResponse* response) {
  std::string row = request->row();
  std::string col = request->col();

  if (tablet_->map.find(row) == tablet_->map.end() ||
      tablet_->map[row].find(col) == tablet_->map[row].end()) {
    response->set_status(KVResStatus::FAILURE);
    response->set_error_msg("ERR entry not found.");
  } else {
    std::string old_value = tablet_->map[row][col];
    tablet_->map[row].erase(col);
    if (tablet_->map[row].size() == 0) {
      tablet_->map.erase(row);
    }
    response->set_status(KVResStatus::SUCCESS);
    response->set_message(old_value);
  }

  return Status::OK;
}

}  // namespace KVStore

int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:10000");
  KVStore::KVStoreServiceImpl service;

  KVStore::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();

  return 0;
}