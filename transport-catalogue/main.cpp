#include "json.h"
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

int main() {
    using namespace std;

    const json::Document doc = json::Load(cin);

    transport::TransportCatalogue catalogue;
    input::ReadBaseRequests(doc, catalogue);

    const auto& root_map = doc.GetRoot().AsMap();
    render::RenderSettings settings;
    auto rs_it = root_map.find("render_settings");
    if (rs_it != root_map.end() && rs_it->second.IsMap()) {
        settings = render_config::ParseRenderSettings(rs_it->second.AsMap());
    }

    render::MapRenderer renderer{settings};

    // ✅ Вместо прямого вывода SVG — вывод JSON-ответов
    output::ReadStatRequests(doc, catalogue, renderer);

    return 0;
}
