#include "json_reader.h"

#include <iostream>
#include <sstream>
using namespace std::literals;
namespace input {

void ReadBaseRequests(const json::Document& doc, transport::TransportCatalogue& catalogue) {
    const auto& root = doc.GetRoot().AsMap();
    std::vector<std::tuple<std::string, std::string, int>> temp_distances;

    auto base_requests_it = root.find("base_requests");
    if (base_requests_it == root.end() || !base_requests_it->second.IsArray()) {
        return;
    }

    const auto& base_requests = base_requests_it->second.AsArray();

    // === 1-й проход: добавляем только остановки ===
    for (const auto& request_node : base_requests) {
        const auto& obj = request_node.AsMap();
        auto type_it = obj.find("type");
        if (type_it == obj.end() || type_it->second.AsString() != "Stop") {
            continue;
        }

        auto name_it = obj.find("name");
        auto lat_it  = obj.find("latitude");
        auto lng_it  = obj.find("longitude");

        if (name_it == obj.end() || lat_it == obj.end() || lng_it == obj.end()) {
            continue;
        }

        const std::string& stop_name = name_it->second.AsString();
        geo::Coordinates coords{ lat_it->second.AsDouble(), lng_it->second.AsDouble() };
        catalogue.AddStop(stop_name, coords);

        // Сохраняем road_distances на потом
        auto rd_it = obj.find("road_distances");
        if (rd_it != obj.end() && rd_it->second.IsMap()) {
            for (const auto& [other_stop_name, dist_node] : rd_it->second.AsMap()) {
                if (dist_node.IsInt()) {
                    temp_distances.emplace_back(stop_name, other_stop_name, dist_node.AsInt());
                }
            }
        }
    }

    // === 2-й проход: сохраняем маршруты во временный буфер ===
    std::vector<std::tuple<std::string, std::vector<std::string>, bool>> temp_buses;

    for (const auto& request_node : base_requests) {
        const auto& obj = request_node.AsMap();
        auto type_it = obj.find("type");
        if (type_it == obj.end() || type_it->second.AsString() != "Bus") {
            continue;
        }

        auto name_it = obj.find("name");
        auto stops_it = obj.find("stops");
        auto roundtrip_it = obj.find("is_roundtrip");

        if (name_it == obj.end() || stops_it == obj.end() || !stops_it->second.IsArray()) {
            continue;
        }

        const std::string& bus_name = name_it->second.AsString();
        bool is_roundtrip = false;
        if (roundtrip_it != obj.end() && roundtrip_it->second.IsBool()) {
            is_roundtrip = roundtrip_it->second.AsBool();
        }

        std::vector<std::string> stops;
        for (const auto& stop_node : stops_it->second.AsArray()) {
            if (stop_node.IsString()) {
                stops.push_back(stop_node.AsString());
            }
        }

        // Расширяем маршрут назад для некольцевого
        if (!is_roundtrip && stops.size() > 1) {
            for (int i = static_cast<int>(stops.size()) - 2; i >= 0; --i) {
                stops.push_back(stops[i]);
            }
        }

        temp_buses.emplace_back(bus_name, std::move(stops), is_roundtrip);
    }

    // === 3-й проход: теперь добавляем маршруты, когда все остановки гарантированно есть ===
    for (const auto& [bus_name, stops, is_roundtrip] : temp_buses) {
        catalogue.AddBus(bus_name, stops, is_roundtrip);
    }

    // === Применяем расстояния между остановками ===
    for (const auto& [from_name, to_name, dist] : temp_distances) {
        const auto* from = catalogue.FindStop(from_name);
        const auto* to = catalogue.FindStop(to_name);
        if (from && to) {
            catalogue.SetDistance(from, to, dist);
        }
    }
}


} // namespace input

namespace render_config
{

    svg::Color ParseColor(const json::Node& node){
       
        if (node.IsString()) {
            return node.AsString();
        } else if (node.IsArray()) {
            const auto& arr = node.AsArray();
            if (arr.size() == 3) {
                return svg::Rgb{
                    static_cast<uint8_t>(arr[0].AsInt()),
                    static_cast<uint8_t>(arr[1].AsInt()),
                    static_cast<uint8_t>(arr[2].AsInt())
                };
            } else if (arr.size() == 4) {
                return svg::Rgba{
                    static_cast<uint8_t>(arr[0].AsInt()),
                    static_cast<uint8_t>(arr[1].AsInt()),
                    static_cast<uint8_t>(arr[2].AsInt()),
                    arr[3].AsDouble()
                };
            }
        }
        return svg::NoneColor;
    }

