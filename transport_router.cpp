#include "transport_router.h"

namespace transport_router {
  TransportRouter::TransportRouter( const transport::TransportCatalogue& catalogue,
                                    const RoutingSettings& settings): catalogue_(catalogue), settings_(settings),
                                    graph_(catalogue.GetAllStops().size()) {
  BuildGraph();
  router_ = std::make_unique<graph::Router<double>>(graph_);
  }
  std::optional<RouteInfo> TransportRouter:: GetOptimalRoute(const transport::Stop* from, const transport::Stop* to) const {
       // 1. Найти вершины по остановкам
       auto it_from = stop_to_vertex_id_.find(from);
       auto it_to = stop_to_vertex_id_.find(to);

       if (it_from == stop_to_vertex_id_.end() || it_to == stop_to_vertex_id_.end()) {
           return std::nullopt;
       }

       graph::VertexId from_id = it_from->second;
       graph::VertexId to_id = it_to->second;

       // 2. Вызвать маршрутизатор
       auto route = router_->BuildRoute(from_id, to_id);
       if (!route) {
           return std::nullopt; // Нет маршрута
       }

       // 3. Заполняем итоговую информацию о маршруте
       RouteInfo route_info;
       route_info.total_time = route->weight;

       // 4. Восстанавливаем маршрут по рёбрам
       for (graph::EdgeId edge_id : route->edges) {
           const auto& edge = graph_.GetEdge(edge_id);
           const auto& edge_info = edges_info_.at(edge_id);

           RouteItem item;
           item.time = edge.weight;
           item.name = edge_info.name;

           if (edge_info.type == EdgeInfo::Type::Wait) {
               item.type = RouteItem::Type::Wait;
               item.span_count = 0;
           } else if (edge_info.type == EdgeInfo::Type::Bus) {
               item.type = RouteItem::Type::Bus;
               item.span_count = edge_info.span_count;
           }

           route_info.items.push_back(std::move(item));
       }

       return route_info;
  }
  void TransportRouter::AddWaitEdges(int stop_count) {
    for (const auto& [stop, arrival] : stop_to_vertex_id_) {
        const graph::VertexId dep = arrival + stop_count;
        graph_.AddEdge({arrival, dep, static_cast<double>(settings_.bus_wait_time)});
        edges_info_.push_back({EdgeInfo::Type::Wait, stop->name, 0});
    }
}
void TransportRouter::AddBusEdges(const std::vector<const transport::Stop*>& stops, const std::string& bus_name, bool forward, int stop_count) {
  int n = static_cast<int>(stops.size());

  if (forward) {
      for (int i = 0; i < n - 1; ++i) {
          AddBusSpanEdges(i, n, 1, stops, bus_name, stop_count, true);
      }
  } else {
      for (int i = n - 1; i > 0; --i) {
          AddBusSpanEdges(i, -1, -1, stops, bus_name, stop_count, false);
      }
  }
}

void TransportRouter::AddBusSpanEdges(int start, int end, int step,
  const std::vector<const transport::Stop*>& stops,
  const std::string& bus_name, int stop_count, bool forward) {

  double dist_sum = 0.0;
  int span = 1;

  for (int j = start + step; j != end; j += step, ++span) {
      dist_sum += forward
          ? catalogue_.GetDistance(stops[j - 1], stops[j])
          : catalogue_.GetDistance(stops[j + 1], stops[j]);

      graph_.AddEdge({
          stop_to_vertex_id_[stops[start]] + stop_count,
          stop_to_vertex_id_[stops[j]],
          ConvertDistanceToTime(dist_sum)
      });

      edges_info_.push_back({EdgeInfo::Type::Bus, bus_name, span});
  }
}
  void TransportRouter::BuildGraph() {
      // 0. Пронумеровали остановки
    int stop_count = 0;
    for (const auto* stop : catalogue_.GetAllStops()) {
        stop_to_vertex_id_[stop] = stop_count++;
    }

    // 1. Инициализируем граф
    graph_ = graph::DirectedWeightedGraph<double>(stop_count * 2);

    // 2. Добавим рёбра ожидания (Wait)
    AddWaitEdges(stop_count);

    // 3. Добавим рёбра движения по автобусам (Bus)
    for (const transport::Bus* bus : catalogue_.GetAllBuses()) {
        const auto& stops = bus->stops;
        if (stops.size() < 2) continue;

        const std::string& bus_name = bus->name;
        AddBusEdges(stops, bus_name, true, stop_count);
        if (!bus->is_roundtrip) {
            AddBusEdges(stops, bus_name, false, stop_count);
        }
    }
}

}// namespace transport_router

