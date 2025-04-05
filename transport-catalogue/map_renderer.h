#pragma once
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}
namespace render
{
    
    enum class TextType {
        Bus,
        Stop
    };
    struct RenderSettings{
        double width = 600.0;
        double height = 600.0;
        double padding = 50.0;
        double line_width = 14.0;
        double stop_radius = 5.0;
        uint32_t bus_label_font_size = 20;
        svg::Point bus_label_offset = {7.0,15.0};
        uint32_t stop_label_font_size = 20;
        svg::Point stop_label_offset = {7.0, -3.0};
        svg::Color underlayer_color = svg::Rgb{255, 255, 255};
        double underlayer_width = 3.0;
        std::vector<svg::Color> color_palette;
    };
    class SphereProjector {
    public:
        
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding);
         
        svg::Point operator()(geo::Coordinates coords) const ;
    
    private:
        double padding_;
        double min_lon_ = 0.0;
        double max_lat_ = 0.0;
        double zoom_coeff_ = 0.0;
    };
    
    class MapRenderer {
        public:
            explicit MapRenderer(RenderSettings settings);
        
            svg::Document RenderMap(const transport::TransportCatalogue& catalogue) const;
        
        private:
            RenderSettings settings_;

            void RenderBusLines(svg::Document& doc,
                                const std::vector<const transport::Bus*>& buses, 
                                const SphereProjector& projector) const;
                            
            void RenderBusLabels(svg::Document& doc,
                                const std::vector<const transport::Bus*>& buses, 
                                const SphereProjector& projector) const;

            void RenderStopPoints(svg::Document& doc,
                                  const std::unordered_set<const transport::Stop*>& stops,
                                  const SphereProjector& projector) const;

            void RenderStopLabels(svg::Document& doc,
                                  const std::unordered_set<const transport::Stop*>& stops,
                                  const SphereProjector& projector) const;
          
            void RenderTextLayer(svg::Document& doc, const std::string& text,
                                  svg::Point pos, svg::Color color,
                                  bool underlayer, TextType type) const;

        };
   
        template <typename PointInputIt>
        inline SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding)
            : padding_(padding) 
           {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end) {
                return;
            }
    
            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;
    
            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;
    
            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }
    
            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }
    
            if (width_zoom && height_zoom) {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            } else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            } else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        
        }
        inline svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }
    
} // namespace render

