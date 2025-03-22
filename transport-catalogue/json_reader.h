#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

namespace input {
    
    void ReadBaseRequests(const json::Document& doc, transport::TransportCatalogue& catalogue);
  
}// namespace input

namespace output {
    
   void ReadStatRequests(const json::Document& doc,
                      transport::TransportCatalogue& catalogue,
                      const render::MapRenderer& renderer);
}// namespace output

namespace render_config {

    render::RenderSettings ParseRenderSettings(const json::Dict& settings_json);
    
    svg::Color ParseColor(const json::Node& node);

template <typename T>
std::optional<T> GetIf(const json::Dict& dict, const std::string& key) {
    auto it = dict.find(key);
    if (it != dict.end()) {
        try {
            if constexpr (std::is_same_v<T, double>) {
                if (it->second.IsDouble()) {
                    return it->second.AsDouble();
                }
            } else if constexpr (std::is_same_v<T, int>) {
                if (it->second.IsInt()) {
                    return it->second.AsInt();
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (it->second.IsString()) {
                    return it->second.AsString();
                }
            } else if constexpr (std::is_same_v<T, bool>) {
                if (it->second.IsBool()) {
                    return it->second.AsBool();
                }
            }
        } catch (...) {
            std::cerr << "Something went wrong\n"; 
        }
    }
    return std::nullopt;
}   
}// namespace render_config