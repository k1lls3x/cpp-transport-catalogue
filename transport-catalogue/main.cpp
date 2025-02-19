// main.cpp
#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    transport::TransportCatalogue catalogue;

    int base_request_count;
    cin >> base_request_count >> ws;

    {
        input::InputReader reader;
        reader.ProcessCommands(cin, catalogue);
    }

    int stat_request_count;
    cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(cin, line);
        ParseAndPrintStat(catalogue, line, cout);
    }
}
