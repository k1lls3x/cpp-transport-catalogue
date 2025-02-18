#include "stat_reader.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>

#include "transport_catalogue.h"
#include "input_reader.h"

void ParseAndPrintStat(const transport::TransportCatalogue& catalogue, 
                        std::string_view request, 
                        std::ostream& output) {

    auto space_pos = request.find(' ');
    if (space_pos == request.npos) {
        output << "Invalid request\n";
        return;
    }

    std::string_view command = request.substr(0, space_pos);
    std::string_view name = input::parsing::Trim(request.substr(space_pos + 1));

    if (command == "Bus") {
        auto info = catalogue.GetBusInfo(name);
        if (!info.exists) {
            output << "Bus " << name << ": not found\n";
        } else {
            output << "Bus " << name << ": "
                   << info.total_stops << " stops on route, "
                   << info.unique_stops << " unique stops, "
                   << std::fixed << std::setprecision(6) << info.route_length << " route length\n";
        }
    } else if (command == "Stop") {
        const transport::Stop* stop = catalogue.FindStop(name);
        if (!stop) {
            output << "Stop " << name << ": not found\n";
        } else {
            auto buses = catalogue.GetBusesForStop(name);
            if (buses.empty()) {
                output << "Stop " << name << ": no buses\n";
            } else {
                output << "Stop " << name << ": buses";
                for (const auto& bus : buses) {
                    output << " " << bus;
                }
                output << "\n";
            }
        }
    } else {
        output << "Invalid request\n";
    }
}