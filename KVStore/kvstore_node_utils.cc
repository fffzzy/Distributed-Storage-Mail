#include "kvstore_node_utils.h"
namespace KVStore {

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

}  // namespace KVStore