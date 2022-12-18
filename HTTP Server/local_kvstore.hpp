#pragma once
#include <string>
#include <unordered_map>
// #include "../KVStore/kvstore.grpc.pb.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

class KVStoreClient
{
private:
    // std::unique_ptr<KVStoreMaster::Stub> kvstore_master_;

    unordered_map<string, unordered_map<string, string>> table;
    bool is_verbose;

public:
    KVStoreClient(std::string kvstore_addr, bool is_verbose) : is_verbose(is_verbose)
    {
        table["123"]["password"] = "456";

        json mails = {
            {
                {"mailId", 7},
                {"sender", "rjx@localhost"},
                {"recipients", {"rjx@localhost", "123@localhost"}},
                {"subject", "This is the mail title"},
                {"content", "This is the mail content"},
                {"time", "Sun Dec 18 19:32:05 2022"},
            },
            {
                {"mailId", 8},
                {"sender", "safe@localhost"},
                {"recipients", {"123@localhost", "abaaba@localhost"}},
                {"subject", "This is the mail title"},
                {"content", "This is the mail content"},
                {"time", "Sun Dec 18 19:32:06 2022"},
            },
            {
                {"mailId", 9},
                {"sender", "rsefs@localhost"},
                {"recipients", {"123@localhost", "abaaba@localhost"}},
                {"subject", "This is the mail title"},
                {"content", "This is the mail content"},
                {"time", "Sun Dec 18 19:32:04 2022"},
            }};

        table["123"]["mails"] = mails.dump();
    };

    // Read commands from STDIN.
    void Run(){};
    std::string GetNodeAddr(const std::string &row, const std::string &col) { return ""; };
    std::string Get(const std::string &row, const std::string &col)
    {
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
             const std::string &value)
    {
        table[row][col] = value;
        if (is_verbose) {
            cout << "Local kvstore just added a value: " + value + " at row: " + row + " at column: " + col << endl;
        }
    };
    void CPut(const std::string &row, const std::string &col,
              const std::string &cur_value, const std::string &new_value){};
    void Delete(const std::string &row, const std::string &col)
    {
        table[row].erase(col);
    };
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
