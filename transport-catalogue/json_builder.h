#pragma once

#include "json.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

namespace json {

class Builder;
class KeyItemContext;
class ValueAfterKeyContext;
class ArrayItemContext;

enum class Context {
    Empty,
    DictKey,
    DictValue,
    Array,
    Complete
};

class Builder {
public:
    Builder() = default;

    KeyItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& Value(Node::Value value);  
    Builder& Key(std::string key);     
    Builder& EndDict();                 
    Builder& EndArray();               
    Node Build();

private:
    Node node_;
    std::vector<Node*> node_stack_;
    std::vector<std::string> keys_stack_;
    Context status_ = Context::Empty;

    void InsertValue(Node::Value value);

    friend class KeyItemContext;
    friend class ValueAfterKeyContext;
    friend class ArrayItemContext;
};

class KeyItemContext {
public:
    explicit KeyItemContext(Builder& builder) : builder_(builder) {}

    ValueAfterKeyContext Key(std::string key);
    Builder& EndDict();

private:
    Builder& builder_;
};

class ValueAfterKeyContext {
public:
    explicit ValueAfterKeyContext(Builder& builder) : builder_(builder) {}

    KeyItemContext Value(Node::Value value);
    KeyItemContext StartDict();
    ArrayItemContext StartArray();

private:
    Builder& builder_;
};

class ArrayItemContext {
public:
    explicit ArrayItemContext(Builder& builder) : builder_(builder) {}

    ArrayItemContext Value(Node::Value value);
    KeyItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndArray();

private:
    Builder& builder_;
};

}  // namespace json
