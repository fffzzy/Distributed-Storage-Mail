#ifndef __kvstore_h__
#define __kvstore_h__
#include <iostream>
#include <unordered_map>

#define SUCCESS 0
#define ERROR -1

struct tablet_struct {
    std::string path;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> map;
};

// TODO: get cluster path based on ip, get tablet path based on rowkey and colkey
std::string getTabletPath();

// write tablet in MEM to tabletFile
int writeTablet(tablet_struct *tablet);

tablet_struct *loadTablet(std::string tabletPath);

#endif /* defined(__kvstore_h__) */