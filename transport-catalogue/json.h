#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;


class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using NodeValue = std::variant<std::nullptr_t, int, double, bool, std::string, Array, Dict>;

    Node() : value_(nullptr) {}
    explicit Node(Array array);
    explicit Node(Dict map);
    explicit Node(int value);
    explicit Node(double value);
    explicit Node(bool value);
    explicit Node(std::string value);
    explicit Node(std::nullptr_t);

    bool IsInt() const;
    bool IsDouble() const;      
    bool IsPureDouble() const;  
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

private:
    NodeValue value_;
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};


Document Load(std::istream& input);
void Print(const Document& doc, std::ostream& output);

}  // namespace json
