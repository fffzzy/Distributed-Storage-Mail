#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;
int main()
{
    std::ifstream f("data.json");
    json data = json::parse(f);
    // Access the values existing in JSON data
    string name = data.value("name", "not found");
    string grade = data.value("grade", "not found");
    // Access a value that does not exist in the JSON data
    string email = data.value("email", "not found");
    // Print the values
    cout << "Name: " << name << endl;
    cout << "Grade: " << grade << endl;
    cout << "Email: " << email << endl;

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

    // Print the values
    std::cout << "First Name: " << fName << std::endl;
    std::cout << "Last Name: " << lName << std::endl;
    std::cout << "Student ID: " << sID << std::endl;
    std::cout << "Department: " << dept << std::endl;
    return 0;
}