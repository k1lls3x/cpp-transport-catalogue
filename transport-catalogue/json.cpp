#include "json.h"

#include <cctype>   
#include <sstream>
#include <string>

using namespace std;

namespace json {


Node::Node(Array array) : value_(std::move(array)) {}
Node::Node(Dict map) : value_(std::move(map)) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(bool value) : value_(value) {}
Node::Node(std::string value) : value_(std::move(value)) {}
Node::Node(std::nullptr_t) : value_(nullptr) {}


bool Node::operator==(const Node& other) const {
    return value_ == other.value_;
}
bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}


bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }
bool Node::IsInt() const { return std::holds_alternative<int>(value_); }
bool Node::IsDouble() const {
    return std::holds_alternative<double>(value_) || IsInt();
}
bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(value_);
}
bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Node::IsString() const { return std::holds_alternative<std::string>(value_); }
bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }
bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }


int Node::AsInt() const {
    if (!IsInt()) throw std::logic_error("Node is not int");
    return std::get<int>(value_);
}
bool Node::AsBool() const {
    if (!IsBool()) throw std::logic_error("Node is not bool");
    return std::get<bool>(value_);
}
double Node::AsDouble() const {
    if (!IsDouble()) throw std::logic_error("Node is not double");
    if (std::holds_alternative<int>(value_)) {
        return static_cast<double>(std::get<int>(value_));
    } else {
        return std::get<double>(value_);
    }
}
const std::string& Node::AsString() const {
    if (!IsString()) throw std::logic_error("Node is not string");
    return std::get<std::string>(value_);
}
const Array& Node::AsArray() const {
    if (!IsArray()) throw std::logic_error("Node is not array");
    return std::get<Array>(value_);
}
const Dict& Node::AsMap() const {
    if (!IsMap()) throw std::logic_error("Node is not dict");
    return std::get<Dict>(value_);
}


Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Node LoadNode(std::istream& input);

// Считываем массив
Node LoadArray(std::istream& input) {
    Array result;
    char c;

    while (true) {
        if (!(input >> c)) {
            throw ParsingError("Array parsing error: unexpected EOF");
        }
        if (c == ']') {
            break;
        }
        input.putback(c);

        result.push_back(LoadNode(input));

        if (!(input >> c)) {
            throw ParsingError("Array parsing error: unexpected EOF after value");
        }
        if (c == ']') {
            break;
        }
        if (c != ',') {
            throw ParsingError(std::string("Expected ',' or ']', but got ") + c);
        }
    }

    return Node(std::move(result));
}


Node LoadString(std::istream& input) {
   
    std::string s;
    char c;
    while (true) {
        if (!input.get(c)) {
            throw ParsingError("String parsing error: unexpected EOF");
        }
        if (c == '"') {
            break;
        } else if (c == '\\') {
       
            if (!input.get(c)) {
                throw ParsingError("String parsing error: bad escape");
            }
            switch (c) {
                case '"':  s.push_back('"');  break;
                case '\\': s.push_back('\\'); break;
                case 'n':  s.push_back('\n'); break;
                case 't':  s.push_back('\t'); break;
                case 'r':  s.push_back('\r'); break;
                default:
            
                    s.push_back(c);
                    break;
            }
        } else {
            s.push_back(c);
        }
    }
    return Node(std::move(s));
}

Node LoadDict(std::istream& input) {
    Dict result;
    char c;

    while (true) {
        if (!(input >> c)) {
            throw ParsingError("Dict parsing error: unexpected EOF");
        }
        if (c == '}') {
            break;
        }
        input.putback(c);

        if (!(input >> c) || c != '"') {
            throw ParsingError("Dict key parsing error: expected '\"'");
        }
        Node key = LoadString(input);
        if (!key.IsString()) {
            throw ParsingError("Dict key must be string");
        }

        // Теперь двоеточие :
        if (!(input >> c) || c != ':') {
            throw ParsingError("Dict parsing error: expected ':' after key");
        }

        // Значение
        Node value = LoadNode(input);
        result.emplace(key.AsString(), std::move(value));

        // Далее либо '}', либо ','
        if (!(input >> c)) {
            throw ParsingError("Dict parsing error: unexpected EOF after value");
        }
        if (c == '}') {
            break;
        }
        if (c != ',') {
            throw ParsingError(std::string("Expected ',' or '}', but got ") + c);
        }
    }

    return Node(std::move(result));
}


