#include "transport_catalogue.h"
namespace transport {
    void TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates) {
        stops_.push_back({std::move(name), coordinates});
        const auto& stop = stops_.back();
        stop_name_to_stop_[stop.name] = &stop;
    }

    void TransportCatalogue::AddBus(std::string name, const std::vector<std::string>& stop_names) {
        Bus bus;
        bus.name = std::move(name);

        for (const auto& stop_name : stop_names) {
            if (auto it = stop_name_to_stop_.find(stop_name); it != stop_name_to_stop_.end()) {
                bus.stops.push_back(it->second);
                
                stop_to_buses_[it->second].insert(bus.name);
            }
        }

        buses_.push_back(std::move(bus));
        const auto& inserted_bus = buses_.back();
        bus_name_to_bus_[inserted_bus.name] = &inserted_bus;
    }

    const Stop* TransportCatalogue::FindStop(std::string_view name) const {
        if (auto it = stop_name_to_stop_.find(name); it != stop_name_to_stop_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const Bus* TransportCatalogue::FindBus(std::string_view name) const {
        if (auto it = bus_name_to_bus_.find(name); it != bus_name_to_bus_.end()) {
            return it->second;
        }
        return nullptr;
    }

    TransportCatalogue::BusInfo TransportCatalogue::GetBusInfo(std::string_view name) const {
        BusInfo info;
        auto it = bus_name_to_bus_.find(name);
        if (it == bus_name_to_bus_.end()) return info;

        const Bus* bus = it->second;
        info.total_stops = bus->stops.size();

        std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
        info.unique_stops = unique_stops.size();

        for (size_t i = 1; i < bus->stops.size(); ++i) {
            info.route_length += ComputeDistance(
                bus->stops[i-1]->coordinates, bus->stops[i]->coordinates
            );
        }

        info.exists = true;
        return info;
    }

    std::set<std::string> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
        const Stop* stop = FindStop(stop_name);
        if (!stop) {
            return {};
        }

        if (auto it = stop_to_buses_.find(stop); it != stop_to_buses_.end()) {
            return it->second;
        }

        return {};
    }
}