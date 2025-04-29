#include "json.h"
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "transport_router.h"

int main() {
  using namespace std;

  const json::Document doc = json::Load(cin);
  const auto& root = doc.GetRoot().AsDict();

  transport::TransportCatalogue catalogue = input::ReadTransportCatalogue(doc);

  render::RenderSettings settings;
  if (auto rs_it = root.find("render_settings");
      rs_it != root.end() && rs_it->second.IsDict()) {
      settings = render_config::ParseRenderSettings(rs_it->second.AsDict());
  }

  render::MapRenderer renderer(settings);

  transport_router::RoutingSettings routing_settings;
  if (auto rt_it = root.find("routing_settings");
      rt_it != root.end() && rt_it->second.IsDict()) {
      routing_settings = routing_config::ParseRoutingSettings(rt_it->second.AsDict());
  }

  transport_router::TransportRouter router(catalogue, routing_settings);

  const auto stat = output::ReadStatRequests(doc, catalogue, renderer, router);

  json::Print(json::Document(json::Node(std::move(stat.responses))), std::cout);

  return 0;
}
