#include "json.h"
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

int main() {
    using namespace std;

    const json::Document doc = json::Load(cin);

    transport::TransportCatalogue catalogue = input::ReadTransportCatalogue(doc);

    const auto& root_map = doc.GetRoot().AsMap();
    render::RenderSettings settings;
    if (auto rs_it = root_map.find("render_settings");
        rs_it != root_map.end() && rs_it->second.IsMap()) {
        settings = render_config::ParseRenderSettings(rs_it->second.AsMap());
    }

    render::MapRenderer renderer(settings); // ← вот это добавь

    const auto stat = output::ReadStatRequests(doc, catalogue, renderer);
    json::Print(json::Document(json::Node(std::move(stat.responses))), std::cout);

    return 0;
}
