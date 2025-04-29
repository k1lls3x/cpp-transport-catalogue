#include "transport_router.h"

namespace transport_router {
  TransportRouter::TransportRouter( const transport::TransportCatalogue& catalogue,
                                    const RoutingSettings& settings): catalogue_(catalogue), settings_(settings),
                                    graph_(catalogue.GetAllStops().size()) {
  BuildGraph();
  router_ = std::make_unique<graph::Router<double>>(graph_);
  }
  std::optional<RouteInfo> TransportRouter:: BuildRoute(const transport::Stop* from, const transport::Stop* to) const {
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
  void TransportRouter::BuildGraph() {
     // 0. Пронумеровали остановки: каждая остановка получит свой индекс arrival
    int stop_count = 0;
    for (const auto* stop : catalogue_.GetAllStops()) {
       stop_to_vertex_id_[stop] = stop_count++;
   }
    // Построим граф на 2 * stop_count вершинах: [0..stop_count-1] — arrival,
    // [stop_count..2*stop_count-1] — departure
    graph_ = graph::DirectedWeightedGraph<double>(stop_count * 2);
   // 1. Рёбра «Wait»: обязательно из arrival → departure, вес = bus_wait_time
    for (const auto& [stop, arrival] : stop_to_vertex_id_) {
          const graph::VertexId dep = arrival + stop_count;
          graph_.AddEdge({arrival, dep, static_cast<double>(settings_.bus_wait_time)});
          edges_info_.push_back({EdgeInfo::Type::Wait, stop->name, 0});
      }

    // λ-функция для перевода метров -> минут
    const auto dist_to_time = [&](double m) {
        return m / (settings_.bus_velocity * 1000.0 / 60.0);
    };

    // 2. Рёбра «Bus»
    for (const transport::Bus* bus : catalogue_.GetAllBuses()) {
        const auto& stops = bus->stops;
        const int n = static_cast<int>(stops.size());
        if (n < 2) continue;

        // ---- прямое направление ----
        for (int i = 0; i < n - 1; ++i) {
            double dist_sum = 0.0;
            for (int j = i + 1, span = 1; j < n; ++j, ++span) {
              dist_sum += catalogue_.GetDistance(stops[j - 1], stops[j]);
                // из departure stops[i] → arrival stops[j]
                graph_.AddEdge({
                    stop_to_vertex_id_[stops[i]] + stop_count,
                    stop_to_vertex_id_[stops[j]],
                    dist_to_time(dist_sum)
                });
                edges_info_.push_back({EdgeInfo::Type::Bus, bus->name, span});
            }
        }

        // ---- обратное направление для некольцевого ----
        if (!bus->is_roundtrip) {
            for (int i = n - 1; i > 0; --i) {
                double dist_sum = 0.0;
                for (int j = i - 1, span = 1; j >= 0; --j, ++span) {
                  dist_sum += catalogue_.GetDistance(stops[j + 1], stops[j]);
                                      // из departure stops[i] → arrival stops[j]
                 graph_.AddEdge({
                                stop_to_vertex_id_[stops[i]] + stop_count,
                                stop_to_vertex_id_[stops[j]],
                                dist_to_time(dist_sum)
                                });
                edges_info_.push_back({EdgeInfo::Type::Bus, bus->name, span});
                }
            }
        }
    }
}

}// namespace transport_router

