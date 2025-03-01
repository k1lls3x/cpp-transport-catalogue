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
    void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, int distance){
        distance_[{from,to}] = distance;
    }
    const Stop* TransportCatalogue::FindStop(std::string_view name) const {
        auto it = stop_name_to_stop_.find(name);
        if (it == stop_name_to_stop_.end()) {
            throw std::runtime_error("Stop not found: " + std::string(name));
        }
        return it->second;
    }
    int TransportCatalogue::GetDistance(const Stop* from , const Stop* to) const{
        auto it_from = distance_.find({from,to});
        if (it_from != distance_.end()) return it_from -> second;
        auto it_to = distance_.find({to,from});
        if (it_to != distance_.end()) return it_to -> second;
        return 0;
    }
    const Bus* TransportCatalogue::FindBus(std::string_view name) const {
        auto it = bus_name_to_bus_.find(name);
        if (it == bus_name_to_bus_.end()) {
            throw std::runtime_error("Bus not found: " + std::string(name));
        }
        return it->second;
    }

    transport::BusInfo TransportCatalogue::GetBusInfo(std::string_view name) const {
        BusInfo info;
        auto it = bus_name_to_bus_.find(name);
        if (it == bus_name_to_bus_.end()) return info;

        const Bus* bus = it->second;
        info.total_stops = bus->stops.size();

        std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
        info.unique_stops = unique_stops.size();

        double geo_route_lenght = 0.0;
        info.route_length = 0.0;
        for (size_t i = 1; i < bus->stops.size(); ++i) {
            const Stop* from = bus -> stops[i-1];
            const Stop* to = bus -> stops[i];

            info.route_length+=GetDistance(from,to);
            geo_route_lenght+=ComputeDistance(from-> coordinates, to-> coordinates);
        }
        info.curvature = (geo_route_lenght == 0) ? 0.0 : info.route_length  / geo_route_lenght;
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