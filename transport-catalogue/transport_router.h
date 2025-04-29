#pragma once
// transport_router.h
#include <algorithm>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "transport_catalogue.h"
#include "router.h"

namespace transport_router {
  struct RoutingSettings{
    int bus_wait_time = 0; // minutes
    double bus_velocity = 0.0; // speed in km/h
  };
  struct RouteItem {
    enum class Type { Wait, Bus };
    Type type;
    std::string name;  // либо название остановки для ожидания, либо автобуса для поездки
    int span_count = 0; // только для Bus
    double time = 0.0;
};
struct EdgeInfo {
  enum class Type { Wait, Bus };
  Type type;
  std::string name; // имя остановки (для Wait) или автобуса (для Bus)
  int span_count = 0; // для автобуса, количество остановок
};
struct RouteInfo {
    double total_time = 0.0;
    std::vector<RouteItem> items;
};
  class TransportRouter{
  public:
    TransportRouter( const transport::TransportCatalogue& catalogue,
                     const RoutingSettings& settings);
    std::optional<RouteInfo> BuildRoute(const transport::Stop* from, const transport::Stop* to) const;
  private:
    void BuildGraph();
    const transport::TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    std::vector<EdgeInfo> edges_info_;
    std::unordered_map<const transport::Stop*, graph::VertexId> stop_to_vertex_id_;
  };
}// namespace transport_router
