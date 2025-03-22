#include "map_renderer.h"
#include <set>
#include <string_view>
#include <algorithm>
#include <unordered_set>
namespace render
{   
    MapRenderer::MapRenderer(RenderSettings settings) : settings_(std::move(settings)){}

    svg::Document MapRenderer::RenderMap(const transport::TransportCatalogue& catalogue) const {
        svg::Document doc;

        std::unordered_set<const transport::Stop*> used_stops;
        std::vector<const transport::Bus*> buses = catalogue.GetAllBuses();
        for (const transport::Bus* bus : buses){
            for (const transport::Stop* stop :bus->stops){
                used_stops.insert(stop);
            }
        } 
        std::vector<geo::Coordinates> geo_coords;
        geo_coords.reserve(used_stops.size());
        for (const transport::Stop* stop : used_stops) {
            geo_coords.push_back(stop->coordinates);
        }

        SphereProjector projector(geo_coords.begin(),geo_coords.end(),
                                  settings_.width,settings_.height,settings_.padding
                                 );

        std::sort(buses.begin(),buses.end(),[](const transport::Bus* lhs, 
                                               const transport::Bus* rhs){
            return lhs ->name < rhs -> name;
        });
      RenderBusLines(doc, buses, projector);       // 1. маршруты
      RenderBusLabels(doc, buses, projector);      // 2. подписи маршрутов
      RenderStopPoints(doc, used_stops, projector); // 3. кружки остановок
      RenderStopLabels(doc, used_stops, projector); // 4. подписи остановок

      return doc;
    
    }
    void MapRenderer::RenderBusLines(svg::Document& doc,const std::vector<const transport::Bus*>& buses, const SphereProjector& projector) const {

            size_t color_index =0;
            for (const transport::Bus* bus : buses){
                if (bus ->stops.empty()) continue;
                svg::Polyline line;
                for (const transport::Stop* stop: bus->stops){
                    line.AddPoint(projector(stop->coordinates));
                }
                line.SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()])
                .SetFillColor(svg::NoneColor)
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    
            doc.Add(line);
            ++color_index;
            }
        }
    
    void MapRenderer::RenderBusLabels(svg::Document& doc,const std::vector<const transport::Bus*>& buses, const SphereProjector& projector) const{
            
            size_t color_index =0;
            for (const transport::Bus* bus:buses){
                if (bus->stops.empty()) continue;
                const svg::Point pos = projector(bus->stops.front()->coordinates);
                const svg::Color& color = settings_.color_palette[color_index % settings_.color_palette.size()];
              
               RenderTextLayer(doc, bus->name, pos, color, true, TextType::Bus);
                RenderTextLayer(doc, bus->name, pos, color, false, TextType::Bus);
        
           if (!bus->is_roundtrip && bus->stops.size() >= 2) {
    size_t mid = bus->stops.size() / 2;
    const transport::Stop* mid_stop = bus->stops[mid];

    // Проверим, чтобы не дублировать начало маршрута
    if (mid_stop != bus->stops.front()) {
        const svg::Point end_pos = projector(mid_stop->coordinates);
        RenderTextLayer(doc, bus->name, end_pos, color, true, TextType::Bus);
        RenderTextLayer(doc, bus->name, end_pos, color, false, TextType::Bus);
    }
}
        
                ++color_index;
            }
            }

            

    void MapRenderer::RenderStopPoints(svg::Document& doc,const std::unordered_set<const transport::Stop*>& stops,const SphereProjector& projector) const{
        std::vector<const transport::Stop*> sorted_stops(stops.begin(), stops.end());
        std::sort(sorted_stops.begin(), sorted_stops.end(),
                  [](const transport::Stop* lhs, const transport::Stop* rhs) {
                      return lhs->name < rhs->name;
                  });
    
        for (const transport::Stop* stop : sorted_stops) {
            svg::Circle circle;
            circle.SetCenter(projector(stop->coordinates))
                  .SetRadius(settings_.stop_radius)
                  .SetFillColor("white");
            doc.Add(circle);
        }
            }

    void MapRenderer::RenderStopLabels(svg::Document& doc,const std::unordered_set<const transport::Stop*>& stops,const SphereProjector& projector) const{
        std::vector<const transport::Stop*> sorted_stops(stops.begin(), stops.end());
        std::sort(sorted_stops.begin(), sorted_stops.end(),
                  [](const transport::Stop* lhs, const transport::Stop* rhs) {
                      return lhs->name < rhs->name;
                  });
    
        for (const transport::Stop* stop : sorted_stops) {
            svg::Point pos = projector(stop->coordinates);
    
           RenderTextLayer(doc, stop->name, pos, "black", true, TextType::Stop);
            RenderTextLayer(doc, stop->name, pos, "black", false, TextType::Stop);

        }
    }
 
    void MapRenderer::RenderTextLayer(svg::Document& doc, const std::string& text,
                                    svg::Point pos, svg::Color color,
                                    bool underlayer, TextType type) const {
        svg::Text txt;
        txt.SetData(text)
        .SetPosition(pos)
        .SetOffset(type == TextType::Bus ? settings_.bus_label_offset : settings_.stop_label_offset)
        .SetFontSize(type == TextType::Bus ? settings_.bus_label_font_size : settings_.stop_label_font_size)
        .SetFontFamily("Verdana")
        .SetFillColor(underlayer ? settings_.underlayer_color : color);

        if (type == TextType::Bus) {
            txt.SetFontWeight("bold");
        }

        if (underlayer) {
            txt.SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        }

        doc.Add(txt);
    }
} // namespace render


