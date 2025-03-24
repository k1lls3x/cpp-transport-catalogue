#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"


namespace input {
    
    transport::TransportCatalogue ReadTransportCatalogue(const json::Document& doc);
  
}// namespace input

namespace output {
    struct StatResponse
    {
        std::vector<json::Node> responses;
    };
    
    
    StatResponse ReadStatRequests(const json::Document& doc,
                              const transport::TransportCatalogue& catalogue,
                              const render::MapRenderer& renderer);
}// namespace output

namespace render_config {

    render::RenderSettings ParseRenderSettings(const json::Dict& settings_json);
    
    svg::Color ParseColor(const json::Node& node);

    template <typename T>
    std::optional<T> GetIf(const json::Dict& dict, const std::string& key) {
        auto it = dict.find(key);
        if (it == dict.end()) return std::nullopt;

        const auto& node = it -> second;
        return std::visit([](const auto& value) -> std::optional<T>{
            using U = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<U,T>){
                return value;
            }
            else if constexpr (std::is_same_v<U,int> && std::is_same_v<T,double>)
            {
                return static_cast<double>(value);
            }else{
                return std::nullopt;
            }
            
        }, static_cast<const json::NodeValue&>(node));
                
    }    
}// namespace render_config