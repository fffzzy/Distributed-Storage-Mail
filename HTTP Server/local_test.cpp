#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

string urlEncode(string str)
{
    string new_str = "";
    char c;
    int ic;
    const char *chars = str.c_str();
    char bufHex[10];
    int len = strlen(chars);

    for (int i = 0; i < len; i++)
    {
        c = chars[i];
        ic = c;
        // uncomment this if you want to encode spaces with +
        /*if (c==' ') new_str += '+';
        else */
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            new_str += c;
        else
        {
            sprintf(bufHex, "%X", c);
            if (ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}

int main()
{
    // std::ifstream f("data.json");
    // json data = json::parse(f);
    // // Access the values existing in JSON data
    // string name = data.value("name", "not found");
    // string grade = data.value("grade", "not found");
    // // Access a value that does not exist in the JSON data
    // string email = data.value("email", "not found");
    // // Print the values
    // cout << "Name: " << name << endl;
    // cout << "Grade: " << grade << endl;
    // cout << "Email: " << email << endl;

    // jdEmployees
    json jdEmployees = {
        {"firstName", "Sean"},
        {"lastName", "Brown"},
        {"StudentID", 21453},
        {"Department", "Computer Sc."}};

    // Access the values
    std::string fName = jdEmployees.value("firstName", "oops");
    std::string lName = jdEmployees.value("lastName", "oops");
    int sID = jdEmployees.value("StudentID", 0);
    std::string dept = jdEmployees.value("Department", "oops");
    json mails = {
        {
            {"mailId", 7},
            {"sender", "rjx@localhost"},
            {"recipients", {"rjx@localhost", "abaaba@localhost"}},
            {"subject", "This is the mail title"},
            {"content", "This is the mail content"},
            {"time", "Time stamp is here"},
        },
        {
            {"mailId", 8},
            {"sender", "rjx@localhost"},
            {"recipients", {"rjx@localhost", "abaaba@localhost"}},
            {"subject", "This is the mail title"},
            {"content", "This is the mail content"},
            {"time", "Time stamp is here"},
        },
        {
            {"mailId", 8},
            {"sender", "rjx@localhost"},
            {"recipients", {"rjx@localhost", "abaaba@localhost"}},
            {"subject", "This is the mail title"},
            {"content", "This is the mail content"},
            {"time", "Time stamp is here"},
        }};
    // Print the values
    std::cout << "First Name: " << fName << std::endl;
    std::cout << "Last Name: " << lName << std::endl;
    std::cout << "Student ID: " << sID << std::endl;
    std::cout << "Department: " << dept << std::endl;
    cout << mails.dump(4) << endl;
    return 0;
}