Node LoadBool(std::istream& input) {
   
    std::string word;
    while (std::isalpha(input.peek())) {
        word.push_back(static_cast<char>(input.get()));
    }
    if (word == "true") {
        return Node(true);
    } else if (word == "false") {
        return Node(false);
    } else {
        throw ParsingError("Invalid bool: " + word);
    }
}


Node LoadNull(std::istream& input) {
    std::string word;
    while (std::isalpha(input.peek())) {
        word.push_back(static_cast<char>(input.get()));
    }
    if (word == "null") {
        return Node(nullptr);
    } else {
        throw ParsingError("Invalid null: " + word);
    }
}

Node LoadNumber(std::istream& input) {
    std::string num_str;

    bool has_point = false;

    while (true) {
        if (!input.good()) {
            break;
        }
        char c = static_cast<char>(input.peek());
        if ((c == '-' && num_str.empty())     
            || (c >= '0' && c <= '9')         
            || (c == '.' && !has_point)) {    
            input.get();
            num_str.push_back(c);
            if (c == '.') {
                has_point = true;
            }
        } else {
            break;
        }
    }

    if (num_str.empty() || num_str == "-") {
        throw ParsingError("Invalid number: " + num_str);
    }

    if (has_point) {
        return Node(std::stod(num_str));
    } else {
        // иначе int
        return Node(std::stoi(num_str));
    }
}

Node LoadNode(std::istream& input) {
    char c;
    if (!(input >> c)) {
        throw ParsingError("LoadNode error: unexpected EOF");
    }
    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 't' || c == 'f') {
        // bool
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') {
        // null
        input.putback(c);
        return LoadNull(input);
    } else if (c == '-' || isdigit(c)) {
        // number
        input.putback(c);
        return LoadNumber(input);
    } else {
        throw ParsingError(std::string("LoadNode error: unexpected token '") + c + "'");
    }
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

void PrintNode(const Node& node, std::ostream& output);

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}


void PrintNode(const Node& node, std::ostream& output) {
    if (node.IsNull()) {
        output << "null";
    } else if (node.IsBool()) {
        output << (node.AsBool() ? "true" : "false");
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsPureDouble()) {
        output << node.AsDouble();
    } else if (node.IsString()) {
     
        output << "\"";
        for (char c : node.AsString()) {
            switch (c) {
                case '\\': output << "\\\\"; break;
                case '\"': output << "\\\""; break;
                case '\n': output << "\\n"; break;
                case '\t': output << "\\t"; break;
                case '\r': output << "\\r"; break;
                default:   output << c;     break;
            }
        }
        output << "\"";
    } else if (node.IsArray()) {
        output << "[";
        const auto& arr = node.AsArray();
        bool first = true;
        for (const auto& elem : arr) {
            if (!first) {
                output << ", ";
            }
            PrintNode(elem, output);
            first = false;
        }
        output << "]";
    } else if (node.IsMap()) {
        output << "{";
        const auto& dict = node.AsMap();
        bool first = true;
        for (const auto& [key, value] : dict) {
            if (!first) {
                output << ", ";
            }
            output << "\"";
            for (char c : key) {
                switch (c) {
                    case '\\': output << "\\\\"; break;
                    case '\"': output << "\\\""; break;
                    case '\n': output << "\\n"; break;
                    case '\t': output << "\\t"; break;
                    case '\r': output << "\\r"; break;
                    default:   output << c;     break;
                }
            }
            output << "\": ";
            PrintNode(value, output);
            first = false;
        }
        output << "}";
    }
}

}  // namespace json
