#pragma once
//input_reader.h
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"
namespace input {
    namespace parsing{
        std::string_view Trim(std::string_view string);
    }
    struct CommandDescription {
        explicit operator bool() const {
            return !command.empty();
        }

        bool operator!() const {
            return !operator bool();
        }

        std::string command;      
        std::string id;          
        std::string description;  
    };

    class InputReader {
    public:
       
        void ParseLine(std::string_view line);
        void ApplyCommands(transport::TransportCatalogue& catalogue) const;

    private:
        std::vector<CommandDescription> commands_;
    };
}