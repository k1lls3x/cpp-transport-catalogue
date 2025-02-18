#pragma once
//stat_reader.h
#include <iosfwd>
#include <string_view>
#include <iostream>
#include <iomanip>
#include "input_reader.h"
#include "transport_catalogue.h"

void ParseAndPrintStat(const transport::TransportCatalogue& tansport_catalogue, 
                       std::string_view request,
                       std::ostream& output);