    render::RenderSettings ParseRenderSettings(const json::Dict& settings_json){
        render::RenderSettings settings;

        if (auto val = GetIf<double>(settings_json, "width")) settings.width = *val;
        if (auto val = GetIf<double>(settings_json, "height")) settings.height = *val;
        if (auto val = GetIf<double>(settings_json, "padding")) settings.padding = *val;
        if (auto val = GetIf<double>(settings_json, "line_width")) settings.line_width = *val;
        if (auto val = GetIf<double>(settings_json, "stop_radius")) settings.stop_radius = *val;
        if (auto val = GetIf<int>(settings_json, "bus_label_font_size")) settings.bus_label_font_size = *val;
        if (auto val = GetIf<int>(settings_json, "stop_label_font_size")) settings.stop_label_font_size = *val;
        if (auto val = GetIf<double>(settings_json, "underlayer_width")) settings.underlayer_width = *val;

        if (auto it = settings_json.find("bus_label_offset");
            it != settings_json.end() && it -> second.IsArray()){
                const auto& arr = it -> second.AsArray();
                if (arr.size() == 2){
                    settings.bus_label_offset = { arr[0].AsDouble(), arr[1].AsDouble()};
                }
        }
        if (auto it = settings_json.find("stop_label_offset");
            it != settings_json.end() && it->second.IsArray()) {
             const auto& arr = it->second.AsArray();
             if (arr.size() == 2) {
                    settings.stop_label_offset = { arr[0].AsDouble(), arr[1].AsDouble() };
                }
        }


        if (auto it = settings_json.find("underlayer_color");
            it != settings_json.end()) {
            settings.underlayer_color = ParseColor(it->second);
        }

   
        if (auto it = settings_json.find("color_palette");
            it != settings_json.end() && it->second.IsArray()) {
            for (const auto& node : it->second.AsArray()) {
                settings.color_palette.push_back(ParseColor(node));
            }
        }

    return settings;

    } 
} // namespace render_config

namespace output {

void ReadStatRequests(const json::Document& doc, transport::TransportCatalogue& catalogue,const render::MapRenderer& renderer) {
    const auto& root = doc.GetRoot().AsMap();
    json::Array responses;

    auto stat_requests_it = root.find("stat_requests");
    if (stat_requests_it == root.end() || !stat_requests_it->second.IsArray()) {
        // Нет stat_requests
        json::Print(json::Document(json::Node(responses)), std::cout);
        return;
    }

    for (const auto& request : stat_requests_it->second.AsArray()) {
        const auto& obj = request.AsMap();

        // id
        auto id_it = obj.find("id");
        if (id_it == obj.end() || !id_it->second.IsInt()) {
            continue;
        }
        int request_id = id_it->second.AsInt();

        // type
        auto type_it = obj.find("type");
        if (type_it == obj.end() || !type_it->second.IsString()) {
            continue;
        }
        const std::string& type = type_it->second.AsString();

        if (type == "Bus") {
            // Запрос о маршруте
            auto name_it = obj.find("name");
            if (name_it == obj.end() || !name_it->second.IsString()) {
                continue;
            }
            const std::string& bus_name = name_it->second.AsString();

            auto bus_info = catalogue.GetBusInfo(bus_name);
                if (!bus_info.exists) {
            responses.push_back(json::Node(json::Dict{
                {"request_id", json::Node(request_id)},
                {"error_message", json::Node(std::string("not found"))}
            }));
        } else {
            responses.push_back(json::Node(json::Dict{
                {"request_id", json::Node(request_id)},
                {"stop_count", json::Node(bus_info.total_stops)},
                {"unique_stop_count", json::Node(bus_info.unique_stops)},
                {"route_length", json::Node(bus_info.route_length)},
                {"curvature", json::Node(bus_info.curvature)}
            }));
        }
        }
        
    else if (type == "Stop") {
        auto name_it = obj.find("name");
        if (name_it == obj.end() || !name_it->second.IsString()) {
            continue;
        }
        const std::string& stop_name = name_it->second.AsString();

        // Проверяем, есть ли такая остановка
        const transport::Stop* stop_ptr = catalogue.FindStop(stop_name);
        if (!stop_ptr) {
            // Остановки нет — "not found"
            responses.push_back(json::Node(json::Dict{
                {std::string("request_id"), json::Node(request_id)},
                {std::string("error_message"), json::Node(std::string("not found"))}
            }));
            continue;
        }

        // Остановка есть. Берём автобусы
        auto buses_set = catalogue.GetBusesForStop(stop_name);
        // buses_set обычно std::set<std::string>, оно уже в алфавитном порядке
        json::Array bus_list;
        for (const auto& bus_name : buses_set) {
            bus_list.push_back(json::Node(bus_name));
        }

        // Печатаем ответ с "buses": [...], даже если bus_list пуст
        responses.push_back(json::Node(json::Dict{
            {std::string("request_id"), json::Node(request_id)},
            {std::string("buses"), json::Node(std::move(bus_list))}
        }));
    }
    else if (type == "Map") {
        std::ostringstream svg_stream;
        svg::Document map = renderer.RenderMap(catalogue);
        map.Render(svg_stream);

        json::Dict result;
        result["request_id"] = json::Node(request_id);
        result["map"] = json::Node(svg_stream.str());
        responses.push_back(json::Node(std::move(result)));
    }
    }

    // Выводим массив ответов
   json::Print(json::Document(json::Node(std::move(responses))), std::cout);
}

} // namespace output
