#include "kvstore.h"

void printTablet(std::unordered_map<std::string, std::unordered_map<std::string, std::string>> map) {
    for (std::pair<std::string, std::unordered_map<std::string, std::string>> rowEntry : map) {
        std::string rowKey = rowEntry.first;
        std::unordered_map<std::string, std::string> row = rowEntry.second;
        std::cout << rowKey << std::endl;
        for (std::pair<std::string, std::string> colEntry : row) {
            std::string colKey = colEntry.first;
            std::string cell = colEntry.second;
            std::cout << colKey << std::endl;
            std::cout << cell << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    tablet_struct *tablet = new tablet_struct();
    tablet->path = getTabletPath();
    tablet->map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>();

    // insert Alice
    tablet->map["Alice"] = std::unordered_map<std::string, std::string>();
    tablet->map["Alice"]["Alice-Col1"] = "Hello, I'm Alice\n";
    tablet->map["Alice"]["Alcie-Col2"] = "This is the second col for Alice(Before delete)!\n";

    // insert Bob
    tablet->map["Bob"] = std::unordered_map<std::string, std::string>();
    tablet->map["Bob"]["Bob-Col1"] = "This is the first col for Bob (old)\n";
    tablet->map["Bob"]["Bob-Col2"] = "Test splited\n content\n";
    tablet->map["Bob"]["Bob-Col3"] = "\tThis is the third col for Bob\n";

    writeTablet(tablet);
    delete tablet;

    tablet = loadTablet(getTabletPath());
    std::cout << "============Print initial tablet============" << std::endl;
    printTablet(tablet->map);
    // modify:
    // delete alice col 2
    tablet->map["Alice"].erase("Alcie-Col2");
    // update Bob
    tablet->map["Bob"]["Bob-Col1"] = "This is the first col for Bob (new)\n";
    // add Cindy
    tablet->map["Cindy"] = std::unordered_map<std::string, std::string>();
    tablet->map["Cindy"]["Cindy-Col1"] = "Hello World, I'm Cindy.";
    tablet->map["Cindy"]["Cindy-Col2"] = "This is Cindy's second col";

    writeTablet(tablet);
    delete tablet;

    tablet = loadTablet(getTabletPath());
    std::cout << "============Print tablet after modification============" << std::endl;
    printTablet(tablet->map);
    delete tablet;

    return 0;
}