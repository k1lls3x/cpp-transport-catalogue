#include "transport_catalogue.h"
//transport_catalogue.h
namespace transport {
    void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
        stops_.push_back({name, coordinates});
        const auto& stop = stops_.back();
        stop_name_to_stop_[stop.name] = &stop;
    }

    void TransportCatalogue::AddBus(const std::string& name,const std::vector<std::string>& stop_names,bool is_roundtrip) {
        Bus bus;
        bus.name = name;
        bus.is_roundtrip = is_roundtrip;

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
        return it != stop_name_to_stop_.end() ? it->second : nullptr;
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
        return it != bus_name_to_bus_.end() ? it->second : nullptr;
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
    const std::deque<const Bus*>& TransportCatalogue::GetAllBuses() const {
      all_buses_.clear();
      for (const Bus& bus : buses_) {
          all_buses_.push_back(&bus);
      }
      return all_buses_;
    }
    const std::deque<const Stop*>& TransportCatalogue::GetAllStops() const {
      all_stops_.clear();
      for (const auto& stop : stops_) {
          all_stops_.push_back(&stop);
      }
      return all_stops_;
  }
  const std::set<std::string>& TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
        const Stop* stop = FindStop(stop_name);
        static const std::set<std::string> empty_result;
        if (!stop) {
            return empty_result;
        }

        if (auto it = stop_to_buses_.find(stop); it != stop_to_buses_.end()) {
            return it->second;
        }

        return empty_result;
    }
}
