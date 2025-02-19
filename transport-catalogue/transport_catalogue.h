#pragma once
//transport_catalogue.h
#include "geo.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <deque>
#include <unordered_set>
#include <set>
#include <stdexcept>
namespace transport {
    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };
    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
    };
    struct BusInfo {
        int total_stops = 0;
        int unique_stops = 0;
        double route_length = 0.0;
        bool exists = false;
    };

    class TransportCatalogue {
    public:
        void AddStop(std::string name, geo::Coordinates coordinates);
        void AddBus(std::string name, const std::vector<std::string>& stop_names);

        const Stop* FindStop(std::string_view name) const;
        const Bus* FindBus(std::string_view name) const;
        std::set<std::string> GetBusesForStop(std::string_view stop_name) const ;
        BusInfo GetBusInfo(std::string_view name) const;

    private:
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;
        std::unordered_map<std::string_view, const Stop*> stop_name_to_stop_;
        std::unordered_map<std::string_view, const Bus*> bus_name_to_bus_;
        std::unordered_map<const Stop*,std::set<std::string>> stop_to_buses_;
    };
}