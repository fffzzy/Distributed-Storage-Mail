#pragma once
#include <string>
#include <unordered_map>
// #include "../KVStore/kvstore.grpc.pb.h"

using namespace std;

class KVStoreClient
{
public:
    KVStoreClient(std::string kvstore_addr) {};

    // Read commands from STDIN.
    void Run(){};
    std::string GetNodeAddr(const std::string &row, const std::string &col) { return ""; };
    std::string Get(const std::string &row, const std::string &col) {
        if (table.find(row) != table.end())
    {
        if (table[row].find(col) != table[row].end())
        {
            return table[row][col];
        }
        else
        {
            return "";
        };
    }
    else
    {
        return "";
    }
    };
    void Put(const std::string &row, const std::string &col,
             const std::string &value) {
                table[row][col] = value;
             };
    void CPut(const std::string &row, const std::string &col,
              const std::string &cur_value, const std::string &new_value){};
    void Delete(const std::string &row, const std::string &col) {
        table[row].erase(col);
    };

private:
    // std::unique_ptr<KVStoreMaster::Stub> kvstore_master_;

    unordered_map<string, unordered_map<string, string>> table;
};

// string KVStoreClient::Get(const std::string &row, const std::string &col)
// {
//     if (table.find(row) != table.end())
//     {
//         if (table[row].find(col) != table[row].end())
//         {
//             return table[row][col];
//         }
//         else
//         {
//             return "";
//         };
//     }
//     else
//     {
//         return "";
//     }
// }

// void KVStoreClient::Put(const std::string &row, const std::string &col,
//                           const std::string &value){
//     table[row][col] = value;
// };
// void KVStoreClient::Delete(const std::string &row, const std::string &col) {
//     table[row].erase(col);
// };
