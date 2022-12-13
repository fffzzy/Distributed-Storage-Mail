#include "kvstore_node_utils.h"
namespace KVStore {
std::string database_path = "../../Database/";

std::string GetNodeDirPath(int node_idx) {
  return database_path + "node" + std::to_string(node_idx) + "/";
}

std::string GetTabletFilePath(int node_idx, int tablet_idx) {
  return GetNodeDirPath(node_idx) + "node" + std::to_string(node_idx) +
         "_tablet" + std::to_string(tablet_idx) + ".txt";
}

std::string GetLogFilePath(int node_idx) {
  return GetNodeDirPath(node_idx) + "node" + std::to_string(node_idx) +
         "_log.txt";
}

int WriteTabletToFile(Tablet* tablet) {
  // if file does not exist, create new file
  std::string path = tablet->path;
  std::fstream tablet_file;
  tablet_file.open(path,
                   std::fstream::in | std::fstream::out | std::fstream::trunc);
  if (!tablet_file) {
    tablet_file.open(
        path, std::fstream::in | std::fstream::out | std::fstream::trunc);
  }
  // close file
  tablet_file.close();

  /* write to file */
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      map = tablet->map;
  std::ofstream file(path);
  if (!file.is_open()) {
    fprintf(stderr, "cannot open %s\n", path);
    return -1;
  }
  // firstly write numRows, ended by \n
  file << map.size() << "\n";
  // for each row
  for (std::pair<std::string, std::unordered_map<std::string, std::string>>
           row_entry : map) {
    std::string row_key = row_entry.first;
    std::unordered_map<std::string, std::string> row = row_entry.second;
    // write row key and numCols, splited by a space and ended by \n
    file << row_key << " " << row.size() << "\n";

    // for each col
    for (std::pair<std::string, std::string> col_entry : row) {
      std::string col_key = col_entry.first;
      std::string cell = col_entry.second;
      // write col key <sp> cell size <sp> cell , ended by \n
      file << col_key << " " << cell.length() << " " << cell << "\n";
    }
  }
  // write finishes, close file
  file.close();

  return 0;
}

Tablet* LoadTabletFromFile(int node_idx, int tablet_idx) {
  std::string tablet_path = GetTabletFilePath(node_idx, tablet_idx);

  std::ifstream file(tablet_path);
  if (!file.is_open()) {
    return NULL;
  }

  Tablet* tablet = new Tablet();
  tablet->tablet_idx = tablet_idx;
  tablet->path = tablet_path;

  int num_rows;
  file >> num_rows;

  // insert each row
  for (int r = 0; r < num_rows; r++) {
    std::string row_key;
    int num_cols;
    file >> row_key >> num_cols;
    tablet->map[row_key] = std::unordered_map<std::string, std::string>();

    // insert each col
    for (int c = 0; c < num_cols; c++) {
      std::string col_key;
      int cell_size;
      file >> col_key >> cell_size;

      // skip a whitespace
      char ignorebuffer[10];
      file.read(ignorebuffer, 1);
      char buffer[cell_size];
      file.read(buffer, cell_size);
      std::string cell = std::string(buffer, cell_size);

      // insert to map
      tablet->map[row_key][col_key] = cell;
    }
  }

  return tablet;
}

int Digest2TabletIdx(int digest, int num_tablet_total) {
  return digest % num_tablet_total;
}

}  // namespace KVStore